# 旧版の手札除去240と次候補の比較

調査日: 2026-10-31。Igoorxの非公式Flashサーバーエミュレータ公開masterに基づく。
現行公式の完全再現とは区別し、v27は240単種のduel subsetとする。

## 台帳と解決

[旧CSV][csv]で正重量REMOVE_ITEMSは240のみ。
SUNDRY、attackKind=REMOVE_ITEMS、value1、price10、weight3。
属性/MP/率/attackExtra/defenseKind/defenseExtra/subvalueは空。249CREATUREは重み0なので除外。

[queueAttack][queue]は手札240の単独1枚使用に制限し、使用者の消費・deal+1後に早期breakする。
通常属性合成より前なので、実行中属性はNE空文字ではなくNoneのまま。
[inflictDamage][remove]はdamage/HP/valueを参照せず、最終対象のgetRandomPieceを1回だけ呼び、
選ばれた実CommandPieceをdiscardする。対象のdealは増やさない。保持奇跡は対象外。

[getRandomPiece/discardPiece][player]は空手札ならNone（乱数なし）、非空ならrandom.choiceを1回。
各physical copyが同確率なので、同じIDが2枚あれば種類の確率は2倍となる。
1枚でも乱数呼出しは省略しない。選んだ実slotだけが消え、残りの並びは保たれる。
所持品の種類・価格・MAGICかどうか・捨て可否による除外はない。
245MORTARも、もしpoolに加わればこの強制除去の対象になる。
複数枚除去のループや、value回の抽選は存在しない。

## 自分・反射・抽選時点

自分指定は[doAttack][attack]の空応答で即時解決する。
自分の240は既に消費済みなので、それ自身を再度選ばない。
相手指定は応答を待ち、空防御で解決する。
None属性のため普通防具/195/普通prefix付き196は拒否され、先頭196だけが反射できる。
SUNDRYなのでWEAPON/MAGIC用の反射・跳ね返し・遮断は一致しない。

[反射処理][response]では196を消費・deal+1してsource/targetを交換する。
**手札抽選は反射時ではなく、最終acceptの解決時**なので、反射に使った196を選ぶことはない。
往復があれば双方の196が既に消費された後の、最後のtargetの手札から抽選する。
kind13の196→FREE suffixは既存契約どおり未消費で、そのcarrierが最終targetの手札に残れば抽選対象になる。
Gloryの選択枚数規約も従来どおり、未消費suffixを含めて判定する。
自己使用や最終解決で手札0になっていれば抽選なし。反射しても命中抽選や初回使用を再実行しない。

## 何が公開されるか

[CommandPiece.writeXML][xml]はremoved itemのIDをcommandChain.pieceへ書く。
通常手札のphysical slot番号は書かず、削除数・母集団数・抽選値も専用fieldとしては送らない。
普通の手札copyのabilityIndexは-1なので、忘却241のような保持リストindexも載らない。
現在未対応のIllusionがあれば真item/illusionItem表示情報が加わるが、v27には導入しない。

非self解決は[応答COMMAND][response]のroom broadcast、self解決は[初回COMMAND][attack]の
全user送信に結果pieceが載る。source/targetは元COMMANDや反射のtarget.nameから分かる文脈情報で、
除去結果piece自体にseat/nameはない。空手札なら結果pieceなしで、失敗結果が本人だけに秘密通知されるものではない。
Nativeの公開eventはattempt・source/target・removed ID・removed有無とし、
slot/population/ticketは観測へ追加しない。相手の残手札を公表してはいけない。

## 消費会計と死亡境界

使用者の240と反射196には通常のgiftがあり、被害除去だけにはgiftがない。
未使用MAGICを消しても保持化せず、過去の保持再使用によるretained_dealsも取り消さない。
244を消すなら、後続のowner病気tickと死亡hookより前に手札から失われる。
240自体はdamage/harmではなく、Heaven転落・病気解除・counter生成を起こさない。
最終解決後は既存のitem終了→死亡処理→owner病気→配布へ進む。

現scopeで被害消費の新台帳は不要。240はmainから開始する単独itemで、
以前のattack/free-cast compositionや未解決counter queueと同時には存在しない。
反射中はまだ被害消去がなく、解決時は直ちにfinish_itemでmain終端へ移る。
pending_deals−retained_dealsはgift付き行動消費を数え続け、
被害消去はhand_count+その消費数<=16の上限を緩めるだけ。
この判断は独立validation担当とも照合済み。将来の自動counter/広域REMOVE_ITEMSでは再検討する。

## v27の意味契約と受入条件

新g27 ABI27は88byte Action/kind0..14/65th tailを維持し、240は既存USEを使う。
新Config.item_removalは独立、旧26profileはflag=falseで配布乱数・ABIを維持する。
mask26のword25は240候補手札。obs1085の新suffixは1062enabled、1063..1078手札240、
1079pending除去、1080attempt、1081source==actor、1082target==actor、
1083removedID/363、1084removed有無。private population/slotは載せない。
検証用uint64 ticketはcurrent手札数で剰余、prefix64個、空0は消費なし・1枚は1消費とする。

自分/相手・空/1枚/重複・元240消費後の抽選・196往復後の抽選・未消費FREE suffix、
普通防御/195/prefix196拒否、公知結果と相手残手札秘匿、244除去後の死亡、gift0、
旧APIの固定出力範囲とbatch atomic rejectionを受入条件に含める。

## 比較して見送った245/99とHP吸収

245はSUNDRY/attackExtra=MORTAR、価格0、重み1、他の数値0。
単体の自動発火はなく、[杵99の検索][pestle]に使う保持物である。
99はWEAPON/LIGHT/ATK30/PESTLE/価格5/重み1、MP/率0、単独列限定。
全not.dead playerを固定参加順に検索して最初の245保持者へraw999を向け、
なければselfを含むrandom aliveへraw30を向ける。245自体は消費しない。
自己対象は自動解決、他者対象のserverは応答処理を受理する。
「Mortar attack, no defense」というログは被害適用後の早期returnであり、
それだけを根拠にserverが防御を拒否すると解釈しない。
cloned/反射payloadにmortar参照とrawを保持し、反射で999を30へ戻さない。
counter・244蘇生・owner病気は通常item終了順に従う。

245を手動DISCARDへ含めてもsourceは消去をskipするが、コマンド自体をrejectはしない。
240の強制除去にはこのskipがない。99+245はhidden所持検索の公開来歴、
無臼時の対象乱数、999とcounter上界、client/server応答差まで確認が必要なので別段階へ分ける。

ABSORB_HPの正重量は26NE武器ATK7(p10)、42NE武器ATK11(p15)、
103TREE武器ATK3/75%(p10)、221LIGHT魔法ATK10/MP10(p15)、全て重み1。
[効果][remove]は残余正の場合、現在sourceへ**残余全量を先に回復（99上限）**し、その後targetへ同量を適用する。
target残HPで回復量をclipしない。最終DARK分岐は吸収回復を通らない。
自己対象も回復→被害なので、上限に触れると差引0にならない（HP95/raw11→99→88）。
221の保持追加、反射後source、暫定HP0救済、倍率/setter/57などの交差が必要なため240より後を推奨する。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[queue]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L110-L162
[remove]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L383-L496
[player]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L238-L264
[attack]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L318-L380
[response]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L511-L644
[xml]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/commandPiece.py#L49-L57
[pestle]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L249-L286
