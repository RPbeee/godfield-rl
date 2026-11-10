#!/usr/bin/env node
// Interactive visible-UI probe. No application-state evaluation, protocol replay,
// request bodies, headers, cookies, HAR, or persistent browser profile.
// node scripts/official_headless_probe.mjs --playwright /path/to/playwright-core --out-dir /private/path
// stdin JSONL: {kind:'state'}, {kind:'fillName',text:'...'},
// {kind:'clickText',text:'生誕する'}, {kind:'clickPoint',x:100,y:200},
// {kind:'clickImage',path:'/images/example.png',handRect:{x:0,y:600,w:1440,h:400}},
// {kind:'waitText',text:'...'}, {kind:'screenshot'}, {kind:'capture'}, {kind:'close'}.
// A capture is an immutable prefix, not necessarily a complete response body.
import { createRequire } from 'node:module';
import fs from 'node:fs/promises';
import { constants } from 'node:fs';
import path from 'node:path';
import { createHash, randomBytes } from 'node:crypto';
import readline from 'node:readline';

const emit = value => process.stdout.write(JSON.stringify(value) + '\n');
const fail = code => { const e = new Error(code); e.probeCode = code; throw e; };
const args = process.argv.slice(2);
const options = {};
for (let i = 0; i < args.length; i += 2) {
  if (!['--playwright', '--out-dir'].includes(args[i]) || !args[i + 1] || options[args[i]])
    fail('invalid_cli');
  options[args[i]] = args[i + 1];
}
if (!options['--playwright'] || !options['--out-dir']) fail('missing_cli');
process.umask(0o077);
const directory = path.resolve(options['--out-dir']);
await fs.mkdir(directory, { recursive: true, mode: 0o700 });
const directoryInfo = await fs.lstat(directory);
if (!directoryInfo.isDirectory() || directoryInfo.isSymbolicLink()) fail('invalid_private_directory');
await fs.chmod(directory, 0o700);
const run = randomBytes(8).toString('hex');
let fileSequence = 0;
const privateFile = async (suffix, bytes) => {
  const file = path.join(directory, `${run}-${String(++fileSequence).padStart(6, '0')}-${suffix}`);
  const handle = await fs.open(file, constants.O_WRONLY | constants.O_CREAT | constants.O_EXCL | constants.O_NOFOLLOW, 0o600);
  try { await handle.writeFile(bytes); } finally { await handle.close(); }
  return file;
};
const packageDirectory = path.resolve(options['--playwright']);
const require = createRequire(path.join(packageDirectory, 'package.json'));
const { chromium } = require(packageDirectory);
let browser, context, page, cdp;
let closing = false, streamSequence = 0;
const requests = new Set(); // Internal CDP IDs are never serialized.
const streams = new Map();
let lastImages = [];

