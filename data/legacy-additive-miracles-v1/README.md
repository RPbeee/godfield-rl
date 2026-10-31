# Legacy additive miracles v1

This independent catalog adds two complete ATK/INCREASE_ATK miracles. All nineteen
earlier catalogs, fingerprints and miracle indices remain fixed. Local indices
0/1 identify 197/217 without renumbering previous utility, offensive or defensive
abilities. Unsupported lookup returns nullptr, and unsupported index returns -1.

| Old ID | Technical label | Element | Standalone ATK / added ATK | MP per use | Old price | Weight |
| --- | --- | --- | --- | --- | --- | --- |
| 197 | legacy-197 | Fire | 2 / 2 | 2 | 10 | 1 |
| 217 | legacy-217 | Light | 10 / 10 | 7 | 15 | 1 |

Both source rows are MAGIC with attack kind ATK and attack extra INCREASE_ATK.
Neither has ordinary defense, special defense, percentage, disease or harm.
Source prices remain in `source-rows.csv`, although the runtime type stores only
the effect amount, element, MP cost and weight.

[AttackData.isValidAttackItem][validator] permits standalone MAGIC use and permits
these extras after a WEAPON base whose hit rate is zero. They cannot augment a
percentage weapon or another MAGIC base. Ordinary element composition includes
their Fire/Light attributes; ordering with supported attribute setters must remain
expressible. A standalone INCREASE_ATK miracle contributes its amount once,
rather than doubling it merely because both ATK roles are present.

[Player.usePiece and useMagic][retention] consume and retain a first hand use,
charge each use's MP cost, and permit repeated retained uses without consuming
the ability. [Command processing][command] adds a pending gift for each use.
The command representation must preserve repeated held additions rather than
representing them as a single unordered bit. MP spending and learning occur before
the incoming effect can be defended or blocked; a later response does not undo
them.

The existing MAGICAL weapon 57 makes order especially significant. After an added
MAGIC pays its MP cost, [queueAttack][command] resets damage to twice the remaining
MP, discarding earlier boosts, then adds that miracle's amount. For example, with
initial MP 10, `57 → ordinary +3 → 197` yields 18, while `57 → 197 → ordinary +3`
yields 21. `57 → 197 → 197` yields 14: the last remaining MP 6 gives 12, then the last
197 adds 2. The final MAGICAL payment empties remaining MP. These are source-code
examples, not fixtures captured from the official client.

MAGIC_FREE, WIDE_ATK209 and DOUBLE_ATK214 remain excluded completely. Their waiver,
multi-target and multiplier behavior is not simplified into these two records.
The old first 19 profiles remain unchanged; current official fidelity and exact
historical XML trace reproduction are not asserted.

`source-rows.csv` preserves original zero-based records from [ItemData.CSV][csv],
while `catalog.csv` stores normalized facts. Old IDs and technical labels are not
verified current official names or identifiers. `manifest.json` fingerprints both
and pins nineteen earlier catalog hashes. Immutable raw source was unavailable;
observed master rows are not asserted byte-identical to the recorded history-head
revision. Historical weights are not verified official probabilities; upstream
[reports distribution uncertainty][issues].

```sh
python3 data/legacy-additive-miracles-v1/generate.py
python3 data/legacy-additive-miracles-v1/generate.py --check
```

[validator]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L78-L94
[retention]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L276-L301
[command]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L110-L136
[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[issues]: https://github.com/Igoorx/godfield-flash#known-issues
