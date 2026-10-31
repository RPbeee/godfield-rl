# Legacy universal reflection v1

This independent catalog preserves old ID196's complete `REFLECT_ANY` effect.
All sixteen earlier catalogs, fingerprints and miracle indices remain fixed.

| Old ID | Technical label | Source type | Special defense | ATK / boost / DEF | Element | Old price | Weight |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 196 | legacy-196 | PROTECTOR | REFLECT_ANY | 0 / 0 / 0 | Neutral | 10 | 1 |

The original attack kind, attack extra, defense kind, attribute, value,
sub-value and percentage fields are empty. In particular there is no ordinary
DFS effect when reflection does not activate. Dedicated `AnyReflectionCard`
membership preserves its special behavior; it must not become plain zero-defense
armor. The card cannot start an ordinary attack or supply added ATK. It is
discardable and has no MP cost, retained miracle, other harm or disease.

Historical [defenderCommand][reaction] accepts `REFLECT_ANY` regardless of the
originating item type, swaps current attacker and defender, clones the original
current attack and stops the defense sequence. Amount, element, chance, status
extras, original artifact ID and retention provenance remain. An already-hit
percentage item is not rerolled. Earlier armor reductions are discarded, while
prefix consumption and status effects remain. The empty source attribute
participates as neutral in the ordinary defense-attribute composition.

The [final exemption][validation] depends on the **first submitted defense piece**:

| Defense sequence | Final validation after reflection |
| --- | --- |
| ID196 first | Skip ordinary attribute, Dark and COUNTER checks |
| Earlier prefix, incoming MAGIC | Skip those checks because of MAGIC origin |
| Earlier prefix, incoming non-MAGIC | Reject Dark or COUNTER origin; require compatible final composed defense attribute |

ID196 used first breaks before later submitted pieces are consumed. An earlier
prefix instead makes ID196's neutral attribute participate in the final mixture.
ID195 before it can make an ordinary weapon attack neutral, but cannot remove
the attack's COUNTER origin. A canonical prefix-plus-special representation must
preserve these different legal outcomes rather than always granting the first-card
exemption to any use of ID196. Exact original XML trace reproduction is not asserted.

This reaction applies only when an incoming item offers a response. Initial
self-targeted spells and automatically resolved self-MP counters expose no
reflection choice. The old branch ends if the new target is finalized dead,
without independently rejecting a dead source. For a reflected money counter,
the credited party follows the current redirected source; immutable original
artifact ownership is a separate fact. Current official-client fidelity remains
unverified, and Fog and Illusion are still excluded.

`source-rows.csv` retains original zero-based data from [ItemData.CSV][csv];
`catalog.csv` normalizes those facts. Old IDs and technical labels are not verified
current official identifiers or names. `manifest.json` fingerprints both files
and pins sixteen earlier catalog hashes. Immutable raw source was unavailable;
observed master rows are not asserted byte-identical to the recorded history-head
revision. Historical weights are not verified official probabilities; upstream
[reports distribution uncertainty][issues].

```sh
python3 data/legacy-any-reflection-v1/generate.py
python3 data/legacy-any-reflection-v1/generate.py --check
```

[reaction]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L534-L581
[validation]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L630-L642
[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[issues]: https://github.com/Igoorx/godfield-flash#known-issues
