# Legacy light/dark catalog v1

This separately versioned allowlist adds 18 historical records: 16 weapons and
two armor records. Ten are Light and eight are Dark; every legacy weight is 1.
The earlier seven catalogs and their fingerprints remain unchanged.

| Roles | Historical IDs and details |
| --- | --- |
| Deterministic weapons | 62, 63, 70, 75, 76, 82, 83, 87, 88, 89, 93, 96, 97 |
| Additive attacks | ID62 retains both Light ATK1 and boost1; ID89 retains both Dark ATK5 and boost5 |
| Glory source | ID70 retains Light ATK2 and its Glory attack extra |
| Chance weapons | ID101 Dark ATK2/50%, ID104 Light ATK3/25%, ID111 Light ATK6/75% |
| Armor | ID158 Light DEF6; ID176 Light DEF10 |

Glory has one ordinary acquisition source when this separate pool is enabled.
The older `legacy-harms-v1` pool still has no Glory source and is not modified.
Percentage fields and secondary effects are preserved, not stripped to admit
cards. All source defense extras are empty, so `CelestialCard` needs no
`on_defense` field in this version.

The table does not implement Light/Dark combat semantics. Element combination,
Light substitution, defense compatibility, Dark's lethal effect, and interactions
with chance or Glory must be validated by the engine.

ID99 (`PESTLE`) and ID117 (`DYING_ATTACK`) are excluded; their extra behavior
cannot be silently removed. Counter records ID191/ID192, all miracles and
guardian effects are also excluded. No new mirror, status disease or miracle
API is inferred from this data. Unsupported IDs return nullptr.

`source-rows.csv` preserves the selected original fields and zero-based row
indices from the historical emulator's
[ItemData.CSV](https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV).
`catalog.csv` normalizes those fields. Technical labels and old IDs are not a
verified current-client name/ID mapping. `manifest.json` fingerprints both files
and records the selection and retrieval limits. The generator verifies exact
normalization and ID disjointness from all earlier catalogs.

Immutable raw content was unavailable; the observed master rows are not asserted
byte-identical to the recorded branch-history revision. Historical weights are
not verified official acquisition probabilities; the upstream
[README](https://github.com/Igoorx/godfield-flash#known-issues) reports distribution
uncertainty. Current official compatibility is unverified.

Regenerate and verify from the repository root:

```sh
python3 data/legacy-celestial-v1/generate.py
python3 data/legacy-celestial-v1/generate.py --check
```
