#!/usr/bin/env node
// Offline current-client UI affordance equivalence for an experimental Dream pool.
// This script never writes files; stdout is the complete reproducible report.
'use strict';
const fs = require('node:fs');
const path = require('node:path');
const { loadPinnedClient } = require('./official_client_oracle.cjs');
const args = process.argv.slice(2);
let client = null, check = false;
for (let i = 0; i < args.length; ++i) {
  if (args[i] === '--check' && !check) check = true;
  else if (args[i] === '--client' && client === null && i + 1 < args.length && !args[i + 1].startsWith('--')) client = args[++i];
  else throw new Error('Usage: node scripts/official_dream_v32_oracle.cjs --client PATH [--check]');
}
if (client === null) throw new Error('--client is required');
const directory = path.resolve(__dirname, '../data/official-v2');
const { A, B, t, $, binding, manifest } = loadPinnedClient(['--client', client, '--catalog-dir', directory], 'scripts/official_dream_v32_oracle.cjs');
if (binding.version !== 'official-v2') throw new Error('Dream audit requires the pinned official-v2 client/catalog pair');
const facts = JSON.parse(fs.readFileSync(path.join(directory, 'cards.json'), 'utf8'));
// Local v32 executable allowlist. Runtime tests separately compare these IDs to Match.
const supported = [3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239];
if (supported.length !== 236 || new Set(supported).size !== 236) throw new Error('Unexpected Dream supported IDs');
const keys = ['category', 'element', 'ability', 'curse', 'cost', 'hitRate', 'isPlusAtk', 'atk>0', 'def>0'];
const signature = c => [c.category, c.element ?? null, c.ability ?? 'none', c.curse ?? 'none', c.cost ?? 0,
  c.hitRate ?? 0, c.isPlusAtk != null, (c.atk ?? 0) > 0, (c.def ?? 0) > 0];
