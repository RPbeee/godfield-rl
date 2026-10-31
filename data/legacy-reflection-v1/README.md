# Legacy neutral weapon reactions v1

This independent allowlist preserves two simple neutral historical weapons.

| Historical ID | Internal label | Base attack | Weapon reaction | Price | Weight |
| --- | --- | --- | --- | --- | --- |
| 20 | legacy-020 | 5 | bounce (`FLICK_WEAPON`) | 10 | 6 |
| 39 | legacy-039 | 10 | reflect (`REFLECT_WEAPON`) | 5 | 3 |

Both records have no attack extra, attribute, hit-rate field or ordinary defense
value. Their weapon reaction is separate from the base attack. Both remain
non-discardable weapons. Internal labels and historical IDs are not verified
current official names or IDs.

No positive-weight simple non-miracle `BLOCK_WEAPON` record matched this
selection. Miracle ID215 is excluded. ID196 has `REFLECT_ANY` and is also
excluded: restricting its behavior to neutral weapons would remove part of
the artifact's effect. Magic reactions, conditions, elements and other extra
effects are not silently stripped.

Original fields and zero-based row indices from the historical emulator's
[ItemData.CSV](https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV)
are preserved in `source-rows.csv`; `catalog.csv` is their normalized form.
`manifest.json` hashes both and records the selection and retrieval limits.
Generation verifies exact normalization and ID disjointness from all four
earlier catalogs. Earlier data and fingerprints remain unchanged.

The table maps reaction types only. Target selection, redirection, response
ordering, consumption and chain completion require engine validation.
`WeaponReaction::none` and `block` are reserved enum values without matching
records in this version. Unsupported IDs return nullptr without fallback.

Immutable raw content was unavailable, so observed master rows are not asserted
byte-identical to the recorded branch-history revision. Historical weights
are not verified official acquisition probabilities; the upstream
[README](https://github.com/Igoorx/godfield-flash#known-issues) warns that its
distribution may be inaccurate. Current official compatibility is unverified.

Regenerate and verify from the repository root:

```sh
python3 data/legacy-reflection-v1/generate.py
python3 data/legacy-reflection-v1/generate.py --check
```
