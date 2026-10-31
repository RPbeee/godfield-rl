# Legacy doubling miracle v1

This independent catalog adds source ID214, technical label `legacy-214`: a neutral
MAGIC with DOUBLE_ATK, MP6, old price10 and weight1. Its attack kind and numeric
value are empty: the effect doubles an already initialized attack; it cannot be
used as a standalone attack. Local slot0 preserves every prior ability index.

[Source validation][validation] accepts it only after a guaranteed-hit WEAPON base.
[Ordered processing][turn] pays MP6 on every use and doubles at that position;
normal attribute mixing then includes its neutral attribute. Retained uses can
repeat, including after an earlier hand use learns the ability in the same command.
The MP99 cap permits at most16 uses. A later setter can restore a colored attack.

MAGICAL57 first resets damage to twice the remaining MP after each added MAGIC
payment, before applying that extra. With initial MP10, `57 → 214` yields16 and
empties MP: payment leaves4, reset gives8, doubling gives16. Conservative raw and
32-counter limits are `480 <<16` and `480 <<48`; MAGICAL57 itself reaches at most597.
These are source-code deductions, not official-client fixtures.

WIDE_ATK209 and MAGIC_FREE remain unsupported. All twenty earlier catalog hashes
are pinned and checked for ID collisions. The source price is retained in
`source-rows.csv`; runtime fields are ID, label, element, MP cost and weight.
`catalog.csv` stores normalized facts, with its SHA256 in the manifest/header.

Historical IDs, labels and acquisition weights are not verified current official
identifiers, names or probabilities. Master rows were observed2026-10-31; immutable
raw content was unavailable, so the history-head commit is provenance context,
not proof that its content matches this snapshot.

```sh
python3 data/legacy-doubling-miracles-v1/generate.py
python3 data/legacy-doubling-miracles-v1/generate.py --check
```

[validation]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L78-L94
[turn]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L226-L286
