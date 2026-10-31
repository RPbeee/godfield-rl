"""Generate five pure status/cure miracles, or verify with --check."""
from pathlib import Path
import argparse
import csv
import hashlib
import io
import json

directory = Path(__file__).resolve().parent
project = directory.parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--check', action='store_true')
args = parser.parse_args()
manifest = json.loads((directory / 'manifest.json').read_text())
source_bytes = (directory / 'source-rows.csv').read_bytes()
catalog_bytes = (directory / 'catalog.csv').read_bytes()
assert hashlib.sha256(source_bytes).hexdigest() == manifest['source_rows_sha256']
assert hashlib.sha256(catalog_bytes).hexdigest() == manifest['catalog_sha256']
source = list(csv.DictReader(source_bytes.decode().splitlines()))
rows = list(csv.DictReader(catalog_bytes.decode().splitlines()))
expected = [(202, 'neutral', 'lower', 'none', 'none', 3, 10, 1), (208, 'dark', 'none', 'none', 'dark_cloud', 5, 10, 1), (211, 'neutral', 'all', 'none', 'none', 5, 15, 1), (212, 'neutral', 'none', 'cold', 'none', 6, 10, 1), (224, 'neutral', 'none', 'heaven', 'none', 15, 15, 1)]
ids = [r[0] for r in expected]
retained = [197, 198, 199, 200, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 226]
assert len(rows) == len(source) == manifest['catalog_count'] == 5
assert [int(r['id']) for r in rows] == manifest['selection']['ids'] == ids
assert sum(int(r['weight']) for r in rows) == manifest['total_selected_weight'] == 5
assert manifest['stable_local_slots'] == {str(id):i for i,id in enumerate(ids)}
assert manifest['retained_type_ids'] == retained == sorted(set(retained))
assert len(manifest['previous_catalog_sha256']) == 35
old_ids = set()
old_records = {}
for original, expected_hash in manifest['previous_catalog_sha256'].items():
    raw = (directory.parent / original / 'catalog.csv').read_bytes()
    old_manifest = json.loads((directory.parent / original / 'manifest.json').read_text())
    assert hashlib.sha256(raw).hexdigest() == expected_hash == old_manifest['catalog_sha256']
    seen = {int(r['id']) for r in csv.DictReader(raw.decode().splitlines())}
    assert not set(ids) & seen and not seen & old_ids, 'catalog ID collision'
    old_ids |= seen
    old_records.update({int(r["id"]): r for r in csv.DictReader(raw.decode().splitlines())})
assert len(old_ids) == 225

# New evidence is independent from older manifests' historical retrieval flags.
immutable_bytes = (directory / manifest['immutable_source_file']).read_bytes()
assert hashlib.sha256(immutable_bytes).hexdigest() == manifest['immutable_source_sha256']
evidence = json.loads((directory / manifest['immutable_comparison_file']).read_text())
assert evidence['immutable_source_sha256'] == manifest['immutable_source_sha256']
immutable_rows = list(csv.reader(immutable_bytes.decode().splitlines()))
assert len(immutable_rows) == evidence['immutable_rows'] == 364
columns = ['type','attack_kind','attack_extra','defense_kind','defense_extra','attribute','value','sub_value','hit_rate','price','weight']
immutable = {id: dict(zip(columns,r)) for id,r in enumerate(immutable_rows)}
for row in source:
    assert {f: row[f] for f in columns} == immutable[int(row['id'])]
numeric_checks = 0
for id, row in old_records.items():
    r = immutable[id]
    value, sub = int(r['value'] or 0), int(r['sub_value'] or 0)
    facts = {
        'attack': value if r['attack_kind'] == 'ATK' else 0,
        'boost': (value if r['type'] in ['WEAPON','MAGIC'] else sub) if r['attack_extra'] == 'INCREASE_ATK' else 0,
        'defense': (sub if r['type'] == 'WEAPON' else value) if r['defense_kind'] == 'DFS' else 0,
        'hp': value if r['type'] == 'SUNDRY' and r['attack_kind'] == 'INCREASE_HP' else 0,
        'mp': value if r['type'] == 'SUNDRY' and r['attack_kind'] == 'INCREASE_MP' else 0,
        'amount': value, 'cost': sub if r['type'] == 'MAGIC' else 0,
        'price': int(r['price'] or 0), 'weight': int(r['weight'] or 0), 'hit_rate': int(r['hit_rate'] or 0),
    }
    for field, value in facts.items():
        if field in row and row[field] != '':
            assert int(row[field]) == value, (id,field)
            numeric_checks += 1