const groups = new Map();
let giftWeight = 0;
for (const id of supported) {
  const c = facts[id - 1];
  if (c.id !== id || !(c.giftRate > 0)) throw new Error('Unsupported/ungifted audit model');
  giftWeight += c.giftRate;
  const key = JSON.stringify(signature(c));
  if (!groups.has(key)) groups.set(key, []);
  groups.get(key).push(id);
}
const multi = [...groups.values()].filter(g => g.length > 1);
const singleton = [...groups.values()].filter(g => g.length === 1).flat();
const categories = [B.P, B.j, B.u, B.E, B.k, B.I, B.N, B.O];
const elements = [null, B.x, B.z, B.A, B.y, B.l, B.r];
const card = id => $.p[id - 1];
const typed = ids => A.u(ids.map(card), t.G);
const visible = id => Object.assign(Object.create(A.aC.prototype), { e: card(id), f: null, w: null });
const list = ids => A.u(ids.map(visible), t.k);
$.t = () => ({ z: 'offline-viewer' });
const main = (prefix, id) => A[binding.selection].prototype[binding.append].call({ r: false }, list(prefix), visible(id));
const response = (prefix, id, category, element, attack, flash) => {
  const incoming = Object.assign(Object.create(A.aN.prototype), card(5), { d: category, e: element, y: '' });
  return A[binding.selection].prototype[binding.append].call({ r: true, a: {
    x: { J: () => ({ ay: { ak: curse => flash && curse === B.F } }) },
    f: { gaW: () => ({ d: A.u([incoming], t.G), [binding.attackFlag]: () => attack }) }
  } }, list(prefix), visible(id));
};
const same = (a, b, context) => { if (a !== b) throw new Error('Dream UI affordance mismatch: ' + context); };
const comparisons = { ordered_pairs: 0, start_and_default_target: 0, magic_cost: 0, main_append: 0, response_append: 0 };
for (const group of groups.values()) for (const a of group) for (const b of group) {
  if (a === b) continue;
  ++comparisons.ordered_pairs;
  same(card(a).gkM(), card(b).gkM(), `start ${a}/${b}`);
  same(card(a).glj(), card(b).glj(), `default target ${a}/${b}`);
  comparisons.start_and_default_target += 2;
  for (const prefix of [[], [6], [233], [189], [4]]) for (const suffix of [[], [204], [231, 204]]) {
    same(A[binding.cost](typed([...prefix, a, ...suffix])), A[binding.cost](typed([...prefix, b, ...suffix])), `cost ${a}/${b}`);
    ++comparisons.magic_cost;
  }
  for (const id of supported) {
    same(main([id], a), main([id], b), `main next ${a}/${b}`);
    same(main([a], id), main([b], id), `main first ${a}/${b}`);
    same(main([6, a], id), main([6, b], id), `main last ${a}/${b}`);
    comparisons.main_append += 3;
  }
  for (const category of categories) for (const element of elements) for (const attack of [false, true])
    for (const flash of [false, true]) for (const prefix of [[], [189], [190], [233], [118]]) {
      same(response(prefix, a, category, element, attack, flash), response(prefix, b, category, element, attack, flash), `response next ${a}/${b}`);
      ++comparisons.response_append;
    }
}
const report = {
  schema_version: 1,
  scope: 'experimental:compatible-dream-ui-classes-v32;not-official-fake-distribution;not-payload-equivalence',
  source: manifest.client_url,
  client_sha256: manifest.client_sha256,
  catalog_version: manifest.catalog_version,
  cards_sha256: manifest.cards_sha256,
  supported_ids: supported,
  pool_models: supported.length,
  pool_gift_weight: giftWeight,
  signature: keys,
  class_count: groups.size,
  eligible_class_count: multi.length,
  eligible_model_count: multi.reduce((n, g) => n + g.length, 0),
  eligible_gift_weight: multi.flat().reduce((n, id) => n + facts[id - 1].giftRate, 0),
  singleton_model_count: singleton.length,
  singleton_gift_weight: singleton.reduce((n, id) => n + facts[id - 1].giftRate, 0),
  comparisons,
  classes: [...groups].map(([key, ids]) => ({ signature: Object.fromEntries(keys.map((k, i) => [k, JSON.parse(key)[i]])), model_ids: ids })),
  predicate_contexts: {
    selected_items_used: false,
    cost_prefixes: [[], [6], [233], [189], [4]], cost_suffixes: [[], [204], [231, 204]],
    main: 'Every supported next model with substitution at next, first, and last position.',
    response_categories: ['trade', 'weapons', 'armor', 'sundries', 'miracles', 'devils', 'guardians', 'phenomena'],
    response_elements: [null, 'fire', 'water', 'wood', 'stone', 'light', 'darkness'],
    response_is_attack: [false, true], flash: [false, true], response_prefixes: [[], [189], [190], [233], [118]]
  },
  limitations: [
    'These classes preserve the tested public client selection, affordability and target affordances, not payload ATK/DEF/price/recovery values.',
    'The local supported pool, 50percent gift-only masking and uniform alternate-face choice are experimental; this report does not establish official server fake generation or mismatch handling.',
    'All eligible models in this pool are nonmiracle physical items. Singleton classes, including all miracles/trade, the five Dream sources and both summoning models, have no alternate face.',
    'The client is executed offline without Dart application startup, DOM, Firebase, timers, account or game connection. This is not server transition evidence.',
    'A future catalog, allowlist, ability or predicate change requires regeneration and renewed closure review. Model178 is included as a singleton; its redraw execution is a separate experimental server contract.'
  ],
  network_or_application_startup: false,
  generator: 'node scripts/official_dream_v32_oracle.cjs --client /path/to/main.dart.js [--check]'
};
if (giftWeight !== 499 || groups.size !== 120 || multi.length !== 30 || report.eligible_model_count !== 146 || singleton.length !== 90 || report.eligible_gift_weight !== 321 || report.singleton_gift_weight !== 178
    || comparisons.ordered_pairs !== 992 || comparisons.start_and_default_target + comparisons.magic_cost
      + comparisons.main_append + comparisons.response_append !== 1830240)
  throw new Error('Dream class fingerprint changed');
const output = JSON.stringify(report, null, 2) + '\n';
if (check && fs.readFileSync(path.join(directory, 'dream-compatibility-v32.json'), 'utf8') !== output)
  throw new Error('Dream compatibility report differs; review and regenerate explicitly');
process.stdout.write(output);
