# Legacy money counter v1

This independent catalog adds old ID194 without modifying any of the fourteen
earlier catalogs, fingerprints or miracle indices. Dedicated membership retains
the full `ABSORB_YEN` effect, including its resource-shortage behavior.

| Old ID | Technical label | Source type | Attack kind | Defense kind | Element | Multiplier | Old price | Weight |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 194 | legacy-194 | PROTECTOR | ABSORB_YEN | COUNTER | Neutral | 1 | 10 | 1 |

The source has no attack extra, defense extra, attribute, value, secondary value
or percentage. The normalized armor has attack, boost, defense, HP and MP all
zero, price 10, weight 1 and discardable true. There is no MP cost, harm, disease
or retained miracle slot. The empty source attribute normalizes to neutral and
participates in defense composition rather than being omitted.

[Counter construction][counter] targets the current incoming attacker and sets
the requested amount to positive raw incoming damage remaining after ordinary
defense, multiplied by one. It does not use actual HP loss or Dark execution
damage. The later money-counter response cannot subtract ordinary defense from
this already decided payment amount.

[Resolution][resolution] first credits the counter source with the full requested
amount, capped at 99. It then charges the target that amount. [Player.decreaseYen][payment]
pays from money first, then MP, then HP, with a floor of zero for each resource.
Credit is not restricted to the target's available money or actual money loss.
This is not a conserved money-only transfer.

A finalized-dead counter source can still receive that credit if the queued
target remains alive. Payment-related HP loss is a direct resource shortage,
not positive attack damage, and therefore does not generate another counter.

For example, with an amount of 10, source money 20 and target money/MP/HP of
3/4/40, the result is source money 30 and target resources 0/0/37. A source already
at 98 receives only one additional money unit, while the target still pays the
full amount. These are source-code examples, not fixtures from the official UI.

Unlike-counter ordering remains significant. This catalog uses the existing
ordered counter representation; it adds no simplified unordered substitute.
Old server counter-defense acceptance remains distinct from old bot intent
(unresolved C05). Current official-client fidelity and exact historical XML trace
reproduction are not asserted. Fog counter188 and Illusion counter189 remain
excluded completely.

Original fields and zero-based IDs from [ItemData.CSV][csv] are retained in
`source-rows.csv`. `catalog.csv` stores normalized facts. Technical labels and old
IDs are not verified current official names or identifiers. `manifest.json`
fingerprints both files and pins fourteen earlier catalog hashes. Immutable raw
source was unavailable, so observed master rows are not asserted byte-identical
to the recorded history-head revision. Historical weights are not verified
official acquisition probabilities; upstream [reports distribution uncertainty][issues].

```sh
python3 data/legacy-money-counter-v1/generate.py
python3 data/legacy-money-counter-v1/generate.py --check
```

[counter]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L588-L602
[resolution]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L457-L461
[payment]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L101-L121
[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[issues]: https://github.com/Igoorx/godfield-flash#known-issues
