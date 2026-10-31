# Legacy four-element catalog v1

This separate allowlist contains 54 deterministic historical records with
positive acquisition weight: 23 weapons and 31 armor records. It does not
modify the neutral or miracle catalogs, IDs, or fingerprints.

| Element | Weapons | Armor | Total |
| --- | --- | --- | --- |
| Fire | 8 | 9 | 17 |
| Water | 5 | 8 | 13 |
| Wood | 6 | 7 | 13 |
| Stone | 4 | 7 | 11 |

The historical source calls Wood `TREE` and Stone `SOIL`. Seven weapons also
have an attack boost; ID78 is both attack and defense capable. All 54 records
have historical weight 1. Labels such as `legacy-078` are internal IDs, not
verified official names or current-client IDs.

`source-rows.csv` contains selected original fields and their zero-based row
indices from the historical emulator's
[ItemData.CSV](https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV).
`catalog.csv` normalizes those fields into the native `ElementCard` representation.
Both files have independent SHA-256 fingerprints in `manifest.json`. Generation
checks the source selection, normalization, record counts, and ID disjointness
from the existing catalogs.

The selection does not strip special effects. It excludes Light/Dark,
probabilistic hits, conditions, attribute-adding ID65/ID85, reflection,
blocking, counters, and miracles. Zero-weight alias records such as
ID314/ID324/ID332/ID333 are outside the ordinary acquisition pool and are
also excluded. Unsupported IDs return nullptr, without a neutral fallback.

The branch-history commit is recorded but immutable raw content could not be
retrieved; the observed master rows are not asserted byte-identical to it.
Historical weights are not verified official probabilities; the upstream
[README](https://github.com/Igoorx/godfield-flash#known-issues) reports uncertainty
about artifact distribution. Current official-client compatibility remains
unverified. Element combination and defense semantics belong to the engine,
not this catalog.

Regenerate or verify from the repository root:

```sh
python3 data/legacy-elements-v1/generate.py
python3 data/legacy-elements-v1/generate.py --check
```
