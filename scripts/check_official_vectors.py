"""Offline integrity of the official-client-derived conformance fixture."""
import collections
import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parent.parent
folder = root / 'data' / 'official-v1'
manifest = json.loads((folder / 'manifest.json').read_text())
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
assert resource_manifest['methods'] == ['c5.eW', 'c5.dH', 'c5.bc']
print('7898 actual-client resource updates: fingerprints verified offline; no scheduling claim.')
