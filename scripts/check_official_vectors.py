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
