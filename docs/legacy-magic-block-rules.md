# 旧版の魔法遮断8種

調査日: 2026-10-31。対象はIgoorxの非公式Flashサーバーエミュレータ。
v19は正の出現重みを持つBLOCK_MAGIC8種を追加し、公式完全再現とは区別する。
追加MAGIC、MAGIC_FREE、未対応の状態異常などは今回の範囲へ混ぜない。

## 正確な台帳範囲

[旧CSV][csv]は以下の8種を持つ。全NE・命中率0・重み1。
MP費用や奇跡保持化はなく、病気/harmsの追加効果もない。

| ID | 通常の役割 | defenseExtra | 価格 |
| --- | --- | --- | --- |
| 27 | WEAPON ATK7 | BLOCK_MAGIC | 15 |
| 54 | WEAPON ATK15 | BLOCK_MAGIC | 10 |
| 55 | WEAPON ATK15、INCREASE_ATK15 | BLOCK_MAGIC | 10 |
| 152 | PROTECTOR DFS6 | BLOCK_MAGIC | 10 |
| 165 | PROTECTOR DFS8 | BLOCK_MAGIC | 10 |
| 182 | PROTECTOR DFS13 | BLOCK_MAGIC | 15 |
| 183 | PROTECTOR DFS14 | BLOCK_MAGIC | 15 |
| 185 | PROTECTOR DFS15 | BLOCK_MAGIC | 15 |

重み0の312/313/363は通常プールへ入れない。55の攻撃追加値15は既存の普通boostと
同じ加算・属性合成に従い、遮断能力を追加する理由で失わせない。

## MAGIC入力では強制発火

[defenderCommand][defense]はincoming先頭神器のtypeがMAGICならBLOCK_MAGICを発火し、
breakする。自身のDFS処理より前なので、印字DEFはその魔法に適用されない。
普通防具として使い、遮断だけを任意に省くサーバー経路はない。

APIでは8種をMAGIC相手に使うときdefend.baseへ指定し、普通slotsへの選択を拒否する。
％魔法は既に命中して応答を求められた場合に限って遮断できる。
LIGHT/DARKや攻撃魔法のほか、216回復・220入金など任意のMAGIC payloadを止める。
自己対象魔法は空防御で自動解決するので、遮断神器を選ぶ応答機会がない。

[特殊後の検査][check]はincoming MAGICなら属性・DARK・counter確認をskipする。
前置防具の属性適合は不要だが、すでに実行した消費・配布予定、175などの即時状態変化は残る。
遮断時はinflictDamageへ進まず、prefixにcounterがあっても予約しない。
本profileで既存採用するGlory1枚制限は適用する。

## 非MAGICでは通常の役割を維持

5防具は通常DFSとして使える。適合するcounter入力にも普通DFSの規則を適用する。
3武器には通常防御値がなく、非MAGIC相手に遮断baseとして使えない。
MAGICAL57はWEAPON、counterはPROTECTORであり、どちらもMAGIC遮断の対象外。
対象分類を単にEffect.damageかどうかで判断しない。

## 費用・状態・item終了

[攻撃側の消費][use]は魔法が届く前に終わっている。遮断しても攻撃者のMP支払、
初使用の保持化、使用による配布予定を取り消さない。
遮断者は実際に使った手札品の数だけ消費と配布予定+1を受け、追加のMPや保持は発生しない。
反射・跳ね返しと異なりsource/targetを交換せず、再応答や再命中抽選も行わない。

blockedで現在のitemを終了した後は[Roomの通常処理][finish]へ戻る。
死亡確定、残FIFO、全queue終了後のowner病気tickと配布の順序を維持する。
前置175でHeavenから暫定HP0になった場合、遮断は回復を生まないためそのitem終了で
死亡確定し得る。後ろのFIFOを消して通常手番を即開始する実装にはしない。

現在の16byte Actionで、base=遮断品、slots=前置防具、counter_order=その中のcounter順を
表せる。遮断で抑止するcounterにも選択順の正しい列挙を要求する既存契約を保つ。
遮断発火後の未使用suffixを選んだwire表現は、既存の意味Actionの範囲外。

## 受入条件

8種の通常配布到達、MAGIC時のbase必須とslots opt-out拒否、通常DFS/武器/55boostの維持、
LIGHT/DARK・％・回復/入金の遮断、MP/保持費用を戻さないこと、prefixの属性免除・
状態変化/消費維持・counter非予約、57/counter分類、Glory/Heaven/FIFOを確認する。
旧18profileの固定観測寸法とAction layoutも保護する。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[defense]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L534-L603
[check]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L630-L643
[use]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L110-L139
[finish]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py#L426-L496
