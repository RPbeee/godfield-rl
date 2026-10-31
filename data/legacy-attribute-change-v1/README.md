# Legacy attribute-change weapons v1

This independent catalog preserves the two historical `ADD_ATTRIBUTE` weapons.
All ten earlier catalogs, fingerprints and retained miracle indices are unchanged.

| Old ID | Technical label | Element setter | Standalone ATK | Added ATK | DEF | MP cost | Old price | Weight |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 65 | legacy-065 | Fire | 2 | 0 | 0 | none | 15 | 1 |
| 85 | legacy-085 | Water | 5 | 0 | 0 | none | 15 | 1 |

Both rows are WEAPON/ATK with `ADD_ATTRIBUTE`, no other attack effect, no defense
effect and no percentage. `Card.attack` preserves the standalone value, while
`Card.boost` is deliberately zero. Their distinct catalog membership carries the
attribute-setter behavior; treating them as ordinary boosts would lose the effect.

Historical [queueAttack][turn] adds damage only for `INCREASE_ATK`. An
`ADD_ATTRIBUTE` weapon establishes damage if it is first, but does not add its
printed value to an already established attack. At its position in the command
sequence, it overwrites the accumulated element. Later ordinary boosts still
participate in normal element mixing. For example, a neutral boost after a Water
setter returns the result to neutral, while putting that setter last yields Water.
The implementation must preserve the reachable results of these order choices;
a hand-slot bitmask with one fixed arbitrary traversal would omit valid outcomes.

[AttackData.isValidAttackItem][attack] allows the setter as an addition only to a
WEAPON base whose percentage is zero. It cannot augment a MAGIC base or a
percentage weapon. Both setters can be used alone as ordinary weapon bases.
There are no MAGIC or PROTECTOR `ADD_ATTRIBUTE` records in the inspected CSV.
ID195 `REMOVE_ATTRIBUTE` is a separate defensive operation and is excluded.

Current official attack-addition display and this historical non-addition behavior
remain an unresolved compatibility discrepancy (C04). The normalized old data
does not assert current official behavior or resolve that discrepancy.

Original zero-based source rows are retained from [ItemData.CSV][csv] in
`source-rows.csv`; `catalog.csv` is their normalized runtime form. Names are
technical labels, not verified official names. `manifest.json` fingerprints both
datasets and pins all ten earlier catalog hashes. Immutable raw content could not
be retrieved, so observed master rows are not asserted byte-identical to the
recorded history-head commit. Old weights are not verified official acquisition
probabilities; upstream [reports distribution uncertainty][issues].

Regenerate or verify from the repository root:

```sh
python3 data/legacy-attribute-change-v1/generate.py
python3 data/legacy-attribute-change-v1/generate.py --check
```

[turn]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L226-L286
[attack]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L78-L94
[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[issues]: https://github.com/Igoorx/godfield-flash#known-issues
