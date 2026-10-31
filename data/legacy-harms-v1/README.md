# Legacy Glory/Dark Cloud sources v1

The strict allowlist contains **one** source record: historical ID43,
neutral ATK11 with a Dark Cloud attack extra, price15 and legacy weight1.
It has no ordinary defense, other extra, attribute or hit-rate field.

**No Glory source matches this pool.** The inspected Glory records are ID70
(Light weapon), ID191 (Light counter protector), and ID200 (Light probabilistic
miracle). They remain excluded; their attributes and effects are not stripped
to manufacture a simple source. Dark Cloud's ID192 (Dark counter) and ID208
(Dark miracle) are also excluded.

`Harm::glory` exists to represent explicitly constructed test states. Ordinary
draws from this catalog cannot produce a Glory-applying artifact. Benchmark or
training reports must distinguish such fixtures from normal pool acquisition.

Historical zero-based source fields from
[ItemData.CSV](https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV)
are retained in `source-rows.csv`; `catalog.csv` normalizes them into `HarmCard`.
`legacy-043` is a technical label, not a verified current official name or ID.
The table records the source effect only: damage prerequisites, target,
stacking, cure interaction and execution timing require engine validation.

`manifest.json` independently fingerprints source and normalized data and
records all related exclusions. The generator checks normalization and ID
disjointness from all six earlier catalogs. Earlier pools and fingerprints
remain unchanged.

Immutable raw content could not be fetched; observed master rows are not
asserted byte-identical to the recorded history-head revision. Historical
weights are not verified official probabilities; upstream's
[README](https://github.com/Igoorx/godfield-flash#known-issues) reports distribution
uncertainty. Current official-client compatibility is unverified.

Regenerate and verify from the repository root:

```sh
python3 data/legacy-harms-v1/generate.py
python3 data/legacy-harms-v1/generate.py --check
```
