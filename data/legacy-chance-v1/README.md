# Legacy chance catalog v1

This separately versioned allowlist contains 12 historical weapons with a
positive `hitRate` field, no extra attack effect, and no special defense effect.
ID113 additionally has ordinary defense value 6. Existing neutral, miracle and
deterministic element catalogs and fingerprints are unchanged.

| Element | Count |
| --- | --- |
| Fire | 4 |
| Water | 3 |
| Wood | 2 |
| Stone | 3 |

Seven records have `hitRate=75`, three have 50, and two have 25. Each historical
acquisition weight is 1. No simple neutral record matched the inspected source
selection. Names such as `legacy-100` are technical labels, not current official
names or IDs.

`source-rows.csv` records selected original fields with zero-based row indices
from the historical emulator's
[ItemData.CSV](https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV).
`catalog.csv` normalizes those facts. Both are independently hashed in
`manifest.json`; the generator checks their exact correspondence and ID
disjointness from previous catalogs.

The table preserves `hitRate` without interpreting it as a maximum hit count or
as a one-shot-only guarantee. Whether an attack repeats and when it ends must
be established from the engine's resolution rules. No `hit_count` field has
been inferred or added.

Light/Dark IDs101/104/111/117 and status/absorption IDs102/103 are excluded;
their extra behavior is not erased. Special defense, miracles and counters are
also outside this pool. Unsupported IDs return nullptr without fallback.

The branch-history revision is recorded, but immutable raw content could not
be retrieved; the observed master rows are not asserted byte-identical to it.
The old emulator's [README](https://github.com/Igoorx/godfield-flash#known-issues)
warns that acquisition distribution may be inaccurate. Restricted pool weights
are not verified official probabilities, and current-client compatibility is
unverified.

Regenerate and check from the repository root:

```sh
python3 data/legacy-chance-v1/generate.py
python3 data/legacy-chance-v1/generate.py --check
```