assert numeric_checks == evidence['older_numeric_field_checks'] == 1531
covered, source_checks = set(), 0
for name in manifest['previous_catalog_sha256']:
    for path in (directory.parent / name).glob('*source-rows.csv'):
        for row in csv.DictReader(path.open()):
            if not {'id',*columns}.issubset(row): continue
            id = int(row['id'])
            covered.add(id)
            for field in columns:
                assert row[field] == immutable[id][field], (name,id,field)
                source_checks += 1
assert covered == old_ids and len(covered) == evidence['older_catalog_ids'] == 225
assert source_checks == evidence['older_source_column_checks'] == 4169

prices = (directory.parent / 'legacy-selling-v1/sale-prices.csv').read_bytes()
selling = json.loads((directory.parent / 'legacy-selling-v1/manifest.json').read_text())
assert hashlib.sha256(prices).hexdigest() == manifest['previous_sale_prices_sha256'] == selling['sale_prices_sha256']
assert not set(ids) & {int(r['id']) for r in csv.DictReader(prices.decode().splitlines())}
normalized = io.StringIO(newline='')
writer = csv.writer(normalized, lineterminator='\n')
writer.writerow(['id','label','element','cure','disease','harm','cost','price','weight'])
for original, r in zip(source,expected):
    id,element,cure,disease,harm,cost,price,weight = r
    assert original['id'] == str(id) and original['type'] == 'MAGIC' and original['sub_value'] == str(cost)
    assert original['price'] == str(price) and original['weight'] == str(weight)
    assert original['attribute'] == ('DARK' if element == 'dark' else '')
    for field in ('defense_kind','defense_extra','value','hit_rate'): assert original[field] == ''
    kind = 'REMOVE_LOWER_HARMS' if cure == 'lower' else 'REMOVE_ALL_HARMS' if cure == 'all' else 'ADD_HARM'
    extra = 'DARK_CLOUD' if harm == 'dark_cloud' else 'COLD' if disease == 'cold' else 'HEAVEN' if disease == 'heaven' else ''
    assert original['attack_kind'] == kind and original['attack_extra'] == extra
    assert sum(v != 'none' for v in [cure,disease,harm]) == 1
    writer.writerow([id,f'legacy-{id}',*r[1:]])
assert normalized.getvalue().encode() == catalog_bytes
header = (project / 'include/godfield/status_miracles.hpp').read_text()
assert manifest['catalog_sha256'] in header
assert all(str(id) in header for id in retained)
generated = '// Generated by data/legacy-status-miracles-v1/generate.py.\n// Source rows and retrieval limits: data/legacy-status-miracles-v1/manifest.json.\n#include "godfield/status_miracles.hpp"\n#include <array>\n\nnamespace godfield::legacy {\nnamespace {\nconstexpr std::array<StatusMiracle, status_miracle_count> records{{\n    {202, "legacy-202", Element::neutral, Cure::lower, Disease::none, Harm::none, 3, 10, 1},\n    {208, "legacy-208", Element::dark, Cure::none, Disease::none, Harm::dark_cloud, 5, 10, 1},\n    {211, "legacy-211", Element::neutral, Cure::all, Disease::none, Harm::none, 5, 15, 1},\n    {212, "legacy-212", Element::neutral, Cure::none, Disease::cold, Harm::none, 6, 10, 1},\n    {224, "legacy-224", Element::neutral, Cure::none, Disease::heaven, Harm::none, 15, 15, 1},\n}};\nconstexpr auto make_index() {\n    std::array<std::uint8_t, 225> index{};\n    for (std::size_t i = 0; i < records.size(); ++i) index[records[i].id] = static_cast<std::uint8_t>(i + 1);\n    return index;\n}\nconstexpr auto index = make_index();\nconstexpr bool valid_records() {\n    for (const auto& row : records) {\n        const int effects = (row.cure != Cure::none) + (row.disease != Disease::none) + (row.harm != Harm::none);\n        if (effects != 1 || row.cost <= 0 || row.weight != 1 || row.price < 0) return false;\n    }\n    return true;\n}\nstatic_assert(valid_records());\nstatic_assert(status_retained_type_ids.size() == 27 && records[0].id == 202 && records[4].id == 224);\n}\nstd::span<const StatusMiracle> status_miracles() noexcept { return records; }\nint status_miracle_index(CardId id) noexcept {\n    return id < index.size() && index[id] ? static_cast<int>(index[id]) - 1 : -1;\n}\nconst StatusMiracle* find_status_miracle(CardId id) noexcept {\n    const int slot = status_miracle_index(id);\n    return slot < 0 ? nullptr : &records[slot];\n}\n}\n'
path = project / 'src/status_miracles.cpp'
if args.check:
    assert path.read_text() == generated, 'status_miracles.cpp needs regeneration'
    print('Status miracles verified: five types,27 retained IDs;35 catalogs and216 prices intact.')
else:
    path.write_text(generated)
    print(f'Generated {path}.')
