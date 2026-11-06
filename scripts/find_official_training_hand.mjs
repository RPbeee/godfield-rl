#!/usr/bin/env node
// Bounded visible-UI hand search, not an official distribution experiment.
// This wrapper owns stdin and serializes the existing probe's JSONL commands.
// During search: {"kind":"state"} returns a compact cached report;
// {"kind":"abort"} stops at the next command boundary and returns home.
// After a hit/limit/abort, JSONL commands pass through to the same live probe.
import { spawn } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import path from 'node:path';
import fs from 'node:fs/promises';
import readline from 'node:readline';

const emit = value => process.stdout.write(JSON.stringify(value) + '\n');
const error = code => { const e = new Error(code); e.code = code; return e; };
function argumentsFor(args) {
  const values = {};
  for (let i = 0; i < args.length; i += 2) {
    if (!['--playwright', '--out-dir', '--max-trials', '--targets'].includes(args[i]) ||
        !args[i + 1] || args[i + 1].startsWith('--') || values[args[i]] !== undefined)
      throw error('invalid_cli');
    values[args[i]] = args[i + 1];
  }
  if (!values['--playwright'] || !values['--out-dir']) throw error('missing_cli');
  const count = values['--max-trials'] ?? '12';
  if (!/^[1-9]\d*$/.test(count) || Number(count) > 12) throw error('invalid_max_trials');
  const requested = (values['--targets'] ?? '34,83,216').split(',');
  if (!requested.length || requested.some(x => !['3', '34', '83', '216'].includes(x)) ||
      new Set(requested).size !== requested.length) throw error('invalid_targets');
  return { package: path.resolve(values['--playwright']), directory: path.resolve(values['--out-dir']),
    limit: Number(count), targets: requested.map(Number) };
}
const here = path.dirname(fileURLToPath(import.meta.url));
const delay = ms => new Promise(resolve => setTimeout(resolve, ms));
const HAND = { x: 180, y: 480, w: 750, h: 220 };
const hasGF = state => /G\.?\s*F\.?\s*\d+/.test(state?.body ?? '');
const isGF1 = state => /G\.?\s*F\.?\s*1\b/.test(state?.body ?? '');
const isHome = state => !hasGF(state) && (state?.body ?? '').includes('コンピュータと対戦') &&
  (state?.body ?? '').includes('修行') && !(state?.body ?? '').includes('戦いを始める') &&
  !(state?.body ?? '').includes('戦いをあきらめる');
const isSettings = state => !hasGF(state) && /修行者\s*2\s*人/.test(state?.body ?? '') &&
  (state?.body ?? '').includes('終末の時') && (state?.body ?? '').includes('なし') &&
  (state?.body ?? '').includes('戦いを始める');
function handOf(state) {
  return (state?.images ?? []).filter(r => /^\/images\/items\/[a-z-]+\/[a-z0-9-]+\.webp$/.test(r.path) &&
    r.w > 0 && r.h > 0 && r.x + r.w / 2 >= HAND.x && r.x + r.w / 2 < HAND.x + HAND.w &&
    r.y + r.h / 2 >= HAND.y && r.y + r.h / 2 < HAND.y + HAND.h)
    .map(r => ({ path: r.path, x: r.x, y: r.y, w: r.w, h: r.h }))
    .sort((a, b) => a.y - b.y || a.x - b.x);
}

