# Legacy wide miracle v1

This independent catalog adds source ID209, technical label `legacy-209`: neutral
MAGIC with WIDE_ATK, MP5, hitRate100, old price10 and weight1. Its attack kind and
numeric value are empty. It cannot be a base or standalone cast. Local slot0 and
a separate holding count preserve all twenty-one earlier catalogs and indices.

[Source validation][validation] permits the extra only after a guaranteed-hit
WEAPON base. [Ordered processing][turn] pays MP5 each use, sets the massive-attack
flag, chance100 and neutral attribute, then applies normal attribute mixing.
A later setter can restore a color. Retained use may repeat, including after an
earlier hand use learns209 in the same command. Every use contributes a pending
gift. The MP99 cap allows nineteen uses.

With MAGICAL57, every added MAGIC pays first and then resets damage to twice the
remaining MP before its extra is applied. WIDE_ATK leaves that reset amount alone.
With initial MP10, `57 → 209` yields10, then final MAGICAL payment empties MP.
Repeated WIDE uses cannot be collapsed: spending, gifts, neutral reset and the
MAGICAL reset all remain meaningful. These are source deductions, not captured
official-client fixtures.

The source enqueues one clone per alive enemy, not repeated hits. In a duel the
sole alive enemy replaces a supplied self target. At pop, chance100 still consumes
one hit roll before the dead-target check; DarkCloud short-circuits that roll.
Chance100 therefore differs from the guaranteed-hit chance0 sentinel.

The completed attack remains WEAPON-origin. Neutral WIDE can use weapon reactions
20/39 directly; a colored result requires195 neutralization.215 and196 remain
available under their own constraints; MAGIC-only responses do not apply.

`source-rows.csv` retains every original column, including price10; `catalog.csv`
normalizes ID, label, element, MP cost, weight and rate. The deterministic generator
checks both fingerprints, generated C++, header hash, all twenty-one old hashes
and ID disjointness. DOUBLE_ATK214 stays independent and MAGIC_FREE unsupported.

Historical IDs, labels and weights are not verified official identifiers, names
or probabilities. Master was observed2026-10-31. Immutable raw retrieval was
unavailable, so the recorded history-head commit does not establish content identity.

```sh
python3 data/legacy-wide-miracles-v1/generate.py
python3 data/legacy-wide-miracles-v1/generate.py --check
```

[validation]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L78-L94
[turn]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L234-L325
