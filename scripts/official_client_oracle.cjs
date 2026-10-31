#!/usr/bin/env node
// Regenerate factual fixtures by running pinned, public client helpers offline.
// The client bundle itself is a local input and is not redistributed here.
'use strict';
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const crypto = require('node:crypto');
const root = path.resolve(__dirname, '..');
const directory = path.join(root, 'data/official-v1');
const manifest = JSON.parse(fs.readFileSync(path.join(directory, 'manifest.json'), 'utf8'));
const args = process.argv.slice(2);
if (args.length !== 2 || args[0] !== '--client') {
  process.stderr.write('Usage: node scripts/official_client_oracle.cjs --client /path/to/main.dart.js\n');
  process.exit(1);
}
const digest = data => crypto.createHash('sha256').update(data).digest('hex');
const bytes = fs.readFileSync(args[1]);
if (digest(bytes) !== manifest.client_sha256) throw new Error('Pinned official client SHA256 mismatch');
const cardsBytes = fs.readFileSync(path.join(directory, 'cards.json'));
if (digest(cardsBytes) !== manifest.cards_sha256) throw new Error('Pinned official catalog SHA256 mismatch');
let source = bytes.toString('utf8');
const marker = 'var s=A.wG';
if (source.split(marker).length !== 2) throw new Error('Client export marker changed');
source = source.replace(marker, 'globalThis.oracle={A,B,J,t,$};var s=A.wG');
// dartMainRunner prevents the application entry point from running. No DOM,
// Firebase, fetch, timers, user credentials, or room connection are provided.
const box = { dartMainRunner() {} };
vm.createContext(box);
vm.runInContext(source, box, { timeout: 5000 });
box.cardsText = cardsBytes.toString('utf8');
vm.runInContext('oracle.A.ty(oracle.A.nF(JSON.parse(cardsText)))', box, { timeout: 5000 });
const { A, B, $, t } = box.oracle;
if ($.p.length !== 296 || $.og !== 500) throw new Error('Official loader result changed');
const categories = [B.P, B.j, B.u, B.E, B.k, B.I, B.N, B.O];
const elements = [null, B.x, B.z, B.A, B.y, B.l, B.r];
const attr = e => elements.indexOf(e) - 1; // -1 absent, 0..5 named elements.
const reactions = { bounce: 1, reflect: 2, block: 3 };
const record = id => $.p[id - 1];
const typed = ids => A.u(ids.map(record), t.G);
const visible = (id, used = false) => {
  const value = Object.create(A.aC.prototype);
  value.e = record(id); value.f = null; value.w = used ? {} : null;
  return value;
};
const list = selected => A.u(selected.map(([id, used]) => visible(id, used)), t.k);
$.t = () => ({ z: 'offline-viewer' });
const selection = (selected, next) => A.jM.prototype.ev.call({ r: false }, list(selected), visible(...next));
const response = (selected, next, category, element, attack, flash) => {
  const incoming = Object.create(A.aN.prototype);
  Object.assign(incoming, record(5), { d: categories[category], e: elements[element + 1], y: '' });
  const context = {
    r: true,
    a: {
      x: { J: () => ({ ay: { ak: curse => flash && curse === B.F } }) },
      f: { gaW: () => ({ d: A.u([incoming], t.G), gf3: () => attack }) }
    }
  };
  return A.jM.prototype.ev.call(context, list(selected), visible(...next));
};
let state = 0x6d2b79f5;
const random = n => { state ^= state << 13; state ^= state >>> 17; state ^= state << 5; return (state >>> 0) % n; };
const nextCard = () => random(296) + 1;
const lines = ['# official-client-vectors-v1: C composition; R reaction; M main append; D response append'];
const counts = { C: 0, R: 0, M: 0, D: 0 };
function composition(ids, override) {
  const cards = typed(ids);
  lines.push(['C', override, ids.join(','), A.tw(cards, override), attr(A.l9(cards)), attr(A.tx(cards)), A.la(cards)].join('\t'));
  ++counts.C;
}
for (let id = 1; id <= 296; ++id) composition([id], 0);
for (let i = 0; i < 4096; ++i) {
  const ids = Array.from({ length: random(12) + 1 }, nextCard);
  composition(ids, random(4) === 0 ? random(199) : 0);
}
// Target the rare ordered effects as well as random compositions.
const specials = $.p.filter(c => ['doubleAtk', 'cutCost', 'setElement', 'filterAtkElement'].includes(c.y)).map(c => c.a);
for (const first of [5, 58, 66, 86, 210, 215]) for (const a of specials) for (const b of specials) composition([first, a, b], 0);
for (let id = 1; id <= 296; ++id) for (let category = 0; category < 8; ++category) for (let element = -1; element < 6; ++element) {
  const c = record(id);
  lines.push(['R', id, category, element, reactions[c.e1(categories[category], elements[element + 1])] || 0, +c.kO(elements[element + 1])].join('\t'));
  ++counts.R;
}
for (let i = 0; i < 8192; ++i) {
  const selected = Array.from({ length: random(5) }, () => [nextCard(), !!random(2)]);
  const next = [nextCard(), !!random(2)];
  const encode = s => s.length ? s.map(([id, used]) => `${id}:${+used}`).join(',') : '-';
  lines.push(['M', encode(selected), `${next[0]}:${+next[1]}`, +selection(selected, next)].join('\t'));
  ++counts.M;
  const category = random(8), element = random(7) - 1, attack = !!random(2), flash = !!random(2);
  lines.push(['D', +flash, category, element, +attack, encode(selected), `${next[0]}:${+next[1]}`, +response(selected, next, category, element, attack, flash)].join('\t'));
  ++counts.D;
}
// All model types, trade/used-miracle combinations, flash/FREE, and filter/reaction prefixes.
for (let id = 1; id <= 296; ++id) for (const prefix of [[], [[1, false]], [[2, false]], [[4, false]], [[189, false]], [[188, false]], [[188, false], [188, false]]]) {
  const encode = s => s.length ? s.map(([id, used]) => `${id}:${+used}`).join(',') : '-';
  for (const used of [false, true]) {
    lines.push(['M', encode(prefix), `${id}:${+used}`, +selection(prefix, [id, used])].join('\t'));
    ++counts.M;
    for (const flash of [false, true]) {
      lines.push(['D', +flash, 1, -1, 1, encode(prefix), `${id}:${+used}`, +response(prefix, [id, used], 1, -1, true, flash)].join('\t'));
      ++counts.D;
    }
  }
}
const output = lines.join('\n') + '\n';
fs.writeFileSync(path.join(directory, 'client-vectors.tsv'), output);
const metadata = {
  schema_version: 1, source: manifest.client_url, client_sha256: manifest.client_sha256,
  cards_sha256: manifest.cards_sha256, vectors_sha256: digest(output), counts,
  purpose: 'Actual pinned public client composition, reaction and visible append predicates; not server transition fixtures.',
  network_or_application_startup: false,
  generator: 'node scripts/official_client_oracle.cjs --client /path/to/main.dart.js'
};
fs.writeFileSync(path.join(directory, 'client-vectors.manifest.json'), JSON.stringify(metadata, null, 2) + '\n');
process.stdout.write(JSON.stringify(metadata) + '\n');
