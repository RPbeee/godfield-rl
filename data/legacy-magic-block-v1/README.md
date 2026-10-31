# Legacy magic block v1

This independent catalog adds eight complete positive-weight `BLOCK_MAGIC`
artifacts. All eighteen earlier catalogs, fingerprints and miracle indices remain
unchanged. Dedicated `MagicBlockCard` membership retains the special effect.

| Old ID | Technical label | Source type | ATK | Added ATK | DEF | Old price | Weight |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 27 | legacy-027 | WEAPON | 7 | 0 | 0 | 15 | 1 |
| 54 | legacy-054 | WEAPON | 15 | 0 | 0 | 10 | 1 |
| 55 | legacy-055 | WEAPON | 15 | 15 | 0 | 10 | 1 |
| 152 | legacy-152 | PROTECTOR | 0 | 0 | 6 | 10 | 1 |
| 165 | legacy-165 | PROTECTOR | 0 | 0 | 8 | 10 | 1 |
| 182 | legacy-182 | PROTECTOR | 0 | 0 | 13 | 15 | 1 |
| 183 | legacy-183 | PROTECTOR | 0 | 0 | 14 | 15 | 1 |
| 185 | legacy-185 | PROTECTOR | 0 | 0 | 15 | 15 | 1 |

All original attributes are empty, normalize to neutral and participate in
ordinary attribute composition. No member has a percentage, harm, disease, MP
cost or retained miracle. ID55's INCREASE_ATK is preserved together with its
standalone attack and special defense. The other weapons cannot act as attack
boosters. Protectors cannot start ordinary attacks. Discardable follows the
existing profile convention: false for weapons and true for armor.

Historical [defenderCommand][reaction] checks BLOCK_MAGIC before the selected
artifact's own DFS subtraction. If the originating item is MAGIC, using the
artifact **must activate the special block**; treating the same selected artifact
as ordinary DFS without blocking would invent another outcome. Earlier defense
pieces are consumed and their status effects remain, but the incoming effect is
blocked. It applies no damage, HP recovery or money increase and creates no
counter from that blocked effect. The original caster's earlier MP payment and
first-use miracle retention are not refunded or undone.

The [special branch][validation] skips final ordinary attribute, Dark and counter
checks for MAGIC origin. Thus utility, Light/Dark and already-hit percentage
miracles can all be blocked; no second hit roll occurs. Initial self-resolving
spells offer no response and therefore no block choice.

Against non-MAGIC origins, BLOCK_MAGIC does not activate. The weapons keep their
ordinary attacks, ID55 keeps added ATK15, and the protectors keep their ordinary
DFS6/8/13/14/15, including applicable defenses against counter attacks. MAGICAL
weapon57 remains WEAPON-origin and is not a magic-block target. These ordinary
roles must not be stripped to simplify the special behavior. Weight-zero
duplicates312,313 and363 are excluded entirely.

`source-rows.csv` retains original zero-based records from [ItemData.CSV][csv];
`catalog.csv` stores normalized values. `manifest.json` fingerprints both and pins
all eighteen earlier catalog hashes. Technical labels and old IDs are not verified
current official names or identifiers. Immutable raw source was unavailable;
observed master rows are not asserted byte-identical to the recorded history-head
revision. Historical weights are not verified official acquisition probabilities;
upstream [reports distribution uncertainty][issues]. Exact original XML traces
and current official-client fidelity are not asserted.

```sh
python3 data/legacy-magic-block-v1/generate.py
python3 data/legacy-magic-block-v1/generate.py --check
```

[reaction]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L534-L581
[validation]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L630-L637
[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[issues]: https://github.com/Igoorx/godfield-flash#known-issues
