# Legacy counter extensions v1

This separate catalog adds four complete historical counters. All thirteen earlier
catalogs, fingerprints and miracle indices remain fixed, including ID190's
independent Stone counter catalog. IDs188 (Fog), 189 (Illusion) and 194 (money
absorption) are excluded entirely rather than simplified.

| Old ID | Technical label | Element | Effect | Residual multiplier | Hit rate | Target | Old price | Weight |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 187 | legacy-187 | Fire | Damage | 1 | 75% | All alive enemies | 10 | 1 |
| 191 | legacy-191 | Light | Direct Glory | 0 | guaranteed | Incoming source | 10 | 1 |
| 192 | legacy-192 | Dark | Direct Dark Cloud | 0 | guaranteed | Incoming source | 10 | 1 |
| 193 | legacy-193 | Neutral | Source MP recovery | 2 | guaranteed | Self | 10 | 1 |

Every source row is PROTECTOR with defense kind COUNTER, absent value/sub-value
and no defense extra. Runtime `Card` fields attack, boost, defense, HP and MP are
zero, price is 10, weight is 1 and discardable is true. Dedicated membership retains
the complete counter effect, multiplier, hit rate, harm and target semantics.
None consumes MP or creates a retained miracle. Technical names and old IDs are
not verified current official identifiers or names.

The [counter construction][counter] uses positive raw damage remaining after
ordinary defense; multipliers do not use actual HP loss or Dark execution damage.
ID191/192 directly add their harms at [resolution][resolution], without HP damage
or a `hasDamaged` condition. ID192's Dark element therefore does not create a
Dark execution. ID193 increases its counter source player's own MP by twice the
residual, capped at 99; it is not a target MP transfer or an MP-cost miracle.

ID187 [enumerates alive enemies when enqueued][queue]. In a duel with its only
enemy already dead, it creates no entry. Each queued enemy gets one 75% attempt,
not repeated attacks on a single target. At pop the historical hit test occurs
before checking whether that target has since died. Dark Cloud forces a hit and
skips the random roll. These enqueue/pop distinctions affect seeded trajectories.

ID193's original attribute field is empty. [Item.fromData][parser] preserves this
as the empty string, and [defense composition][mix] distinguishes it from the
unselected `None` sentinel. Its normalized element is neutral and participates
in composition; it must not be omitted from attribute mixing merely because it
is neutral.

Unlike counters can change later state and queue survival according to selection
order. Their engine action representation must preserve that ordering. The old
server acceptance path for counter defense remains distinct from old bot intent
(unresolved C05). Neither the data nor that implementation choice proves current
official-client compatibility or exact historical XML trace reproduction.

`source-rows.csv` retains the original zero-based records from [ItemData.CSV][csv].
`catalog.csv` is normalized data. `manifest.json` independently fingerprints both
and pins the thirteen earlier catalog hashes. Immutable raw source was unavailable;
observed master rows are not asserted byte-identical to the recorded history-head
commit. Old weights are not verified official acquisition probabilities; upstream
[reports distribution uncertainty][issues].

```sh
python3 data/legacy-counter-extensions-v1/generate.py
python3 data/legacy-counter-extensions-v1/generate.py --check
```

[counter]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L588-L602
[resolution]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L445-L482
[queue]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L290-L325
[parser]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/item.py#L27-L41
[mix]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L553-L556
[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[issues]: https://github.com/Igoorx/godfield-flash#known-issues
