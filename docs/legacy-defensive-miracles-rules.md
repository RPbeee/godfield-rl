# 旧版の防御奇跡と魔法反応

調査日: 2026-10-31。Igoorxの非公式Flashサーバーエミュレーターをwebで読んだ結果。
現行公式の実測仕様ではない。参照先の`master`は可変なので、実装台帳には取得物とhashを残す。
共通の連鎖は[反射メモ](legacy-reflection-rules.md)、暫定HP0は[病気メモ](legacy-disease-rules.md)、
元の攻撃奇跡は[攻撃奇跡メモ](legacy-offensive-miracles-rules.md)を参照。

## 最初に追加する範囲

[旧CSV][csv]でMAGICの防御専用候補は次の2種。いずれもattackKind・attackExtra・defenseKindは空、
属性は無属性、value・hitRateは空から0、MPはsubValue列である。[Item.fromData][item]参照。

| 旧ID | defenseExtra | MP | 価格 | 重み | 作用する元の先頭神器 |
| --- | --- | --- | --- | --- | --- |
| 210 | FLICK_MAGIC | 5 | 15 | 1 | MAGIC |
| 215 | BLOCK_WEAPON | 6 | 15 | 1 | WEAPON。ただし属性の追加検査あり |

2種だけを新profileへ追加する案を推奨する。210は魔法反射ではなくランダム跳ね返し、215は遮断。
魔法を反射するMAGICの候補をCSVから捏造しない。
物理神器のREFLECT_MAGIC・FLICK_MAGIC・BLOCK_MAGIC、REFLECT_ANY、属性除去195、
counter、防御時無消費化、倍率・全体化等は同時に追加しなくてもこの2種を再現できる。

物理の魔法反応にも実データはある。例えば40はWEAPON/REFLECT_MAGIC、
119はPROTECTOR/FLICK_MAGIC、171はPROTECTOR/REFLECT_MAGIC。
これらは攻撃値・通常防御値・追加ATK等の役割も異なるため、今回の2種と同じ台帳レコードとして代用しない。

## 初使用と保持からの応答

[defenderCommand][defense]は反応列の各要素について、保持参照なら`useMagic`、
手札なら`usePiece`を呼び、配布予定を1増やす。その後に特殊効果を判定する。
[CommandPiece][piece]が`isAbility`を区別するため、防御奇跡も初使用・保持再使用の両方が可能。

| 使用経路 | 手札 | 保持 | MP | 配布予定 |
| --- | --- | --- | --- | --- |
| 手札210/215の応答 | 1枚消費 | その種類+1 | 5または6消費 | +1 |
| 保持210/215の応答 | 消費なし | 不変 | 同じコスト消費 | +1 |
| MP不足・未所有 | 使用確認で拒否 | 増加しない | 支払わない | 増加しない |

保持化・消費の具体順序は[Player.usePiece/useMagic][use]に従い、既存の奇跡と共通。
初使用で同じ種類を既に保持していても重複数が増える。再使用で所持数は減らない。
使用直後には配布しない。連鎖終了後に各座席の配布予定を精算する。
攻撃者が元の奇跡へ支払ったMP・初回取得を反応時に再実行したり、返金したりしない。

210/215は通常手番の先頭としては[AttackData.isValidAttackItem][valid]のattackKind条件を満たさない。
防御奇跡をmainで空撃ちして保持化してよいという根拠はない。
手札からの通常DISCARDは可能だが、捨てると保持化・MP消費・配布増分は発生しない。
保持側の通常DISCARDは今回追加しない。

## WEAPON/MAGICの区別と属性

[defenderCommandの特殊分岐][defense]は元の先頭神器のtypeで発火対象を決める。
威力が正かや、手札から使ったか保持から使ったかだけでは判定しない。

| 反応 | WEAPONへ | MAGICへ | 末尾の属性条件 |
| --- | --- | --- | --- |
| REFLECT_WEAPON / FLICK_WEAPON / BLOCK_WEAPON | 発火 | 発火しない | 闇禁止、counter由来禁止、最終防御属性が適合 |
| REFLECT_MAGIC / FLICK_MAGIC / BLOCK_MAGIC | 発火しない | 発火 | 元がMAGICなら武器用属性検査を省略 |
| REFLECT_ANY | typeを問わず発火 | typeを問わず発火 | 選択列の先頭がREFLECT_ANYなら属性検査を省略 |

