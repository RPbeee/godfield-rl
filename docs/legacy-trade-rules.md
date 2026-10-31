# 旧版の売却3 SELL・購入4 BUY

調査日: 2026-11-01。Igoorxの非公式Flashサーバーエミュレータ公開masterを一次根拠とする。
公式完全再現と区別し、既存0..99資源・16枚上限・duel subsetへ追加する判断資料。

## 正重量台帳と推奨する導入順

[CSV][csv]の3はTRADE/SELL、4はTRADE/BUY、両方price5・weight20。
attackExtra/defenseKind/defenseExtra/属性/value/subvalue/MP/率は空（数値0）。
他の正重量TRADEは対応済み2 EXCHANGEだけ。

v31の確定scopeは**SELL3だけ**。商品1枚・target1人を選んだ後は既存の応答で閉じ、新しい乱数や承認phaseを要しない。
BUY4は抽選後に現在の買い手のyes/noを別行動として待つため、その次の段階を推奨する。

## SELL: 2枚の所有検査と先払い消費

[convertPiecesToOwnedPieces/queueAttack][queue]は全physical入力を先に所有品へ変換する。
列は先頭3＋売却商品1枚のちょうど2枚。先頭3をusePieceしてdeal+1した後、
商品を直接discardPieceし、その価格をdecidedValueへ保存してbreakする。
商品は攻撃追加品としての妥当性検査やusePieceを通らない。

従って商品は同じ手札の別physical copyならWEAPON、MAGIC、245MORTAR、239FREE、TRADEも選べる。
別copyの3自身を売ることも可能。3と同じslotの二重指定、所持していない239などは所有変換で拒否される。
FIXEDや保持abilityは実手札のdiscardPiece対象にならず、保持奇跡の売却は不可。
MAGIC商品はMPを払わず、保持化せずに手札から消える。245の手動DISCARD保護もここにはない。

消費は**手札2枚、gift1**。商品分のgiftを加えない。
239を商品にすると事前FREE走査が先頭TRADEを無料対象にするが、TRADEにMP費用はないので追加効果はない。
239は他の商品と同じく実消費され、価格5を請求する。未所持239を無料化suffixとして借りる経路ではない。
3枚以上のextra、保持MAGIC追加、普通boost/setter/214/209を付けることはできない。

## SELL: 型・対象・反射

SELLはisActionをtrueにしない。指定selfも許し、selfなら[doAttack][attack]で空応答の即時解決。
他者なら通常の応答待ちとなる。
queueAttackの早期breakにより属性はNoneで、chance0/非damage。
普通防具/195/普通prefix付き196は合法にならず、先頭196だけが反射できる。
TRADEなのでWEAPON/MAGIC用の反射・跳ね返し・遮断には一致しない。

商品は最初の使用者から既に消えているが、pieceList[1]のitem ID/priceは保持される。
[clone/反射][response]はこれを保持してcurrent source/targetを交換する。
反射後に現在sourceが商品を所持していることを要求したり、そこから再び商品を消費したりしない。
一度も解決していない商品を複製するのではなく、先払い消費された商品の配布先と受益者が入れ替わる。
kind13の196→FREE未消費suffixとGloryの入力枚数規約は既存契約に従う。

## SELL: 請求→直接配布→入金

[inflictDamage][sell]は次の順序で実行し、金欠や満杯でも途中でrollbackしない。

1. current targetへ商品価格全量をdecreaseYenで請求。
2. current targetへ商品IDをdealItem(...,True)で新しい手札copyとして直接配布。
3. current sourceへ商品価格全量をincreaseYen（99上限）。

[decreaseYen][player]はYEN不足分をMP、それでも不足ならHPから払い、MP/YEN/HPの下限0を守る。
商品価格を実際のYEN消費量やtarget残資源で減額しない。
194はsourceへ先に入金する効果なので、その順序をSELLへ流用しない。

商品価格10、targetYEN3/MP4/HP40ならtargetはYEN0/MP0/HP37になる。
sourceはその不足とは無関係に価格10を受け取る（99上限）。
targetが自分でも順序を省略しない。同じ初期値でself SELLなら支払後0/0/37、
商品が戻った後に入金され、最終YEN10/MP0/HP37となる。MP/HPの損失は返らない。

[dealItem][deal]はisAction=Trueでもhand_count==16ならFalseを返す。
SELLはこの失敗を無視するので、満杯targetは支払いだけ行い、商品は消失し、sourceへの入金は続く。
成功時は元のobjectを移動せず、IDからfreshなcopyを末尾へappendする。
isAction=TrueはIllusion判定とprivate DEAL通知を省略するが、手札上限は免除しない。
この直接配布に重み付き配布RNGを使ったり、MAGICの保持取得を発火させたりしない。

請求でHP0になっても配布とsource入金まで終えてから死亡/244・owner病気・通常gift配布へ進む。
商品244の配布に成功すれば、今受け取った244が同じ請求によるHP0を救える（alive2条件）。
self/196反射後でも同じで、満杯で新244の配布に失敗した場合はその新copyによる救済はない。
価格0の245も有効商品で、資源変化0でも直接配布を試みる。
自己売却では商品が戻るので、通常成功時のnet手札変化は3の消費とgift1だけになる。
相手売却ならsourceは2消費/gift1でnet1枚減り、targetは直接配布成功時に1枚増える。

## BUY: 抽選と承認phase

