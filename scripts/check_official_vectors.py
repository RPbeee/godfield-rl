"""Offline integrity of the official-client-derived conformance fixture."""
import argparse
import collections
import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser()
parser.add_argument('--catalog-dir', type=Path, default=root / 'data' / 'official-v2')
args = parser.parse_args()
folder = args.catalog_dir.resolve()
manifest = json.loads((folder / 'manifest.json').read_text())
profiles = {
    'official-v1': ('764a50524e4b6b3f510415da7128abd8ad99dcb87b45b8572d98ddd96889cabd',
                    '9ce45da9dd372da1ab30397068885a20bbd5927b0891946b5fc7bc07226fa1a8',
                    ['c5.eW', 'c5.dH', 'c5.bc'],
                    ['c5.ce', 'h7.aG/dR/lV', 'aC.gL', 'l_.$0 (tooltip widget builder stubbed)', 'tL/pz + c5.dA/hP (visuals stubbed)', 'fG.hs(false)']),
    'official-v2': ('5ece06c224b6a7a5d877eb45d22ea18b82a03aaf1b7161cbbdf6a1e23ceab2cd',
                    '8e465e2d296a2dd039d6e3b6e22956447c1fc4a5572a31e2597cdf20c551dc2a',
                    ['c5.eV', 'c5.dG', 'c5.bc'],
                    ['c5.ce', 'h8.aG/dQ/lU', 'aC.gL', 'l2.$0 (tooltip widget builder stubbed)', 'tO/pC + c5.dz/hO (visuals stubbed)', 'fH.hr(false)']),
}
profile = profiles[manifest['catalog_version']]
assert (manifest['client_sha256'], manifest['cards_sha256']) == profile[:2]

vectors = json.loads((folder / 'client-vectors.manifest.json').read_text())
raw = (folder / 'client-vectors.tsv').read_bytes()
assert vectors['schema_version'] == 1
assert vectors['client_sha256'] == manifest['client_sha256']
assert vectors['cards_sha256'] == manifest['cards_sha256']
assert vectors['vectors_sha256'] == hashlib.sha256(raw).hexdigest()
assert vectors['network_or_application_startup'] is False
counts = collections.Counter(line.split(b'\t', 1)[0].decode() for line in raw.splitlines() if line and not line.startswith(b'#'))
assert dict(counts) == vectors['counts'] == {'C': 4878, 'R': 16576, 'M': 12336, 'D': 16480}
print('50270 official-client vectors: source/catalog/fixture fingerprints verified offline.')

state_manifest = json.loads((folder / 'state-vectors.manifest.json').read_text())
state_raw = (folder / 'state-vectors.tsv').read_bytes()
assert state_manifest['schema_version'] == 1
assert state_manifest['client_sha256'] == manifest['client_sha256']
assert state_manifest['cards_sha256'] == manifest['cards_sha256']
assert state_manifest['vectors_sha256'] == hashlib.sha256(state_raw).hexdigest()
assert state_manifest['network_or_application_startup'] is False
state_counts = collections.Counter(line.split(b'\t', 1)[0].decode() for line in state_raw.splitlines() if line and not line.startswith(b'#'))
assert dict(state_counts) == state_manifest['counts'] == {'P': 7163, 'F': 5184, 'G': 7104, 'C': 2080, 'H': 609}
assert state_manifest['methods'] == profile[3]
assert state_manifest['extracted_predicates'] == {'fog_outer': 'l.ay.ak(B.q)&&!l.x&&!p', 'fog_enemy': 'i===B.p&&m!==l||i!==j'}
print('22140 fresh-state client/helper/branch vectors: fingerprints verified offline.')

resource_manifest = json.loads((folder / 'resource-vectors.manifest.json').read_text())
resource_raw = (folder / 'resource-vectors.tsv').read_bytes()
assert resource_manifest['schema_version'] == 1
assert resource_manifest['client_sha256'] == manifest['client_sha256']
assert resource_manifest['vectors_sha256'] == hashlib.sha256(resource_raw).hexdigest()
assert resource_manifest['network_or_application_startup'] is False
resource_counts = collections.Counter(line.split(b'\t', 1)[0].decode() for line in resource_raw.splitlines() if line and not line.startswith(b'#'))
assert dict(resource_counts) == resource_manifest['counts'] == {'R': 7898}
assert resource_manifest['methods'] == profile[2]
print('7898 actual-client resource updates: fingerprints verified offline; no scheduling claim.')

if manifest['catalog_version'] == 'official-v2':
    for name in ['client-vectors.tsv', 'state-vectors.tsv', 'resource-vectors.tsv']:
        assert (folder / name).read_bytes() == (root / 'data/official-v1' / name).read_bytes(), name
    print('All80308 v2 cases reproduce the archived v1 vector bytes; catalog presentation changes are separate.')