したがって215単独は今回の無属性WEAPONへ使える。火水木土・光には無属性が適合せず、闇は明示的に禁止。
命中済みの確率WEAPONもtypeは武器だが、今回の台帳の確率武器は有属性なので215単独では使えない。
無属性確率武器が将来存在すれば、chanceだけを理由に拒否する分岐ではない。
発火条件はtype=WEAPONであり、attackKind=ATKやhitRate=0を追加要求する旧コードではない。

210は光・闇・基本4属性のMAGICを跳ね返せる。命中済みの確率奇跡も対象で、命中は再抽選しない。
MAGIC216のHP回復、220の所持金増加も対象。damage限定にすると既存プールで到達する効果を失う。
同じ回復でもSUNDRY由来ならMAGICではないので210の対象にはならない。

最初の命中判定でmissなら反応機会はない。自己対象の通常奇跡は空防御で即解決されるため、
自分の通常使用に210を割り込ませる経路もない。

## 通常防具の前置、特殊1枚、Glory

応答列は順番に消費する。通常DFSの病気付与・防御減算を処理し、最初の特殊効果でループを抜ける。
後ろの選択は使われない。最初の実装は「通常防具prefix＋特殊1枚」に限定できる。
既存のprefix maskと特別なbaseを保ち、複数特殊を無順序のmaskで全部消費しない。

跳ね返し・反射は未減算の元の攻撃本体へ戻るので、前置防具の防御力は返す威力に残らない。
前置防具の消費・配布予定・病気などの副作用は残る。
遮断は元の効果を適用せず解決を終えるため、prefixの副作用以外の被弾効果も発生しない。

旧手動入力のサーバーはGloryによる単枚制限を検証しない一方、botは候補を1枚へ絞る。
v7以降と同じ意図規則を採用する場合、保持からの210/215も使用神器1枚として数える。
「手札消費0だからGloryでも防具prefixを追加できる」としない。
この差は[harmsメモ](legacy-harms-rules.md)と同じく旧サーバー完全再現との相違として残す。

旧サーバーは回復への応答でも通常DFSを先に処理してから210へ進める。
今回は非damageにもprefixを許す方向とする。通常DFSだけを使い、特殊を使わない応答も可能。
最終属性の適合条件はあり、現在のHP/MP/所持金増加・解除は無属性なので全属性の通常防具が適合する。

非ATK行動の`atkData.damage`は初期値-1のままで、DFS減算で0へ変わる。
しかし[inflictDamageのHP/MP/所持金処理][utility]は元の神器の`item.value`を使い、
この減算後の`damage`を回復量には使わない。したがって、防具の消費・配布予定・防御時病気は起きても、
回復・所持金増加の量は減らず、解除効果も消えない。
MP回復234のHeaven付与も、MP増加に続く独立の処理なので通常防具では防げない。
例えばHP+10へDEF30を送ってもHP+10であり、回復0へ変換しない。

## 跳ね返し先、再応答、暫定HP0

210は現在の防御者を新しいsourceとし、[Room.getRandomAlive][room]で全生存者からtargetを選ぶ。
1対1では本人も相手も候補。ここで生存はHPではなく死亡確定フラグで判定する。
他者ならその人へ応答を求め、本人なら即時に元の効果を解決し、さらに防御し直す機会はない。
通常turn_ownerは元の行動者のままで、病気tickを反応者へ移さない。

被弾時Gloryは最終的な対象へ付き、闇の致死も最終対象へ適用する。
回復や所持金も最終対象へ与える。反応者自身へHP回復が飛んだ場合は反応者が回復する。

病気prefixでHeavenから転落してHP0になっても、死亡確定前なら後続210/215の使用は続く。
例えば他者の216回復へ175→210と応答すると、Fever付与で一度HP0になり、
210の自己跳ね返しでHP+10となってから解決を終える旧サーバー経路が読める。
これはソースから導いた条件例で、Flash UIがその複合選択を許すかは未検証。
HP0を見た瞬間に終局とすると、死亡確定前に回復する経路を失う。
一方で、死亡確定済みの者へ通常行動を許可する根拠ではない。

反応奇跡は保持再使用できるため、連鎖の有限性を手札数だけでは説明できない。
今回はMP回復を連鎖へ追加しないので各使用のMP5/6で有限になる。
理論上MP99ずつなら保持210を各19回使え、計38回の反応余地がある。
元の奇跡コストや手札の反応も含め、従来の32回上限を固定した検証は再点検する。
学習用max_decisionsによるtruncatedは自然な遮断や解決完了と区別する。

