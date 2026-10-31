# 旧版の万能反射196

調査日: 2026-10-31。対象はIgoorxの非公式Flashサーバーエミュレータの
公開master。公式完全再現を主張せず、v16までの対応範囲に196だけを追加する判断資料。
188霧・189幻覚および未対応神器を、万能反射の追加に伴って実装済みにしない。

## 台帳と、選択順の例外

[旧CSV][csv]の196はPROTECTOR、defenseExtra=REFLECT_ANY、属性NE、価格10、重み1。
attackKind・attackExtra・defenseKind・値・subvalue・命中率は空（読込後0）。
印字ATK/DEF/boostなし、MP消費・奇跡保持化なし。通常攻撃のbaseや追加boostにはならない。

[defenderCommand][defense]はREFLECT_ANYを入力分類に関係なく発火し、
currentAttackのattacker/defenderを交換してbreakする。ただし後段の合法性検査は
「先頭の選択神器がREFLECT_ANY」または「incoming先頭神器がMAGIC」の場合だけ免除される。

| 応答での実使用順 | incoming MAGIC | incoming 非MAGIC |
| --- | --- | --- |
| 196を先頭に使用 | 属性・DARK・counter検査を免除 | 属性・DARK・counter検査を免除 |
| 通常prefixの後に196 | 同じく免除 | DARK禁止、COUNTER禁止、最終属性適合が必要 |

196は最後にNEとして防御属性合成へ参加するため、prefix付きの最終防御属性はNE。
[checkDefense][attribute]に照らすと、非MAGICのprefix付き196は最終incoming属性がNEの
非counterに限る。195で通常の属性武器をNE化してから196を使うことは可能だが、
counter来歴を消せないのでcounterに195→196は使えない。
MAGICならLIGHT/DARKを含め、前置防具の属性に関係なく196まで到達できる。

196より後ろの選択品にはusePiece/deal+1/DFS効果が実行されない。
前置175の病気効果や195の属性消去、前置品の消費・配布予定は残る。
通常DEFで減った威力は元currentAttackのcloneに戻るため失われる。
発火時にはprefix中のcounter予約も行わない。

注意: 旧serverは先に全選択列の所持を検証し、COMMANDには選択列全体を載せる。
したがって196の後に置いた未消費神器もwireで公表され得る。
本エンジンの意味Actionを「消費するprefixと発火base」に正規化すれば、
そのような無効suffix選択の公表までを再現する契約にはならない。
196を単に任意prefix付きの万能特殊baseとして扱う実装も誤りになる。

## 対応counterと、受益者の変更

[counter予約][counter]は元攻撃の残余正・特殊未発火で起きる。
予約した各itemのtype/defenseKindはPROTECTOR/COUNTERのまま保存する。
196先頭なら、到達して応答を求められた次のitemを返せる。

| ID | 反射する内容 | 反射後の扱い |
| --- | --- | --- |
| 190 | SOILダメージ、残余×2 | raw威力・属性・counter来歴を保持、新targetへ |
| 187 | FIRE・75％ダメージ、残余×1 | 命中済みitemのみ応答可能、再抽選せず新targetへ |
| 191 | LIGHT・純GLORY | 純付与を新targetへ。印字属性によるダメージは発生しない |
| 192 | DARK・純DARK_CLOUD | 純付与を新targetへ。DARK即死には変換しない |
| 194 | NE・所持金請求、残余×1 | **現在の新sourceへ入金し、新targetへ請求する** |

194の[ABSORB_YEN][effects]はcurrent attackerに要求全量を先加算（99上限）し、
その後current defenderの所持金→MP→HPから支払う。
たとえばBがAへ194を予約しAが196で返すと、Aが受益者、Bが支払者になる。
予約元Bや通常手番ownerへ入金を固定してはいけない。元の要求量は変えない。

193は別扱い。[予約時][counter]にsource=target=反撃者となり、
[doAttack][attack]の自己対象分岐が空防御で自動解決するため196を選ぶ機会がない。
現範囲の正常なゲーム進行から「193を196で反射する局面」は作らない。
自己対象で使う通常回復・魔法や、自分へ跳ね返されたitemも同様に応答を省略する。

## 命中・source・origin・FIFO

[AttackData.clone][clone]はisCounter、decidedValue、chance、属性、pieceListを保持する。
反射はdoAttackやqueueAttackを再実行せず、現在のsource/targetだけを交換する。
187や％魔法の再命中判定、DarkCloudの必中判定、MP再支払、再保持化は発生しない。
LastChanceは元の抽選の記録として維持する。反射で新たな抽選イベントを作らない。

通常手番ownerと、解析用の予約originは維持してよいが、効果の受益者・支払者、
後続counterを予約するときの対象はcurrent source/targetから決める。
196で返された190をさらに普通防御＋190で受ければ、その残余から新しいcounterが
現在の相手へ予約される。既存FIFOの末尾へ追加し、196自体は新itemを積まない。
反射中に残queueの並べ替え・開始、死亡確定、owner病気tick、配布を行わない。

## deadと暫定HP0

[doAttack][attack]はcounterのsource.deadでは停止せず、target.deadならitemをskipする。
そのため死亡済みsourceが残したcounterは、生存targetへ一度届き得る。
生存targetが196で返すと、新targetがそのdead sourceになる。
[特殊後処理][defense]はtarget.deadで直ちにitemを終了し、反射先へのダメージ・
付与・194入出金を実行しない。196の消費と配布予定は残る。

HP0でもdead=falseならこの停止条件には当たらず、同じitemの応答が続く。
前置175でHeavenからHP0になった人物も、196で反射できる局面なら反射sourceとして残る。
もう一度返った回復魔法で死亡確定前に救われる可能性も保つ。
一方、既にdead確定した人物を回復する応答は、反射先dead停止で生じない。

[Room.endInning][finish]はitemが終わってから死亡確定し、FIFOが空になるまで
次itemを処理する。全queue終了後にownerの病気tick、配布、終局確認へ進む。
反射でcurrent itemが終了しても、後ろの予約itemを消去してはいけない。

## 最小scopeと受入条件

196単種を追加し、既存16byte Actionのdefend.base=196手札slot、slots=実使用prefixを使える。
slots空は196先頭、非空はprefix後196と解釈すれば、現範囲の実状態遷移を区別できる。
後続未消費選択のwire公表は契約外と明記する。本profile既存のGlory1枚制限も適用する。

受入確認には、196単体の全属性・DARK・counter許可、prefix時の分類別制限、
195→196、普通prefixのraw威力復帰、prefixcounter非予約、196後suffix未消費の
正規化方針、187抽選維持、191/192純付与、194受益者逆転・dead返却時入出金なし、
193に応答機会なし、HP0反射/回復救済、FIFO継続を含める。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV#L197
[defense]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L510-L643
[attribute]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/item.py#L128-L147
[counter]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L588-L603
[effects]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L445-L494
[attack]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L317-L380
[clone]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L58-L77
[finish]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py#L369-L496
