# Legacy magic reflection v1

This independent catalog preserves four complete `REFLECT_MAGIC` artifacts.
The fifteen earlier catalogs, fingerprints and miracle indices remain fixed.
Dedicated `MagicReflectionCard` membership retains the special effect.

| Old ID | Technical label | Source type | Ordinary ATK | Added ATK | Ordinary DEF | Old price | Weight |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 40 | legacy-040 | WEAPON | 10 | 0 | 0 | 15 | 1 |
| 171 | legacy-171 | PROTECTOR | 0 | 0 | 9 | 10 | 1 |
| 174 | legacy-174 | PROTECTOR | 0 | 0 | 10 | 10 | 1 |
| 178 | legacy-178 | PROTECTOR | 0 | 0 | 11 | 10 | 1 |

All source attributes are empty and normalize to neutral, including in ordinary
attribute composition. None has attack extras, percentage, harm, disease, MP cost
or miracle retention. ID40 can be a standalone ordinary weapon but cannot be an
attack booster. The three protectors cannot start an ordinary attack; their DFS
values remain available against ordinary weapons and counters when magic
reflection does not activate. Their ordinary effects are not stripped.

Historical [defenderCommand][reaction] activates this reaction only when the
originating attack item is MAGIC. It swaps current attacker and defender, clones
the original current attack, then stops processing the defense sequence before
this artifact's own DFS subtraction. Earlier armor is consumed and any prefix
status effects remain, but earlier damage reductions are discarded by returning
to the original payload. Ordinary WEAPON and COUNTER/PROTECTOR origins do not
activate magic reflection.

For MAGIC origins, the [post-reaction branch][validation] skips the ordinary final
attribute, Dark and counter checks. This applies to Light/Dark damage spells and
utility spells, including HP recovery216 and money increase220. Limiting the
reaction to damaging or neutral miracles would remove part of its behavior.
The branch ends when the new target is finalized dead; it has no independent
finalized-dead-source rejection. Normal self-targeted spells resolve before a
response is offered, so they cannot initially select a reflection defense.

[AttackData.clone][clone] preserves the original pieces, amount/decided value,
chance, element and extra effects. A reflection is not a fresh cast: original
miracle ID, retained provenance and ownership must stay distinguishable from
the current redirected source. It pays no further MP and learns no miracle.
An already-hit percentage spell is not rerolled, because reflection resumes
defender response rather than enqueueing and launching a new attack. After
swapping two distinct participants, the target is still distinct from the new
source, so this is not the immediate self-bounce path.

Universal reflection196 remains excluded: its first-defense-piece exemption
creates a separate ordering concern. Fog and Illusion effects are also excluded.
These historical server rules do not resolve current official compatibility
questions, including the existing Light-defense discrepancy.

`source-rows.csv` preserves original zero-based records from [ItemData.CSV][csv].
`catalog.csv` contains normalized facts. Old IDs and technical labels are not
verified current official identifiers or names. `manifest.json` fingerprints both
and pins all fifteen earlier hashes. Immutable raw source was unavailable;
observed master rows are not asserted byte-identical to the recorded history-head
commit. Historical weights are not verified official acquisition probabilities;
upstream [reports distribution uncertainty][issues].

```sh
python3 data/legacy-magic-reflection-v1/generate.py
python3 data/legacy-magic-reflection-v1/generate.py --check
```

[reaction]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L534-L581
[validation]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L630-L642
[clone]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L58-L77
[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[issues]: https://github.com/Igoorx/godfield-flash#known-issues
