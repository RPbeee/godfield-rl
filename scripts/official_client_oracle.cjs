#!/usr/bin/env node
// Regenerate factual fixtures by running pinned, public client helpers offline.
// The client bundle itself is a local input and is not redistributed here.
'use strict';
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const crypto = require('node:crypto');
const root = path.resolve(__dirname, '..');
const digest = data => crypto.createHash('sha256').update(data).digest('hex');
// Bindings are selected only by the complete, pinned bundle and catalog pair.
const pinned = {
  '764a50524e4b6b3f510415da7128abd8ad99dcb87b45b8572d98ddd96889cabd': {
    version: 'official-v1', cards: '9ce45da9dd372da1ab30397068885a20bbd5927b0891946b5fc7bc07226fa1a8',
    marker: 'var s=A.wG', loader: 'ty', decodeJSON: 'nF', weight: 'og',
    attack: 'tw', attackElement: 'l9', defenseElement: 'tx', cost: 'la',
    selection: 'jM', append: 'ev', reaction: 'e1', attributeDefense: 'kO', attackFlag: 'gf3',
    roster: 'h7', enemies: 'dR', ending: 'lV', tooltip: 'l_', decoder: 'tL', wireSet: 'pz',
    curseWidget: 'ur', iterator: 'mU', addCurse: 'dA', die: 'hP', removeVisual: 'fU', deathVisual: 'hB',
    inventory: 'fG', sort: 'hs', resources: ['eW', 'dH', 'bc']
  },
  '5ece06c224b6a7a5d877eb45d22ea18b82a03aaf1b7161cbbdf6a1e23ceab2cd': {
    version: 'official-v2', cards: '8e465e2d296a2dd039d6e3b6e22956447c1fc4a5572a31e2597cdf20c551dc2a',
    marker: 'var s=A.wJ', loader: 'tB', decodeJSON: 'nI', weight: 'oj',
    attack: 'tz', attackElement: 'lc', defenseElement: 'tA', cost: 'ld',
    selection: 'jO', append: 'eu', reaction: 'e0', attributeDefense: 'kN', attackFlag: 'gf2',
    roster: 'h8', enemies: 'dQ', ending: 'lU', tooltip: 'l2', decoder: 'tO', wireSet: 'pC',
    curseWidget: 'uu', iterator: 'mX', addCurse: 'dz', die: 'hO', removeVisual: 'fT', deathVisual: 'hA',
    inventory: 'fH', sort: 'hr', resources: ['eV', 'dG', 'bc']
  }
};
function loadPinnedClient(args, script) {
  const options = {};
  for (let i = 0; i < args.length; i += 2) {
    const key = args[i];
    if (!['--client', '--catalog-dir', '--out-dir'].includes(key) || options[key] !== undefined ||
        i + 1 >= args.length || args[i + 1].startsWith('--')) {
      throw new Error(`Usage: node ${script} --client PATH [--catalog-dir PATH] [--out-dir PATH]`);
    }
    options[key] = args[i + 1];
  }
  if (!options['--client']) throw new Error('--client is required');
  const directory = path.resolve(options['--catalog-dir'] || path.join(root, 'data/official-v2'));
  const outputDirectory = path.resolve(options['--out-dir'] || directory);
  const archive = fs.realpathSync(path.join(root, 'data/official-v1'));
  // Resolve existing ancestors as well: a symlink must not bypass archive protection.
  let ancestor = outputDirectory;
  while (!fs.existsSync(ancestor)) ancestor = path.dirname(ancestor);
  const realOutput = path.resolve(fs.realpathSync(ancestor), path.relative(ancestor, outputDirectory));
  if (realOutput === archive || realOutput.startsWith(archive + path.sep))
    throw new Error('The official-v1 archive is immutable; use --out-dir /tmp/...');
  const manifest = JSON.parse(fs.readFileSync(path.join(directory, 'manifest.json'), 'utf8'));
  const bytes = fs.readFileSync(options['--client']);
  const binding = pinned[digest(bytes)];
  if (!binding || manifest.client_url !== 'https://godfield.net/main.dart.js' ||
      manifest.catalog_version !== binding.version || manifest.client_sha256 !== digest(bytes) ||
      manifest.cards_sha256 !== binding.cards) throw new Error('Unknown or mismatched pinned client/catalog pair');
  const cardsBytes = fs.readFileSync(path.join(directory, 'cards.json'));
  if (digest(cardsBytes) !== binding.cards) throw new Error('Pinned official catalog SHA256 mismatch');
  const originalSource = bytes.toString('utf8');
  if (originalSource.split(binding.marker).length !== 2) throw new Error('Pinned client export marker changed');
  const source = originalSource.replace(binding.marker, `globalThis.oracle={A,B,J,t,$};${binding.marker}`);
  // No DOM, Firebase, fetch, timers, user credentials or room connection exist.
  // The sole entrypoint hook suppresses Dart application startup.
  const box = { dartMainRunner() {} };
  vm.createContext(box);
  vm.runInContext(source, box, { timeout: 5000 });
  box.cardsText = cardsBytes.toString('utf8');
  vm.runInContext(`oracle.A.${binding.loader}(oracle.A.${binding.decodeJSON}(JSON.parse(cardsText)))`, box, { timeout: 5000 });
  const { A, B, J, $, t } = box.oracle;
  if ($.p.length !== 296 || $[binding.weight] !== 500) throw new Error('Official loader result changed');
  return { A, B, J, $, t, box, binding, directory, outputDirectory, manifest, originalSource };
}
function writeOracleOutputs(directory, kind, output, metadata) {
  fs.mkdirSync(directory, { recursive: true });
  const files = [`${kind}-vectors.tsv`, `${kind}-vectors.manifest.json`];
  // Check both destinations before touching either. lstat also catches dangling
  // symlinks; shared inodes must not provide a path into an immutable archive.
  for (const name of files) {
    let stat;
    try { stat = fs.lstatSync(path.join(directory, name)); }
    catch (error) { if (error.code === 'ENOENT') continue; throw error; }
    if (stat.isSymbolicLink() || !stat.isFile() || stat.nlink !== 1)
      throw new Error(`Oracle output must be an unshared regular file: ${name}`);
  }
  const temporary = fs.mkdtempSync(path.join(directory, `.${kind}-oracle-`));
  try {
    const contents = [output, JSON.stringify(metadata, null, 2) + '\n'];
    for (let i = 0; i < files.length; ++i)
      fs.writeFileSync(path.join(temporary, files[i]), contents[i], { flag: 'wx', mode: 0o644 });
    // Rename replaces the destination entry rather than following a file link.
    for (const name of files) fs.renameSync(path.join(temporary, name), path.join(directory, name));
  } finally { fs.rmSync(temporary, { recursive: true, force: true }); }
}
module.exports = { loadPinnedClient, writeOracleOutputs, digest };
function generate() {
const { A, B, $, t, binding, outputDirectory, manifest } = loadPinnedClient(process.argv.slice(2), 'scripts/official_client_oracle.cjs');
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
const selection = (selected, next) => A[binding.selection].prototype[binding.append].call({ r: false }, list(selected), visible(...next));
const response = (selected, next, category, element, attack, flash) => {
  const incoming = Object.create(A.aN.prototype);
  Object.assign(incoming, record(5), { d: categories[category], e: elements[element + 1], y: '' });
  const context = {
    r: true,
    a: {
      x: { J: () => ({ ay: { ak: curse => flash && curse === B.F } }) },
      f: { gaW: () => ({ d: A.u([incoming], t.G), [binding.attackFlag]: () => attack }) }
    }
  };
  return A[binding.selection].prototype[binding.append].call(context, list(selected), visible(...next));
};
let state = 0x6d2b79f5;
const random = n => { state ^= state << 13; state ^= state >>> 17; state ^= state << 5; return (state >>> 0) % n; };
const nextCard = () => random(296) + 1;
const lines = ['# official-client-vectors-v1: C composition; R reaction; M main append; D response append'];
const counts = { C: 0, R: 0, M: 0, D: 0 };
function composition(ids, override) {
  const cards = typed(ids);
  lines.push(['C', override, ids.join(','), A[binding.attack](cards, override), attr(A[binding.attackElement](cards)), attr(A[binding.defenseElement](cards)), A[binding.cost](cards)].join('\t'));
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
  lines.push(['R', id, category, element, reactions[c[binding.reaction](categories[category], elements[element + 1])] || 0, +c[binding.attributeDefense](elements[element + 1])].join('\t'));
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

const metadata = {
  schema_version: 1, source: manifest.client_url, client_sha256: manifest.client_sha256,
  cards_sha256: manifest.cards_sha256, vectors_sha256: digest(output), counts,
  purpose: 'Actual pinned public client composition, reaction and visible append predicates; not server transition fixtures.',
  network_or_application_startup: false,
  generator: 'node scripts/official_client_oracle.cjs --client /path/to/main.dart.js' + (binding.version === 'official-v1' ? ' --catalog-dir data/official-v1 --out-dir /tmp/official-v1-replay' : '')
};
writeOracleOutputs(outputDirectory, 'client', output, metadata);
process.stdout.write(JSON.stringify(metadata) + '\n');
}
if (require.main === module) generate();