BUY4は[queueAttack][queue]で1枚だけ使用し、消費/gift1、属性None・MP0・保持化なし。
target指定/196/195などの型制約はSELLと同じ。
[inflictDamage][sell]は最終targetのgetRandomPieceを一度呼ぶ。
手札0なら抽選なし、非空なら各physical copy均等で1回（1枚でも呼ぶ）。
この時点で商品はまだ削除されない。245/MAGIC/武器/価格による抽選除外はない。
買い手のYENが不足していても、先に抽選して商品を公表する。

[応答末尾][response]では商品無し、または現在sourceのYENが商品価格未満なら承認を待たず終了する。
払える場合だけcurrent source本人へyes/noを求める。反射後は元ownerでなく新sourceが買い手となる。
その間に死亡hook・owner病気・通常giftを進めない。
YES/NOの二択で閉じるが、main/responseと別のapproval phaseとactorが必要になる。

**自己BUYは例外**。self空応答で商品を抽選した後、defenderCommandのself早期returnが
末尾BUY承認分岐より先に実行されるため、承認・移転・支払いは起きず、そのまま手番を終える。
自分の4は抽選より先に消費されており、それ自体を選ばない。

[playerBuyResponse][buy]は元targetが選ばれた実copyをまだ持つことを確認する。
YESかつYEN十分なら、sellerの実copy削除→seller価格入金cap99→buyerへfresh配布→buyer価格支払いの順。
YENが足りる時だけ実行するので、BUY成立時にMP/HPの不足cascadeは使わない。
NOでは商品/資源を変えず、使った4とgift1は返さない。
汎用sourceはbuyer満杯で配布失敗しても、seller削除/入金とbuyer支払いを戻さない。
ただしmain4消費後、または196消費後のbuyerは最大15枚なので、
assistant/外部注入なしのduelでapproval時buyer満杯16は到達不能。SELLtarget満杯16は到達可能で必須検証。

## 公開情報と有限なAction案

[初回COMMAND][attack]はSELLの3と商品両方、priceのdecidedValue、source/targetを公開する。
商品IDは相手応答より前から公知。反射COMMANDは新targetと同じpriceを伝える。
BUYは選択商品をcommandChain.pieceへ[writeXML][xml]で全体公開し、slot/population/抽選値は送らない。
承認は[Session BUY][session]のdoBuy有無、結果BUYにもdoBuy有無が載る。
未来の選択結果や相手の残手札全体を追加観測してはいけない。

SELLだけなら新kindでbase=手札3、商品は別slot1個、target0/1、他fields0とすれば88byte Actionで閉じる。
最大16base×15商品×2対象と、既存196応答の有限集合であり新phase/RNG/保持容量を増やさない。
商品候補maskは全physical手札を条件付き候補とし、同slot除外はlegalで検査する。
既存magic型には価格fieldがないため、旧台帳型/hashを変えず独立ID→CSV価格表を加える案を推奨する。
MP費用から価格を推定しない（216は費用6/価格15、220は費用8/価格25、221は費用10/価格15）。

SELLの先払い商品消費はgiftを増やさないが、旧pending−retained会計の上限を緩める方向。
解決後は直接配布を終えて即item終了なので、未解決counterやattack-compositionと商品配布が同時に存在しない。
ただし196を消費してgift待ちの人物が商品を受け取り、その請求で死亡した場合、
死亡者のpending giftは配布されず残る。手札＋gift待ちの物理消費会計が一時的に17となり得るため、
v31はその直接受領分だけを非公開incoming_sale creditとして区別し、手札上限16自体は緩めない。
生存者のgift配布時にcreditを消し、死亡終局では同手番の実配布イベントとの整合を検証する。
restoreには元商品ID/固定価格/元ownerと、反射後current source/targetを区別して保存する。
BUYはさらに選択されたtarget実slot、価格、承認actorを保持し、approval中の他操作を拒否する必要がある。

受入条件は、武器/MAGIC/245/239/別3商品、同slot/未所持/保持売却拒否、gift1だけ、
金欠cascadeとself alias、source99cap、target満杯失敗、196往復後の既消費商品とcurrent受益者、
MP/保持化なし、price0、244/病気/通常gift順、旧ABI/台帳/RNG維持を含める。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV#L4-L5
[queue]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L71-L156
[attack]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L318-L380
[sell]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L397-L429
[player]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L112-L121
[deal]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L205-L227
[response]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L604-L653
[buy]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L50-L70
[xml]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/commandPiece.py#L48-L57
[session]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/session.py#L265-L273

## v31確定API

g31 ABI31は88byte Actionの旧kind0..14を維持し、SELL(kind15)を末尾追加する。
baseは手札3slot、slotsは別商品の1bit、target0/1、extras/counter fieldsは0。
Config.sellingは独立で、旧g30はkind<=14/1175観測を維持する。BUY4はこのprofileへ追加しない。
価格は旧型/hashを変えない216種の独立表、観測は公開手札IDから導ける自分の価格だけを補助特徴にする。
obs1221の1175enabled、1176..1191手札3、1192..1207自分価格/max、
1208pendingSELL、1209商品ID/363、1210price/max、1211event存在、
1212source==actor、1213target==actor、1214event商品ID/363、1215price/max、
1216配布成功、1217YEN支払/99、1218MP支払/99、1219HP支払/99、1220実入金/99。
mask32のword30/31がSELL base/全physical商品候補。完全な組合せはlegalで判定する。
recipientのincoming_saleは物理会計用の内部creditで、観測には含めない。
