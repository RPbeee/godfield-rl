# Legacy Stone counter v1

This independent catalog supports only old ID190. The twelve earlier catalogs,
fingerprints and miracle indices remain unchanged.

| Old ID | Technical label | Source type | Attack kind | Defense kind | Element | Counter multiplier | Old price | Weight |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 190 | legacy-190 | PROTECTOR | ATK | COUNTER | Stone | 2 | 10 | 1 |

The source has no attack extra, defense extra, numeric value, secondary value or
percentage. The normalized `Kind::armor` record has zero attack, boost, defense,
HP and MP and is discardable. `CounterCard` membership preserves its counter
effect; it must not be reduced to plain zero-defense armor. It has no MP cost and
adds no retained miracle index.

Historical [defenderCommand][turn] queues counters only when incoming damage
remaining after ordinary defense is positive. ID190's counter damage is twice
that raw residual, before original attack HP clipping or Dark execution. For
example, a raw residual of 20 produces a counter of 40 even if the original
target has only 3 HP. A Dark original attack does not replace that residual with
its 999-style execution damage. The source queues counters before applying the
original attack; later queue execution and terminal outcomes require engine
validation beyond this table.

This profile follows the old server acceptance path: ordinary defense, ID195
attribute removal and further counters can be used against a counter. The old
bot's intended handling differs; this remains unresolved compatibility issue
C05. No claim of current official-client fidelity follows from choosing that
historical server path. Supporting only one counter kind avoids unlike-counter
queue ordering choices, while preserving the multiplier and element. Exact
reproduction of every original XML trace is not asserted.

Other counter records are completely excluded: 187 (percentage Fire), 188 (Fog),
189 (Illusion), 191 (Glory), 192 (Dark Cloud), 193 (MP recovery), and 194 (money
absorption). Their effects are not stripped to manufacture additional members.

`source-rows.csv` retains original fields and zero-based IDs from
[ItemData.CSV][csv]. `catalog.csv` stores normalized values. Old IDs and technical
labels are not verified current official names or identifiers. `manifest.json`
fingerprints both datasets and pins the twelve earlier catalog hashes.
Immutable raw source was unavailable; observed master rows are not asserted
byte-identical to the recorded history-head commit. Historical weights are not
verified official probabilities; upstream [reports distribution uncertainty][issues].

Regenerate or verify from the repository root:

```sh
python3 data/legacy-counter-v1/generate.py
python3 data/legacy-counter-v1/generate.py --check
```

[turn]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L588-L602
[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[issues]: https://github.com/Igoorx/godfield-flash#known-issues
