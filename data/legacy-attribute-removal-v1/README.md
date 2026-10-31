# Legacy attribute-removal armor v1

This separate catalog retains one complete historical `REMOVE_ATTRIBUTE` effect.
The eleven earlier catalogs, fingerprints and miracle indices remain unchanged.

| Old ID | Technical label | Source type | Special defense | ATK / boost / DEF | MP cost | Old price | Weight |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 195 | legacy-195 | PROTECTOR | REMOVE_ATTRIBUTE | 0 / 0 / 0 | none | 15 | 3 |

The source has no attack kind, attack extra, ordinary defense kind, attribute,
value, secondary value or percentage. In particular, the missing defense kind is
not `DFS`. The normalized `Kind::armor` record has zero numeric attack, boost,
defense, HP and MP, and is discardable. Dedicated `AttributeRemovalCard` catalog
membership retains its special effect; it must never be treated as an ordinary
zero-defense armor. It is not a miracle and adds no retained ability slot.

Historical [defenderCommand][turn] consumes each defense piece in sequence. On
ID195 it changes both the local attack clone and the original current attack to
neutral before proceeding with armor element mixing and later special responses.
Thus an already reflected or bounced attack retains the changed attribute when
the remover preceded that response. A special response before the remover stops
the sequence before that remover is used. Defense prefix consumption and other
effects remain separate from attack damage reduction.

The engine may represent selected removers canonically before its selected
special response, preserving meaningful selected-card outcomes. That compressed
representation does not assert reproduction of every original XML command trace
or card ordering. Current official-client compatibility is unverified.

`source-rows.csv` preserves original fields and zero-based IDs from
[ItemData.CSV][csv]. `catalog.csv` is the normalized runtime representation. Old
IDs and technical names are not verified current official identifiers or names.
`manifest.json` fingerprints the source rows and catalog, and pins all eleven
earlier catalog hashes. Immutable raw source content was unavailable: observed
master rows are not asserted byte-identical to the recorded history-head commit.
Historical weights are not verified official acquisition probabilities; upstream
[reports distribution uncertainty][issues].

Regenerate or verify from the repository root:

```sh
python3 data/legacy-attribute-removal-v1/generate.py
python3 data/legacy-attribute-removal-v1/generate.py --check
```

[turn]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L534-L581
[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[issues]: https://github.com/Igoorx/godfield-flash#known-issues
