# Legacy defensive miracles v1

This separate catalog adds two historical defensive miracles without modifying
any of the nine earlier catalogs, fingerprints or general miracle indices.

| Old ID | Internal label | Reaction kind | MP | Old price | Weight | Defensive local slot |
| --- | --- | --- | --- | --- | --- | --- |
| 210 | legacy-210 | bounce_magic (`FLICK_MAGIC`) | 5 | 15 | 1 | 0 |
| 215 | legacy-215 | block_weapon (`BLOCK_WEAPON`) | 6 | 15 | 1 | 1 |

Both source records are neutral, have no attack or ordinary defense value,
and have no extra attack effect or percentage. Their prices are preserved in
`source-rows.csv` even though the runtime type only needs kind, cost and weight.

The defensive slots are a separate `defensive_abilities` group. They must not
renumber or append to the existing general slots0..14. `defensive_miracle_index()`
returns0/1 or -1 for unsupported IDs; lookup returns nullptr for unsupported IDs.

ID210 applies to effects whose originating item is MAGIC, including offensive,
HP-recovery and money miracles. Limiting it to damaging magic would remove part
of the artifact's behavior. The historical server skips ordinary element-defense
matching for MAGIC reactions, including Light/Dark miracles. ID215 blocks
neutral WEAPON-derived attacks; this does not infer a Dark-weapon bypass.
These are historical target classifications, not a resolution of the separate
current-official Light-defense discrepancy C01.

The source's
[defenderCommand](https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py)
distinguishes first use from retained reuse, MP cost, deferred acquisition,
prefix defense effects and redirected targets. Those rules require engine
validation beyond this table. Retained ID210 can be used without consuming a
hand card, so a chain budget derived only from initial hand counts is insufficient.

Original fields and zero-based source indices are retained from
[ItemData.CSV](https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV).
`catalog.csv` is their normalized runtime form. Technical labels and old IDs
are not verified current official names or IDs. The generator verifies exact
normalization and disjointness from all earlier pools, and checks their pinned
catalog fingerprints. Unsupported other reactions are not simplified to these
two kinds.

`manifest.json` independently fingerprints source and normalized data and records
retrieval limits. Immutable raw content was unavailable; the observed master
rows are not asserted byte-identical to the recorded history-head revision.
Historical weights are not verified official probabilities; upstream's
[README](https://github.com/Igoorx/godfield-flash#known-issues) reports distribution
uncertainty. Current official-client compatibility is unverified.

Regenerate and verify from the repository root:

```sh
python3 data/legacy-defensive-miracles-v1/generate.py
python3 data/legacy-defensive-miracles-v1/generate.py --check
```