215の遮断はこの攻撃項目を終了し、[Session.commandHandler][session]から通常の手番終了へ進む。
今回の1対1・残り攻撃キューなしなら、元turn_ownerの病気tick・配布・終局判定の既存順序を維持する。
遮断のために元攻撃のsource/targetを入れ替えたり、反応者の病気を余分にtickしたりしない。
新LastReactionへblockを記録しsource=target=防御者とする場合は、旧XMLに同じイベント型があるわけではなく、
この環境の観測規約として区別する。

## 状態・行動・観測の提案

既存の15種類の保持indexとuint16 ability maskは維持する。2種追加で17種類になり、同じ16bitへは収まらない。
新しい防御保持配列2と、応答専用の保持反応mask2bitを追加する案が狭く扱いやすい。
手札からは既存DEFENDのbaseで210/215のslotを指し、保持からは新しいDEFEND_CASTを使う。
新kindのbase=防御保持index0/1、slots=通常prefix、target=未指定なら6byte Actionを維持できる。
通常のCASTや既存abilitymaskへ範囲外bitを詰めたり、reserved byteを暗黙利用したりしない。

コアでは次を明示して検証する。

1. 反応種別にblockを追加し、WEAPON/MAGICの作用対象と通常属性制限を別フィールドで扱う。
2. 初使用・再使用・MP不足・Glory・prefixを使用前に検証し、失敗時のbatch原子性を維持する。
3. 防御専用奇跡をmain utility/castから除外し、手札DISCARDとresponseからの初回取得を許す。
4. Pendingの元の奇跡使用者・保持所有者と、現在のsource/targetを区別する。
   跳ね返し後も元の支払者はturn_owner側に残り、反応者のMPだけを追加消費する。
5. LastChanceの元イベントを反応で書き換えず、200のGloryや207の闇を新対象へ解決する。
6. 回復・所持金のPendingでも反応連鎖と暫定HP0を扱い、解決時のHP回復後に死亡判定する。
7. 反応回数・各座席の配布予定・保持数の上限検証を、手札16や反応32だけに依存させない。
8. 新しい保持2種の観測はsuffixへ加え、他者の非公開保持数や抽選prefixを露出しない。
   旧APIの出力寸法・stride・既存保持indexと台帳を維持する。

## 現行説明と受入条件

[2026-09-20の第三者抽出][bible]にはTurbulence=MP5の奇跡跳ね返し、Wall=MP6の無属性武器遮断があり、
210/215の特徴に対応する。ただし旧IDとの同一性や現在の公式動作を直接確認した証拠ではない。
光防御のC01、病気悪化のC02、Gloryの手動入力差は引き続き未解消として扱う。

1. 手札215/MP6でNE武器を遮断: MP0、保持+1、手札消費1、配布予定1、被弾効果なし。
2. 保持215/MP6で再遮断: 保持不変・手札消費なし。MP5なら全状態不変で拒否する。
3. 215は有属性/闇WEAPONとMAGICへ使えず、210はWEAPON/SUNDRYへ使えない。
4. 手札210で198/200/207を跳ね返し、他者targetなら再応答、本人targetなら即解決する。
5. 216/220にも210を使え、効果対象は新targetとなる。元使用者のMP・保持は再変更しない。
6. 命中済み200を返しても命中prefixを追加消費せず、最終対象の残余被弾にGloryが付く。
7. prefix防具の消費・副作用は残り、返る威力は元のまま。Gloryではprefix＋奇跡を拒否する。
8. 防御奇跡を保持から繰り返して、手札無消費・各回MP・各回配布予定・元所有者だけのtickを確認する。
9. 暫定HP0で連鎖が続く局面と、回復自己bounceでHPが戻る局面をscopeに応じて検証する。
10. 旧9profileのAPI境界と再現性、new kindのlayout・違法手・done freeze・秘密情報の非公開を検証する。
11. HP/MP/所持金回復・解除へ通常DFSだけを使っても、消費と副作用だけが起き、元の効果量は減らない。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV#L198-L227
[item]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/item.py
[defense]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L510-L643
[use]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L276-L301
[piece]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/commandPiece.py
[valid]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L78-L94
[room]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py
[bible]: https://github.com/BrandenXia/godfield-bot/blob/main/data/snapshots/2026-09-20/bible.json
[utility]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L445-L485
[session]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/session.py#L274-L304
