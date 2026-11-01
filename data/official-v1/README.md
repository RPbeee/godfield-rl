# official-v1: current public catalog facts

Source: the ordinary public [Japanese data asset](https://godfield.net/i18n/ja.json) loaded by
[the official browser client](https://godfield.net/main.dart.js), retrieved on2026-11-01.
No account, authentication or game/match connection was used.
This catalog is independent from all legacy profiles and does not claim that a complete official simulator exists.

`cards.json` archives only all296 item metadata records, preserving omitted source keys and adding
`id=array_index+1`, exactly as the delivered client loader `A.ty` does. Localized prose, executable
client code and images are not bundled. UTF-8 names and image slugs are short metadata.
`manifest.json` and `http-validators.json` record the original complete JSON/client SHA256, size,
source URL, HTTP ETag and Last-Modified. The original fulltext assets are cached outside the repository.

| Artifact | SHA256 |
| --- | --- |
| Complete source ja.json,49,137bytes | 03a4782dc6899aeedd9b7986a49bd3c0217b96ac38c0779deb588bc94cca3e7c |
| Complete client JS,440,238bytes | 764a50524e4b6b3f510415da7128abd8ad99dcb87b45b8572d98ddd96889cabd |
| Normalized item facts | 9ce45da9dd372da1ab30397068885a20bbd5927b0891946b5fc7bc07226fa1a8 |

The source data's Last-Modified was2026-10-29 07:43:52GMT. A client JS hash matching an older
third-party observation does not prove its separately fetched JSON is unchanged.
The official primary metadata above is distinct from the historical third-party Bible extraction.

## Typed facts and defaults

[official_catalog.hpp](../../include/godfield/official_catalog.hpp) provides `godfield::official::Card`,
`catalog()`, `find_card(id)` and `card_index(id)`. IDs1..296 are contiguous only within this snapshot;
unknown IDs return null/-1. Do not reuse legacy0-based IDs or enum ordinals as a server wire protocol.
Category/Element/Ability/Curse/Guardian are typed, and each `*_name` function restores the source code string.
Ability has60 source codes plusnone. Names and image slugs remain exactly as delivered.

A missing element stays `std::nullopt`, not an inferred neutral or untyped attack.
Missing numeric fields default0, as the actual JS loader does; missing ability/curse/guardian
become none. Present `isPlusAtk` is checked for non-null by the loader; all30 present values
in this snapshot are true. No runtime effect, targeting, defense acceptance or cost is inferred from missing metadata.

| Category | Records | Sum giftRate |
| --- | --- | --- |
| trade | 5 | 60 |
| weapons | 107 | 181 |
| armor | 78 | 147 |
| sundries | 19 | 82 |
| miracles | 30 | 30 |
| devils | 5 | 0 |
| guardians | 42 | 0 |
| phenomena | 10 | 0 |

237 records have positive giftRate, totaling500. The other categories and fixed commands are
still catalog facts, not unsupported records silently dropped from the archive.
The client displays ordinary giftRate/500, guardianAttackRate/20 and appearanceRate as percent.
These are separate fields/distributions. A guardian association on a regular card is not itself a summon operation.

## Legacy-v37 research crosswalk

`legacy-v37-crosswalk.json` covers every231 currently supported old record without changing any
of the37 legacy catalog hashes or the old216 price rows.
The old immutable CSV contains technical numeric IDs and effect values but no item names or image slugs.
Therefore every row keeps `identity_verified=false`: there is no automatic runtime ID conversion.

The deterministic comparison projects old labels/roles into the current vocabulary, then compares
category/element/ability/curse/additional-ATK role, followed by ATK/DEF/rate/abilityValue,
then cost/price/giftRate. It never chooses a record by numeric position.
There are196 provisional unique candidates,34 ambiguous cases and1 unmapped case (old95).
Among unique candidates101 have equal comparable metadata and95 differ. Differences include
price76, giftRate23, abilityValue4, ATK3, cost3 and hitRate1; a row can differ in several fields.
These counts describe candidate comparisons, not proven historical identity or behavioral equivalence.

| Old ID | Provisional official candidate | Concrete metadata difference |
| --- | --- | --- |
| 20 | 21 bouncing-sword | giftRate6→7 |
| 117 | 112 ascension-bow | hitRate75→25; both preserve death power30 |
| 202 | 237 tone | cost3→2; legacy price10 vs omitted current price(default0) |
| 216 | 235 spring | cost6→7; legacy price15 vs omitted current price |
| 220 | 236 treasure | amount15→10, cost8→5; legacy price25 vs omitted current price |
| 240 | 205 nocturnal-broom | legacy giftRate3→current1; old value1 vs omitted current abilityValue |
| 241 | 206 goddess-s-soap | legacy giftRate3→current1; old value1 vs omitted current abilityValue |
| 245 | 209 dangerous-mortar | old price0→1; current abilityValue99 is explicit |

An omitted abilityValue does not mean the effect does nothing: the current client removes3 items
for nocturnal-broom via handler logic. Handler constants and operation order are separate evidence.
GLORY→flash and ILLUSION→dream in the comparison are tentative vocabulary, not a proof that the effects match.
Old65/85 and current66/83 match projected display metadata, while old ADD_ATTRIBUTE's non-additive
execution can still differ from current+ATK composition. Equal metadata cannot settle that conflict.
Old218/219/223 retain several elemental miracle candidates rather than force a guessed name mapping.

## Reproduction

```sh
python3 data/official-v1/generate.py --check
python3 data/official-v1/fetch.py --cache /tmp/godfield-official-cache --require-match
```

The first command is offline and checks facts, generated C++, all37 frozen legacy catalogs and the
entire derived crosswalk. The second performs only public HTTP GETs and writes full original assets
outside the repository. A changed source hash fails `--require-match`; do not silently relabel it as this snapshot.
Use `generate.py` without `--check` to regenerate C++ from the archived facts.

The catalog fixes metadata only. Actual client helper vectors are separately owned evidence in
`client-vectors.tsv`/`client-vectors.manifest.json`; backend transitions, hidden information,
Fog/Dream, guardian behavior, full match lifecycle and all mode rules require further implementation/verification.

## Public-client oracle fixtures

`client-vectors.tsv` contains50,270 results produced by executing the pinned public client's
actual `tw/pq/l9/tx/la/e1/kO` and visible selection `jM.ev` functions. The C++ tests compare
ordered composition, reactions, attribute defense, and visible-model append predicates against
these results. They do not replay server transitions or establish whole-action legality.

The optional Node.js regeneration script checks the full client SHA256 before loading it into
a VM with `dartMainRunner` disabled. The application never starts; no DOM, Firebase, timers,
network APIs, user credentials, or room connection are provided. The fixture uses the archived
model facts with the actual client loader. The executable client bundle remains outside the repo.

```sh
python3 data/official-v1/fetch.py --cache /tmp/godfield-official-cache --require-match
node scripts/official_client_oracle.cjs --client /tmp/godfield-official-cache/main.dart.js
python3 scripts/check_official_vectors.py
```

Node.js is needed only to regenerate the reference results. Normal C++/Python tests read the
committed fixture offline. The fixture manifest pins its SHA256, source/catalog hashes and counts.

Numeric fixture results stay within JavaScript's exact integer range (up to2^53-1).
The native composition API also accepts larger values using exact, checked int64 arithmetic;
that extension does not reproduce JavaScript Number rounding beyond its safe integer range.
The fixture is not a claim of equivalence for arbitrary huge overrides or unbounded input lists.

## Fresh-state/view fixtures

`state-vectors.tsv` adds22,140 cases: actual enemy/alive/ending-display helpers,
visible-model and tooltip helpers, wire Set normalization and curse/death display,
and fresh hand sorting. Fog cases execute the unique predicate expressions extracted
from the pinned fresh-snapshot branch with actual team enums and alive filtering.
Widget construction is stubbed where needed; this is not a whole DOM/game session.
The separate `state-vectors.manifest.json` records these limits and source fingerprints.

```sh
node scripts/official_state_oracle.cjs --client /tmp/godfield-official-cache/main.dart.js
python3 scripts/check_official_vectors.py
python3 python/official_state.py
```

[Snapshot/view contracts](../../docs/official-state-view.md) describe public used-miracle
panels, aliases, fog and dream. [state-schema.json](state-schema.json) documents the
strict import value shape and additional invariants; the implementation also rejects
float/bool/string values for integer fields, duplicate keys and invalid UTF-8.
Unknown JSON, full attack tails and events remain privileged raw evidence and are not
projected or replayed. The C++ projection uses at most18 own actual items because the
fresh client renderer cannot display a19th nonempty instance. It does not impose a
server capacity on foreign inventories or implement nine-player match transitions.

## Resource-event arithmetic and scoped execution

`resource-vectors.tsv` contains7,898 results from the actual client's `c5.eW`,
`c5.dH`, and `c5.bc`: HP clipping and MP/CP debt cascading. The resource primitive
never implicitly marks a player dead. Its fixture manifest pins the source hash.

```sh
node scripts/official_resource_oracle.cjs --client /tmp/godfield-official-cache/main.dart.js
python3 scripts/check_official_vectors.py
```

These results verify the delta arithmetic, not the server's choice of deltas or
its scheduling. The executable132-model pool and its308 normalized gift weight,
turn/death/gift policies, unsupported boundaries and API are documented separately
in [the experimental execution contract](../../docs/official-execution-subset.md).
The full published pool remains237 positive-gift models with total weight500.


## Upstream review on2026-11-02

[The upstream review](upstream-review-2026-11-02.json) records newer public JSON/JS
fingerprints without replacing the original immutable `official-v1` pins. All296
item rows were compared: three image/name metadata rows changed, while numerical
values, abilities and gift weights stayed equal. The inspected removal handlers,
ordered item display and true-model reveal also stayed locally equivalent after
compiler renaming; the complete new client was not audited. Server event fanout,
empty-result emission and the local removal-history lifetime remain unverified or
explicit experimental contracts.
