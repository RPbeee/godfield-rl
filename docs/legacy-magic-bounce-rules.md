# 旧版FLICK_MAGICの物理神器6種

調査日: 2026-11-01。Igoorx非公式Flashサーバー公開masterの一次資料を対象とする。
v34は21/119/121/127/134/140に限定し、現行公式完全再現とは区別する。

## 台帳と通常role

全6種ともNE・hitRate0・weight1・MP費用0、defenseExtra=FLICK_MAGIC。
[CSV][csv]のその他の効果は次のとおり。MAGIC210と違い、使用しても保持化しない。

| ID | type | 通常攻撃/追加 | 通常DEF | 価格 |
| --- | --- | --- | --- | --- |
| 21 | WEAPON | ATK5 / INCREASE_ATK5 | なし | 10 |
| 119 | PROTECTOR | なし | 0 | 5 |
| 121 | PROTECTOR | なし | 1 | 5 |
| 127 | PROTECTOR | なし | 2 | 5 |
| 134 | PROTECTOR | なし | 3 | 5 |
| 140 | PROTECTOR | なし | 4 | 5 |

[getAtk][item]と[queueAttack][queue]に従い、21を基底にするとATK5となる。
INCREASE_ATK分岐で0から5を加算した後、通常ATK初期化は未初期化時だけなので10に二重加算しない。
追加では保証命中WEAPONへboost5/NE属性を合成する。197/217/214/209/FREE等との順序は既存規則。
攻撃使用で21の防御extraをincoming攻撃の跳ね返し能力として継承しない。
非MAGICに対する5防具は通常DEFとして使い、119の0DEFも合法。21には通常DEF用途がない。

## MAGICへの強制跳ね返し

[defenderCommand][defense]は元先頭神器のtypeがMAGICならFLICK_MAGICを発火する。
発火は自身のDFS減算より前でbreakするため、140のDEF4等を魔法軽減に使う分岐はない。
MAGIC対象では該当品をDEFEND.baseへ置き、普通slotsへ入れて反応を省くことを許さない。
同じ列に複数のmatching特殊があれば最初の特殊が発火し、後続は未消費。

先行する普通防具は消費・giftを生じるが、発火後は元currentAttackのcloneへ戻るため、
prefixによる一時的な威力減算は消える。先行statusの実適用は残り、195は元の属性もNEへ変更するので残る。
MAGICなら最終の属性/DARK/counter検査を全skipするため、prefixの属性適合は要求しない。
Light・Dark・命中済み％・216回復・220入金等も型MAGICなら同じ反応となる。
元typeを変えずにpayloadを維持するので、WEAPON57/209合成武器やPROTECTOR counterには発火しない。
Noneを持つ241/240/SELL/BUY/237はSUNDRY/TRADEであり、6種で防御できることにはならない。

## 対象抽選・反応chain・暫定HP0

[getRandomAlive()][room]は引数なしで全not.deadの人物から一様choiceする。
元の攻撃者と今回の防御者本人を含み、HP0でもdead=falseなら候補となる。
sourceを今回の防御者にし、targetだけをこの抽選で決める。turn ownerと元MAGIC来歴は維持する。
targetがsource本人なら追加応答なしで同じ効果を即適用し、それ以外は新targetへ応答を求める。
[clone][clone]は元payloadを保持し、[doAttack][attack]へ戻らないので命中済み％を再抽選しない。
元MP費用・初回保持取得・元使用者のgiftも再実行しない。

175をprefixとしてHeavenからHP0になった防御者も、dead確定前に6種を使える。
その本人へ216が飛べばHP0から回復でき、別人へ飛べばHP0sourceのまま反応chainを続ける。
後から反射/跳ね返しでHP0本人へ戻る救済も既存の暫定HP0契約に従う。
反応chainの途中に新たな死亡確定・owner病気tick・通常giftを挟まない。

## 消費・FREE・Glory

[usePiece][player]は6種を手札から直接消し、MPを支払わず、保持リストへ追加しない。
実際に使うprefix各枚と跳ね返し品にgift+1ずつ。反応後の余分な配布や保持giftはない。
通常の終局・244・owner病気・gift順序は既存のまま。

特殊直後のFREE1枚は事前走査対象でも、特殊のbreak後なので実消費しない。
物理6種自体のMP費用が0なのでsaved MPも0であり、元攻撃MAGICの支払いを無料にはしない。
kind13の既存canonical未消費suffixを使い、さらに余るsuffixのXML入力再現は対象外。
Gloryは[既存profileの選択枚数1枚契約](legacy-harms-rules.md)を維持し、
普通prefix＋特殊や特殊＋未消費FREEの2枚選択を許さない。
これは旧server単体の手動入力検証で必ず拒否されるとの主張ではない。

## v34と受入条件

新しい行動kind・effect・tapeは不要。既存script_bounce_targetsを物理6種にも共用する。
公知の使用カードと実targetをLastReactionへ保存し、from_ability=false/free_card0とする。
未来の抽選targetは観測へ出さない。旧33profileは新flag=falseとし、価格表やRNG経路を維持する。

受入条件は全6種の通常pool到達、21基底/boost、5防具の非MAGIC DEF、MAGICでの強制発火と
ordinary slotsのopt-out拒否、self/相手target、prefix消費とraw保持、195、％再抽選なし、
utility・MP0・Glory・未消費FREE・暫定HP0救済、固定ABI幅・バッチ原子的拒否を含む。

候補調査でSCATTER_YEN/ADD_ITEMは通常ItemData.CSVの正重量カードに見つからず、
汎用handlerだけを根拠に今回の神器として増やさない。助手・MYSTERY・霧/幻覚は別scopeとする。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[item]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/item.py#L111-L126
[queue]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L226-L286
[defense]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L510-L643
[room]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py#L92-L101
[clone]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L58-L77
[attack]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L317-L380
[player]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L276-L301
