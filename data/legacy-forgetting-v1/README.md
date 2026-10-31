# Legacy forgetting v1

Source241, technical label `legacy-241`, is the only positive-weight REMOVE_ABILITIES
artifact: SUNDRY, amount1, price10, weight3, discardable. ATK/boost/DEF/HP/MP and
cost are zero; other extras and defenses are absent. Zero-weight CREATURE250 is
excluded. All twenty-three previous catalogs, hashes and local slots remain fixed.

The CSV attribute is blank, but [queueAttack][queue] exits this branch before
assigning the active attribute. Its payload therefore has attributeNone, not NE.
[Defense validation][defense] rejects ordinary defense against None.195 asserts
an initialized attribute. Only first196's special bypass can reflect this item;
a prefix before196 loses that bypass.

[Resolution][effect] chooses one retained copy of the current target; duplicates
are weighted by their multiplicity. An empty target loses nothing and consumes no
selection RNG. It removes only one copy, not every copy of a kind. Self use resolves
immediately; reflection changes the current target before selection.

The source stores an acquisition-ordered list, chooses a position and then removes
the first matching ID. This environment uses canonical ascending type-ID weighted
ranks instead. It matches the probability distribution, not Python seeded list
positions or XML copy indices. `retained_type_ids` lists all21 supported kinds and
preserves family-local indices. Its fixture contract consumes one RNG or scripted
ticket for every nonempty selection, including population1; tickets are modulo the
current total. Population0 consumes none.

Only the attempted source/target and lost ID or empty result are public. Population,
selection tickets and the target's other retained copies are private.

`source-rows.csv` retains every original column; `catalog.csv` stores normalized
Card fields and amount. The generator checks both hashes, generated C++, header
fingerprint, old23hashes/ID disjointness, and the canonical21-type order.
Historical IDs, labels and weights are not verified official identifiers, names
or acquisition probabilities. Master was observed2026-10-31. Immutable raw retrieval
was unavailable, so the history-head commit does not establish snapshot identity.

```sh
python3 data/legacy-forgetting-v1/generate.py
python3 data/legacy-forgetting-v1/generate.py --check
```

[queue]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L157-L162
[effect]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L430-L444
[defense]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/item.py#L128-L147
