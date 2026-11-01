"""Cache only public official assets outside the repo; never join/authenticate a game."""
from pathlib import Path
import argparse, hashlib, json, urllib.request
p=Path(__file__).resolve().parent
parser=argparse.ArgumentParser();parser.add_argument('--cache',type=Path,default=Path('/tmp/godfield-official-v2-cache'));parser.add_argument('--require-match',action='store_true');args=parser.parse_args()
m=json.loads((p/'manifest.json').read_text());args.cache.mkdir(parents=True,exist_ok=True)
outputs=[('ja.json',m['source_url'],m['source_raw_sha256']),('main.dart.js',m['client_url'],m['client_sha256'])]
matched=True
for name,url,expected in outputs:
 request=urllib.request.Request(url,headers={'User-Agent':'GodfieldCatalogFacts/2.0','Accept':'application/json'if name.endswith('.json')else'application/javascript'})
 with urllib.request.urlopen(request,timeout=30)as response:
  raw=response.read(5_000_001)
  if len(raw)>5_000_000:raise ValueError('unexpected asset size')
  headers={k.lower():v for k,v in response.headers.items()}
 digest=hashlib.sha256(raw).hexdigest();matched &= digest==expected
 if name=='ja.json' and digest==expected:
  source_rows=json.loads(raw)['items']
  normalized=[dict(id=index+1,**row) for index,row in enumerate(source_rows)]
  if normalized!=json.loads((p/'cards.json').read_text()):
   raise ValueError('Archived facts differ from the pinned official source items')
  print('Archived296 metadata rows equal the pinned official JSON items.')
 (args.cache/name).write_bytes(raw)
 (args.cache/(name+'.headers.json')).write_text(json.dumps({'url':url,'headers':headers,'bytes':len(raw),'sha256':digest},indent=2)+'\n')
 print(f'{name}: sha256={digest} bytes={len(raw)} matches_snapshot={digest==expected}')
if args.require_match and not matched:raise SystemExit('Official content changed; review a new version rather than overwriting facts silently.')
