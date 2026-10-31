# Legacy HP absorption v1

All four positive-weight ABSORB_HP rows are preserved in this independent catalog.
The source/normalized fingerprint is shared; runtime weapon and MAGIC arrays have
separate types. All twenty-seven earlier catalogs and local indices remain fixed.

| Old ID | Type | Element / ATK | MP | Rate | Price / weight |
| --- | --- | --- | --- | --- | --- |
| 26 | WEAPON | NE / 7 | 0 | 0 sentinel | 10 / 1 |
| 42 | WEAPON | NE / 11 | 0 | 0 sentinel | 15 / 1 |
| 103 | WEAPON | Wood / 3 | 0 | 75 | 10 / 1 |
| 221 | MAGIC | Light / 10 | 10 | 0 sentinel | 15 / 1 |

Weapons have boost/DEF/staticHP/MP0, no other special effect and are non-discardable.
Runtime MAGIC221 stores amount10/cost10/weight1; source price15 remains in both CSVs.
It can be used standalone/retained/FREE, but ABSORB_HP is not a legal weapon extra.
The75% weapon103 rejects all additions under the existing percentage-base rule.

[Damage resolution][effect] first heals the current source by the positive raw
post-defense amount, capped at99, then damages the target. This is not a transfer
limited by actual target HP loss. Reflection/bounce changes the current source;
a self-directed item heals first and then damages that same seat. The source's
DARK branch suppresses absorption and applies999 damage instead. Natural disease
and automatic244 healing are separate effects.

Main ability slot15 appends221 while keeping the old15 main and every other family
index unchanged. The original forgetting21-ID array is untouched; the new sorted22
`hp_absorption_retained_type_ids` inserts221 between220 and222. Old-profile sampling,
ABI widths and checksum loops must continue using their earlier limits.

`source-rows.csv` preserves all columns and `catalog.csv` stores normalized facts.
The generator verifies fingerprints, generated C++, the header hash, old27hashes,
ID disjointness and both old21/new22 forgetting orders. IDs/labels/weights are not
verified official identifiers, names or acquisition probabilities. Master was
observed2026-10-31; immutable retrieval was unavailable, so history-head does not
prove content identity.

```sh
python3 data/legacy-hp-absorption-v1/generate.py
python3 data/legacy-hp-absorption-v1/generate.py --check
```

[effect]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L483-L494