function append(record, bytes) {
  if (!bytes.length) return;
  record.tail = record.tail.then(async () => {
    if (record.error) return;
    await record.handle.writeFile(bytes);
    record.hash.update(bytes); record.bytes += bytes.length;
  }).catch(() => { record.error = true; });
}
async function startStream(requestId) {
  if (streams.has(requestId)) return;
  const record = { sequence: ++streamSequence, queue: [], initializing: true,
    bytes: 0, hash: createHash('sha256'), tail: Promise.resolve(), finished: false,
    failed: false, error: false, streaming: false, handle: null };
  streams.set(requestId, record);
  record.ready = (async () => {
    try {
      const file = path.join(directory, `${run}-stream-${String(record.sequence).padStart(6, '0')}.part`);
      record.file = file;
      record.handle = await fs.open(file, constants.O_RDWR | constants.O_CREAT | constants.O_EXCL | constants.O_NOFOLLOW, 0o600);
      // After streaming is enabled, data events may arrive before this reply.
      // bufferedData precedes those queued events; never append the queue first.
      const result = await cdp.send('Network.streamResourceContent', { requestId });
      append(record, Buffer.from(result.bufferedData || '', 'base64'));
      for (const bytes of record.queue) append(record, bytes);
      record.queue.length = 0;
      record.initializing = false; record.streaming = true;
    } catch {
      record.initializing = false; record.error = true; record.queue.length = 0;
    }
  })();
}
async function capture() {
  const records = [...streams.values()];
  await Promise.all(records.map(record => record.ready));
  // Enqueue each snapshot in its stream's write chain. Later network chunks
  // append after this task, so prefix/hash/completion refer to one boundary.
  const captures = await Promise.all(records.map(record => {
    // Sample completion before inserting the barrier: a later loadingFinished
    // may follow chunks that have been queued *after* this prefix snapshot.
    const finishedAtBoundary = record.finished, failedAtBoundary = record.failed;
    const snapshot = record.tail.then(async () => {
      const bytes = record.bytes;
      const completed = record.streaming && finishedAtBoundary && !failedAtBoundary && !record.error;
      const hash = record.hash.copy().digest('hex');
      const data = Buffer.alloc(bytes);
      if (bytes) {
        let offset = 0;
        while (offset < bytes) {
          const read = await record.handle.read(data, offset, bytes - offset, offset);
          if (!read.bytesRead) fail('prefix_read_failed');
          offset += read.bytesRead;
        }
      }
      const file = await privateFile(`stream-${String(record.sequence).padStart(6, '0')}-prefix.bin`, data);
      return { file, bytes, sha256: hash, completed, capture_error: record.error };
    });
    record.tail = snapshot.then(() => {}, () => { record.error = true; });
    return snapshot;
  }));
  const metadata = { schema: 'official-headless-prefix-capture-v1', captures };
  const file = await privateFile('capture.json', JSON.stringify(metadata, null, 2) + '\n');
  return { metadata: file, captures };
}
const rect = value => {
  if (!value || typeof value !== 'object') fail('invalid_rect');
  const r = { x: value.x, y: value.y, w: value.w ?? value.width, h: value.h ?? value.height };
  if (!Object.values(r).every(Number.isFinite) || r.w <= 0 || r.h <= 0) fail('invalid_rect');
  return r;
};
const text = value => { if (typeof value !== 'string' || !value.length) fail('invalid_text'); return value; };
async function state() {
  const inputs = await page.locator('input:visible').evaluateAll(elements => elements
    .filter(e => e.type !== 'password' && e.type !== 'hidden')
    .map(e => ({ type: e.type, placeholder: e.placeholder, value: e.value })));
  const images = await page.locator('img:visible').evaluateAll(elements => elements.map(e => {
    const r = e.getBoundingClientRect();
    return { path: new URL(e.src, document.baseURI).pathname, x: r.x, y: r.y, w: r.width, h: r.height };
  }).filter(r => r.w > 0 && r.h > 0 && r.x < innerWidth && r.y < innerHeight && r.x + r.w > 0 && r.y + r.h > 0));
  lastImages = images;
  // Visible text geometry only; no application state or event handlers.
  const texts = await page.locator('body').evaluate(body => {
    const walker = document.createTreeWalker(body, NodeFilter.SHOW_TEXT);
    const result = [];
    while (walker.nextNode()) {
      const node = walker.currentNode, text = node.textContent.trim();
      if (!text || !node.parentElement.checkVisibility()) continue;
      const range = document.createRange(); range.selectNodeContents(node);
      const r = range.getBoundingClientRect();
      if (r.width > 0 && r.height > 0 && r.x >= 0 && r.y >= 0 && r.bottom <= innerHeight)
        result.push({ text, x: r.x, y: r.y, w: r.width, h: r.height });
    }
    return result;
  });
  return { body: await page.locator('body').innerText(), inputs, images, texts };
}
async function close() {
  if (closing) return;
  closing = true;
  if (browser) await browser.close().catch(() => {});
  await Promise.all([...streams.values()].map(async record => {
    await record.ready; await record.tail;
    if (record.handle) await record.handle.close().catch(() => {});
  }));
}
try {
  browser = await chromium.launch({ headless: true });
  context = await browser.newContext({ viewport: { width: 1440, height: 1000 }, locale: 'ja-JP' });
  page = await context.newPage(); page.setDefaultTimeout(15000);
  cdp = await context.newCDPSession(page);
  cdp.on('Network.requestWillBeSent', event => {
    // Only incoming Firestore channel response content is retained.
    try {
      const url = new URL(event.request.url);
      if (event.request.method === 'GET' && url.protocol === 'https:' &&
          url.hostname === 'firestore.googleapis.com' && /\/channel$/.test(url.pathname))
        requests.add(event.requestId);
      else requests.delete(event.requestId);
    } catch { /* No request details leave this callback. */ }
  });
  cdp.on('Network.responseReceived', event => {
    if (requests.has(event.requestId)) void startStream(event.requestId);
  });
  cdp.on('Network.dataReceived', event => {
    const record = streams.get(event.requestId);
    if (!record || !event.data || record.error) return;
    const bytes = Buffer.from(event.data, 'base64');
    if (record.initializing) record.queue.push(bytes); else append(record, bytes);
  });
  cdp.on('Network.loadingFinished', event => {
    const record = streams.get(event.requestId); if (record) record.finished = true;
    requests.delete(event.requestId);
  });
  cdp.on('Network.loadingFailed', event => {
    const record = streams.get(event.requestId); if (record) record.failed = true;
    requests.delete(event.requestId);
  });
  await cdp.send('Network.enable');
  await page.goto('https://godfield.net/', { waitUntil: 'domcontentloaded', timeout: 30000 });
  emit({ ready: true, state: await state() });
  const lines = readline.createInterface({ input: process.stdin, crlfDelay: Infinity });
  for await (const line of lines) {
    if (!line.trim()) continue;
    try {
      const action = JSON.parse(line);
      if (!action || typeof action !== 'object') fail('invalid_action');
      if (action.kind === 'state') emit({ state: await state() });
      else if (action.kind === 'fillName') {
        const input = page.locator('input:visible').filter({ visible: true });
        if (await input.count() !== 1) fail('name_input_not_unique');
        await input.fill(text(action.text)); emit({ state: await state() });
      } else if (action.kind === 'clickText') {
        const target = page.getByText(text(action.text), { exact: true }).filter({ visible: true });
        if (await target.count() !== 1) fail('text_not_unique');
        // The visible text node can be covered by its pointer-handling parent.
        await target.click({ force: true }); emit({ state: await state() });
      } else if (action.kind === 'clickImage') {
        const imagePath = text(action.path), region = rect(action.handRect ?? action.rect);
        if (!imagePath.startsWith('/') || imagePath.includes('?') || imagePath.includes('#')) fail('invalid_image_path');
        const observed = lastImages.filter(r => r.path === imagePath && r.x + r.w / 2 >= region.x &&
          r.x + r.w / 2 < region.x + region.w && r.y + r.h / 2 >= region.y && r.y + r.h / 2 < region.y + region.h);
        if (observed.length !== 1) fail('observed_image_not_unique');
        const current = await page.locator('img:visible').evaluateAll(elements => elements.map((e, index) => {
          const r = e.getBoundingClientRect(); return { index, path: new URL(e.src, document.baseURI).pathname,
            x: r.x, y: r.y, w: r.width, h: r.height };
        }));
        const old = observed[0];
        const candidates = current.filter(r => r.path === old.path && ['x', 'y', 'w', 'h'].every(k => Math.abs(r[k] - old[k]) < 1));
        if (candidates.length !== 1) fail('image_changed_request_state');
        await page.locator('img:visible').nth(candidates[0].index).click({ force: true });
        emit({ state: await state() });
      } else if (action.kind === 'clickPoint') {
        if (!Number.isFinite(action.x) || !Number.isFinite(action.y) || action.x < 0 || action.x >= 1440 || action.y < 0 || action.y >= 1000)
          fail('invalid_point');
        await page.mouse.click(action.x, action.y); emit({ state: await state() });
      } else if (action.kind === 'waitText') {
        await page.getByText(text(action.text), { exact: true }).first().waitFor({ state: 'visible', timeout: 20000 });
        emit({ state: await state() });
      } else if (action.kind === 'screenshot') {
        const file = await privateFile('screenshot.png', await page.screenshot()); emit({ screenshot: file });
      } else if (action.kind === 'capture') emit({ capture: await capture() });
      else if (action.kind === 'close') { emit({ capture: await capture() }); await close(); emit({ closed: true }); lines.close(); break; }
      else fail('unknown_action');
    } catch (error) {
      // Playwright/CDP messages can contain URLs or other transport details.
      emit({ error: error.probeCode || 'action_failed' });
    }
  }
} catch (error) {
  emit({ error: error.probeCode || 'startup_failed' }); process.exitCode = 1;
} finally { await close(); }