async function main() {
  // All CLI/catalog validation precedes child creation (invalid input never opens a browser).
  const options = argumentsFor(process.argv.slice(2));
  const catalog = JSON.parse(await fs.readFile(path.join(here, '../data/official-v2/cards.json'), 'utf8'));
  const targetPaths = new Map(options.targets.map(id => {
    const card = catalog.find(c => c.id === id);
    if (!card || !['trade', 'weapons', 'miracles'].includes(card.category) || !/^[a-z0-9-]+$/.test(card.imageName))
      throw error('catalog_target_invalid');
    return [`/images/items/${card.category}/${card.imageName}.webp`, id];
  }));
  const child = spawn(process.execPath, [path.join(here, 'official_headless_probe.mjs'),
    '--playwright', options.package, '--out-dir', options.directory], { stdio: ['pipe', 'pipe', 'ignore'] });
  let pending = null, ended = false, searching = true, abortRequested = false;
  let latest = null, attempt = 0, trialDeadline = Infinity, closing = false;
  let phase = 'startup';
  const input = readline.createInterface({ input: process.stdin, crlfDelay: Infinity });
  const output = readline.createInterface({ input: child.stdout, crlfDelay: Infinity });
  let readyResolve, readyReject;
  const ready = new Promise((resolve, reject) => { readyResolve = resolve; readyReject = reject; });
  const readyTimer = setTimeout(() => readyReject(error('startup_timeout')), 25000);
  function settleFailure(e) {
    clearTimeout(readyTimer); readyReject(e);
    if (pending) { const current = pending; pending = null; clearTimeout(current.timer); current.reject(e); }
  }
  child.on('error', () => { ended = true; settleFailure(error('probe_start_failed')); });
  child.on('exit', () => {
    ended = true; settleFailure(error('probe_closed')); input.close();
  });
  child.stdin.on('error', () => settleFailure(error('probe_input_closed')));
  output.on('line', line => {
    let value;
    try { value = JSON.parse(line); } catch { settleFailure(error('probe_invalid_json')); return; }
    if (value.ready === true) {
      clearTimeout(readyTimer); latest = value.state; readyResolve(value); return;
    }
    if (!pending) { if (value.error) settleFailure(error('probe_unsolicited_error')); return; }
    const current = pending;
    if (value.state) latest = value.state;
    if (value.error) {
      pending = null; clearTimeout(current.timer);
      if (current.allowActionErrors) current.resolve([value]);
      else current.reject(error('probe_action_failed'));
      return;
    }
    current.responses.push(value);
    // close emits a capture followed by closed; do not send a second command in between.
    if (current.kind === 'close' && value.closed !== true) return;
    pending = null; clearTimeout(current.timer); current.resolve(current.responses);
  });
  async function send(action, enforceTrial = true, allowActionErrors = false) {
    if (ended) throw error('probe_closed');
    if (pending) throw error('concurrent_command_forbidden');
    const timeout = Math.min(25000, enforceTrial ? trialDeadline - Date.now() : 25000);
    if (timeout <= 0) throw error('trial_timeout');
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        if (!pending || pending.timer !== timer) return;
        pending = null; reject(error('command_timeout'));
        // The command may still be running: never issue another UI action after timeout.
        child.kill('SIGTERM');
      }, timeout);
      pending = { kind: action.kind, responses: [], resolve, reject, timer, allowActionErrors };
      child.stdin.write(JSON.stringify(action) + '\n');
    });
  }
  function compact(status = phase) {
    const hand = handOf(latest);
    return { search: { status, attempt, max_trials: options.limit, targets: options.targets,
      gf1: isGF1(latest), visible_hand: hand,
      matched: hand.filter(r => targetPaths.has(r.path)).map(r => ({ model: targetPaths.get(r.path), ...r })) } };
  }
  function checkAbort() { if (abortRequested) throw error('search_aborted'); }
  async function command(action) { checkAbort(); return send(action); }
  async function poll(predicate, timeout = 25000) {
    const end = Math.min(Date.now() + timeout, trialDeadline);
    while (Date.now() < end) {
      checkAbort();
      await send({ kind: 'state' });
      if (predicate(latest)) return;
      await delay(250);
    }
    throw error('visible_state_timeout');
  }
  async function returnHome(cleanup = false) {
    // Only the observed training header back control is used; unknown UI is not guessed.
    if (isHome(latest)) return;
    if (!hasGF(latest)) throw error('unknown_exit_ui');
    phase = 'quitting';
    await send({ kind: 'clickPoint', x: 240, y: 23 }, !cleanup);
    await send({ kind: 'waitText', text: '戦いをあきらめる' }, !cleanup);
    await send({ kind: 'clickText', text: '戦いをあきらめる' }, !cleanup);
    const end = Math.min(Date.now() + 25000, cleanup ? Infinity : trialDeadline);
    while (Date.now() < end) {
      await send({ kind: 'state' }, !cleanup);
      if (isHome(latest)) return;
      await delay(250);
    }
    throw error('home_return_not_confirmed');
  }
  async function safeClose() {
    if (closing) return;
    closing = true;
    if (!ended && !pending) {
      try { await send({ kind: 'close' }, false); } catch { /* No concurrent retry. */ }
    }
    if (!ended) child.kill('SIGTERM');
    input.close();
  }
  // Exactly one owner reads stdin. Exploration never dispatches parallel child commands.
  let interactiveTail = Promise.resolve();
  input.on('line', line => {
    if (!line.trim()) return;
    let action;
    try { action = JSON.parse(line); } catch { emit({ error: 'invalid_input_json' }); return; }
    if (searching) {
      if (action?.kind === 'abort') { abortRequested = true; emit({ search: { status: 'abort_requested', attempt } }); }
      else if (action?.kind === 'state') emit(compact());
      else emit({ error: 'search_accepts_only_abort_or_state' });
      return;
    }
    interactiveTail = interactiveTail.then(async () => {
      if (!action || typeof action !== 'object' || typeof action.kind !== 'string') throw error('invalid_action');
      const responses = await send(action, false, true);
      for (const response of responses) emit(response);
      if (action.kind === 'close') input.close();
    }).catch(async e => { emit({ error: e.code || 'interactive_failed' }); await safeClose(); });
  });
  // EOF is not a request to keep an abandoned training game/browser alive.
  input.on('close', () => {
    if (closing || ended) return;
    if (searching) abortRequested = true;
    else void interactiveTail.finally(() => safeClose());
  });
  try {
    await ready;
    await poll(s => isHome(s) || ((s?.body ?? '').includes('生誕する') && s.inputs?.length === 1));
    if (!isHome(latest)) {
      await command({ kind: 'fillName', text: 'RulesCheck' });
      await command({ kind: 'clickText', text: '生誕する' });
      await poll(isHome);
    }
    for (attempt = 1; attempt <= options.limit; ++attempt) {
      checkAbort(); trialDeadline = Date.now() + 90000; phase = 'settings';
      if (!isHome(latest)) throw error('trial_not_at_home');
      await command({ kind: 'clickText', text: '修行' });
      // Confirm the visible settings, never silently change an unknown preset.
      await poll(isSettings);
      await command({ kind: 'clickText', text: '戦いを始める' });
      phase = 'waiting_gf1_hand';
      let fingerprint = null;
      await poll(s => {
        const hand = handOf(s);
        if (!isGF1(s) || hand.length !== 9) { fingerprint = null; return false; }
        const value = JSON.stringify(hand);
        if (value === fingerprint) return true;
        fingerprint = value; return false;
      });
      const report = compact('trial_hand'); emit(report);
      if (report.search.matched.length) {
        phase = 'candidate';
        const captures = await command({ kind: 'capture' });
        const screenshots = await command({ kind: 'screenshot' });
        emit({ ...compact('candidate'), capture: captures[0].capture, screenshot: screenshots[0].screenshot });
        // Only one full state is emitted during automatic search, at the candidate stop.
        emit({ state: latest });
        searching = false; trialDeadline = Infinity;
        emit({ interactive: true, reason: 'candidate', attempt });
        if (input.closed) await safeClose();
        return;
      }
      await returnHome(); phase = 'home';
      if (attempt < options.limit) await delay(500);
    }
    attempt = options.limit; phase = 'limit'; searching = false; trialDeadline = Infinity;
    emit({ ...compact('limit'), interactive: true, home_confirmed: isHome(latest) });
    if (input.closed) await safeClose();
  } catch (e) {
    if (e.code === 'search_aborted') {
      try {
        await returnHome(true); searching = false; phase = 'aborted';
        emit({ ...compact('aborted'), interactive: true, home_confirmed: true });
        if (input.closed) await safeClose();
        return;
      } catch { /* Unknown or unconfirmed exit never starts a further trial. */ }
    }
    emit({ error: e.code || 'search_failed', attempt, phase, further_trials_cancelled: true,
      home_confirmed: isHome(latest), browser_close_requested: true });
    await safeClose(); process.exitCode = 1;
  }
}
try { await main(); } catch (e) { emit({ error: e.code || 'startup_failed' }); process.exitCode = 1; }
