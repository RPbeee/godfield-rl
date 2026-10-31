# Legacy pure status and cure miracles v1

Five MAGICs are added:202 lower cure MP3/price10;208 DARK direct DarkCloud
MP5/price10;211 all cure MP5/price15;212 neutral Cold MP6/price10;224 neutral Heaven
MP15/price15. All have weight1, zero raw attack value/hit rate and no defense role.
Every normalized record has exactly one cure/disease/harm. The pool becomes230
artifacts with weight484; merchandise max30 and raw/counter bounds stay unchanged.

[Using a physical MAGIC][holding] learns one new-family copy and pays MP; recasting
preserves copies and pays again, unless the existing adjacent FREE rule waives that
use. Local0..4 follow IDs202/208/211/212/224. A separate new27-ID retained list includes
these types for weighted forgetting; old21/22 lists and all earlier holding indices
are unchanged. The old216 price table/hash stay frozen; these five prices are new
runtime fallbacks only when the feature is enabled.

[Resolution][resolution] invokes existing disease/harm/cure behavior, including
[Heaven][harm] and [lower/all removal][cure]. These are main/cast effects, not raw
weapon damage or attack extras. Matching MAGIC reactions preserve original MP and
provenance while returning or blocking the status payload.

Canonical defense excludes consuming a no-defense status MAGIC for zero DEF. The
source weak response guard may pay/learn it without executing its main status
effect, with possible response-time learning/hand-cycling value. This acceptance
is documented separately rather than claimed redundant or fully implemented.

The generator checks source/catalog hashes, generated C++, header fingerprint,
all old35 catalog hashes and ID disjointness, plus the frozen216 price table hash.
FOG/ILLUSION, assistants, MYSTERY and117 remain excluded. Historical IDs/labels/
prices/weights are not verified current official identifiers, names or probabilities.
Master was observed2026-11-01. Root subsequently retrieved the fixed commit
`fcc83740a3d3cd9968e6e5cd2b7ed781f53abe80` through gh API despite child curl/web failures.
[New immutable evidence](immutable-comparison.json) records the full364-row raw
CSV SHA256 `a6b6892d6234656c40d5171b042ce7aeefcd309ae1abcd6c3fa61e55c552f8b1`. All five selected status rows, old225 IDs,1531 available
numeric facts and4169 stored source-column checks match. The cached web master
reconstruction also matches those raw bytes. [The raw archive](immutable-item-data.csv)
is pinned by the generator. This new evidence does not rewrite older manifests'
historical retrieval flags or prove current official behavior/future master identity.

```sh
python3 data/legacy-status-miracles-v1/generate.py
python3 data/legacy-status-miracles-v1/generate.py --check
```

[holding]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L276-L301
[resolution]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L477-L482
[harm]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L147-L165
[cure]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L184-L203
