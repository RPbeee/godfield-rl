# Legacy item removal v1

Source240, technical label `legacy-240`, is the only positive-weight REMOVE_ITEMS
artifact: SUNDRY, amount1, price10, weight3, discardable. Ordinary ATK/boost/DEF/HP/MP,
MP cost and all extras are0. Zero-weight CREATURE249 is excluded. All twenty-six
older catalogs, hashes, Kind values and indices remain fixed.

[Queue processing][queue] consumes240 and earns its source gift, then exits this
branch before assigning an active attribute. The payload is None, not ordinary NE.
[Defense validation][defense] rejects ordinary defense/195; first196 can use its
special bypass, while prefix armor before196 cannot.

[Resolution][effect] chooses one copy uniformly from the current target hand and
calls discardPiece directly. The removed target card earns no replacement gift.
Even ordinary non-discardable artifacts can be removed this way; unplayed MAGICs
are not learned, and retained abilities are unaffected. Self use first consumes
the source240, so that copy is not part of its own selection population.

[Random selection][selection] returns empty without RNG for an empty hand. The
explicit hand order already represents duplicate copies. The public event is the
source/target and removed ID or empty result; slot, population, ticket and other
hidden hand contents are not policy observations. End-of-turn pending gifts can
refill a hand later in the same step and must not be confused with compensation
for the removed copy.

`source-rows.csv` retains every original column; `catalog.csv` normalizes Card
fields and amount. The generator verifies hashes, generated C++, header fingerprint,
old26hashes and ID disjointness. IDs/labels/weights are not verified current official
identifiers, names or acquisition probabilities. Master was observed2026-10-31;
immutable retrieval was unavailable, so history-head does not prove content identity.

```sh
python3 data/legacy-item-removal-v1/generate.py
python3 data/legacy-item-removal-v1/generate.py --check
```

[queue]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L126-L159
[effect]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L423-L429
[selection]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L238-L254
[defense]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/item.py#L128-L147
