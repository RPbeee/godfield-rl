# Legacy mana weapon v1

This independent catalog adds only old ID57, preserving its complete `MAGICAL`
base-weapon effect. All seventeen earlier catalogs, fingerprints and miracle
indices remain fixed. Magic-block artifacts and additional magic remain excluded.

| Old ID | Technical label | Source type | Attack kind / extra | Fixed ATK / boost / DEF | Element | MP multiplier | Old price | Weight |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 57 | legacy-057 | WEAPON | ATK / MAGICAL | 0 / 0 / 0 | Neutral | 2 | 10 | 1 |

The source has no fixed value, secondary value, defense kind, defense extra,
attribute or percentage. Runtime `Card.attack`, boost, defense, HP and MP fields
are zero; dedicated `ManaWeaponCard` membership and its multiplier retain the
dynamic effect. `Card.mp = 0` means no static recovery/cost field, not a free
positive-strength attack. Price is 10, weight is 1 and discardable follows the
existing weapon profile rule (false). There is no miracle retention or minimum
required MP; zero MP is valid.

Historical [queueAttack][effect] sets base damage to twice the source's current
MP and [then sets MP to zero][payment] before response. Ordinary supported
INCREASE_ATK pieces add their values and participate in ordinary element mixing;
supported ADD_ATTRIBUTE pieces retain their selected overwrite outcome. Thus
zero MP yields zero base damage, but ordinary boosts can still add damage.

[AttackData.isValidAttackItem][validator] does not allow MAGICAL as an added
component, so ID57 can only be a weapon base. It remains WEAPON-origin even
though its name refers to magic: weapon reactions and universal reflection apply
according to the existing conditions, while MAGIC-only reflection/bounce/block
do not apply. Response and reflection do not spend the original MP again.

This restricted pool has no additional magic197/217, MAGIC_FREE, DOUBLE_ATK or
WIDE_ATK. If those are added later, the source's MP-payment/recalculation and
ordering behavior must be preserved rather than extending this base-only formula
without further work. Assistant-generated MAGICAL effects are outside the pool.

With MP capped at 99 and at most 15 ordinary extras of ATK15, the current damage
bound is `99 * 2 + 15 * 15 = 423`, below the existing 480 base bound used for
counter overflow limits. This bound applies to the selected profile, not every
possible historical command extension.

`source-rows.csv` preserves the zero-based record from [ItemData.CSV][csv], while
`catalog.csv` holds normalized data. Names are technical labels, not verified
current official names or IDs. `manifest.json` fingerprints both files and pins
all seventeen earlier catalog hashes. Immutable raw source was unavailable;
observed master rows are not asserted byte-identical to the recorded history-head
revision. Historical weights are not verified official acquisition probabilities;
upstream [reports distribution uncertainty][issues]. Current official-client
fidelity and exact historical XML trace reproduction are not asserted.

```sh
python3 data/legacy-mana-weapon-v1/generate.py
python3 data/legacy-mana-weapon-v1/generate.py --check
```

[effect]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L240-L248
[payment]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L288-L289
[validator]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L78-L94
[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[issues]: https://github.com/Igoorx/godfield-flash#known-issues
