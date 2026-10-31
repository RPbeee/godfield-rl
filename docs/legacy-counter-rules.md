# 旧版COUNTER: 反撃予約・元攻撃・死亡・キュー

調査日: 2026-10-31。Igoorxの非公式Flashサーバーエミュレーターをwebで読んだ結果。
公式の現行挙動や旧クライアントUIの実測ではない。可変`master`のため台帳には取得物・hash・範囲を残す。
v13の採用範囲は190単種で、同種複数選択を許す。後述の旧サーバーとbotの差は未解消のまま保持する。
190以外の行は後続のための調査記録であり、この段階の台帳には追加しない。

## データと分類

[旧CSV][csv]のCOUNTERは187〜194の8種。全て**PROTECTOR**で、defenseKind=COUNTER。
WEAPONでもMAGICでもなく、通常DFSは0。価格10・重み1、MP消費なし・保持化なし。
value/subValueの空欄は0となり、可変の値は発火時に決める。[Item.fromData/getDef][item]参照。

| 旧ID | 属性 | attackKind / extra | 発火後の効果 | 候補 |
| --- | --- | --- | --- | --- |
| 187 | 火 | ATK | 残余rの威力、命中75% | 次段階 |
| 188 | 水 | ADD_HARM / FOG | 現在攻撃元へFog | 霧未対応につき除外 |
| 189 | 木 | ADD_HARM / ILLUSION | 現在攻撃元へIllusion | 幻覚未対応につき除外 |
| 190 | 土 | ATK | 現在攻撃元へ威力2r、必中 | v13採用 |
| 191 | 光 | ADD_HARM / GLORY | 現在攻撃元へGlory | 次段階 |
| 192 | 闇 | ADD_HARM / DARK_CLOUD | 現在攻撃元へDarkCloud | 次段階 |
| 193 | 無属性 | INCREASE_MP | 反撃使用者自身のMP+2r | 次段階 |
| 194 | 無属性 | ABSORB_YEN | 現在攻撃元からrの所持金を吸収 | 新しい資源移転のため別段階推奨 |

191/192は直接の状態付与でATKではない。192の闇属性を理由にHP0にしてはいけない。
193は相手へMPを与える効果でもない。
PROTECTORは通常手番の先頭にできず、通常DISCARDはできる。捨てても反撃は発生しない。

## 反撃予約のタイミング

[defenderCommand][defense]は選択列を消費し、普通DFSと防御時病気を処理してから最終防御属性を照合する。
反射・跳ね返し・遮断が発火しなかった場合だけ、残りの`clone.damage`が正ならCOUNTERを列順に探す。
それぞれを`queueAttack(..., forced=True)`へ予約し、その**後**に元攻撃の`inflictDamage`を呼ぶ。

1. 実際に選んだ各防具・指輪を消費し、その使用者の配布予定+1。
2. 普通DFS後の残余rを確定する。counter自体のDEFは0。
3. r>0なら各選択COUNTERの反撃を予約する。
4. 元攻撃の被弾時状態付与・闇等・HP減算を解決する。
5. 手番終了処理へ入り、死亡確定してから、予約済みキューの続きを実行する。
6. キューが空になった後、元turn_ownerの病気tick・配布・終局へ進む。

r=0なら指輪は消費されても反撃しない。非ATKの回復・所持金増加・解除ではrは正にならず反撃しない。
反射等が発火するとCOUNTER予約ブロック自体を通らないため、前置した指輪を使っても反撃は予約されない。
この場合もprefix190は消費・配布予定増分・防御属性合成には参加する。
指輪の発見時点で予約する実装にせず、prefix走査が終わって特殊発火の有無が確定してから予約する。
複数特殊の後ろに置かれた選択品が未使用になる点は既存の意味的prefix制限と同じ。

rは実際のHP減少量ではない。HP2でATK10を受けても190は20。
闇ATK5で死ぬ場合でも190は10であり、999の2倍ではない。
被弾状態付与がHeavenを転落させても、予約値はその前のrから決まる。
守護神による後段吸収を含む範囲へ将来広げるときも、実HP差へ置き換えない。

## 強制キューと対象

[queueAttack][queue]のforced経路は所有確認・再消費・MP支払・配布増分を繰り返さない。
消費・配布予定は最初の防御使用時の1回だけ。
反撃使用者が新source、元攻撃の**現在source**が新targetであり、通常turn_ownerとは限らない。
193だけはtargetを反撃使用者自身にする。

187/194にはr、190/193には2rを`decidedValue`として記録し、`isCounter=True`にする。
191/192はADD_HARMであるため、残余ダメージを持つ通常攻撃へ変換しない。
190も新しいAttackDataとして作るため、元攻撃の病気・harm・奇跡・命中率・属性除去・setter由来を継承しない。
生成直後は固有の土属性ATK2r・counter190だけを持つ。反撃へ195を使えば、その項目を新たに無属性化できる。

187は正のhitRateを持つATKなので、強制キューでも全敵展開へ入り、当時生存している敵へ1回ずつ積む。
1対1では元の相手1人。命中抽選は予約時でなくキューを取り出す[doAttack][hit]で行う。
対象のDarkCloudはその抽選時点で調べ、あればhit RNGを消費せず命中する。
193の自己対象は空応答で即解決し、MPは上限99。

191/192の状態付与は[inflictDamage][damage]の直接ADD_HARM経路。
残余被弾の有無を再要求せず、192で闇の999ダメージは発生しない。

## 元攻撃後に死んだ使用者の反撃

[Room.beforeEndInning][room]はキューを取り出す前に`handlePlayersDeath`を呼ぶ。
元被弾でHP0になった反撃使用者はここでdeadとなるが、既に積んだ反撃を削除する処理はない。
[doAttack][hit]が調べるのは**target.dead**で、source.deadを理由にスキップしない。

従って、190の使用者が元攻撃で死亡しても、生きている相手への予約反撃は実行される。
反撃が相手も倒せば、生存者なしの終局が起こり得る。
原攻撃で敵が死亡した時点のwinnerを先に確定してはいけない。

193は自己対象なので、元被弾で使用者がdeadになればtarget.deadによりスキップされ、MPを得ない。
反撃のtargetが別の先行効果ですでにdeadでも、その項目はスキップする。
正の確率項目はtarget.dead確認より先に命中式を評価するため、スキップでもhit RNGを消費し得る。
この死者は、既存の病気＋反射中の「HP0だが未確定」と区別して管理する必要がある。

キューの途中では配らず、元turn_ownerの病気tickもまだ実行しない。
元所有者が反撃で死亡確定していればそのtickはしない。反撃使用者を新しい通常手番所有者にしない。

## 属性、195、Glory、既存特殊

counterも通常防御列の最終属性合成へ含まれる。指輪を1枚ずつ適合判定するのではない。
元攻撃を195で無属性にすれば指輪との組合せを受理できるが、**新しい反撃の属性**は指輪固有のものに戻る。
例えば195＋190で元闇攻撃を無属性化して受けても、反撃は土属性。

Gloryの意図規則なら指輪も使用神器1枚なので、指輪単独は可能、指輪＋DFS/195は2枚で拒否する。
旧サーバーにGlory枚数検証がない差は既存profileと同じく明記する。

counterの先頭typeはPROTECTORなので、既存20/39のWEAPON反応、210のMAGIC反応、215のWEAPON遮断は発火しない。
REFLECT_ANY196は別の例外だが未対応のままとする。
元の攻撃がWEAPONでもMAGICでも、正の残余rと属性適合があれば最初のcounterを予約する。

## 反撃への防御: サーバーとbotの未一致

[bot.buildDefense][bot]はisCounterAttackなら普通防具や指輪を組み立てる前にreturnし、反撃へ防御しない意図を示す。
一方、serverの[defenderCommand][defense]の通常DFS・195・COUNTER経路には、counter由来を拒否する条件がない。
最終属性に適合すれば普通防具を使え、残余が正なら更にcounterを予約し得る。
serverの末尾にはcounter禁止検査があるが、これは特殊反応のブロック内である。

これは公式でどちらが正しいかを確定した証拠ではない。
旧サーバー受理とbot意図のどちらを採用するか、C05候補として明示する必要がある。
v13は**旧サーバー受理**を採用し、反撃へ普通DFS・195・更なる190を使える。
botの防御不可という意図を採った挙動ではなく、公式完全再現として扱わない。
意図規則として反撃への防御を禁止するなら、元攻撃後のcounterは選択肢なしで自動drainできる。
その場合もサーバー受理経路の完全移植とは区別する。

server受理を採用すると、195を併用した190への再190などで威力が2倍に増幅する経路がある。
手札の指輪で有限でも、既存のamount上限やint範囲を無検証で使わない。
「counterを受けたので先にfinish_turn」として元ownerのtickや配布を挟むのも誤り。

## v13のscopeと6byte Action

190だけを追加し、通常DEFEND.slotsへ普通DFS/195と同じく複数190を含める案。
反撃へ外部応答を待つPending/queueとdead管理が必要になるが、Actionは6byteを維持できる。
190単独は無属性・闇・木の入力に適合する。普通DFS等と混ぜる場合は最終合成属性で照合し、
195があれば元入力を無属性にした上で判定する。

同じ応答で選んだ190は全て、同じr・source・target・土属性・必中の同一項目を生成する。
従って、同種の190同士の選択順をmaskへまとめても遷移は変わらない。
特殊より前のprefix190として消費する既存の意味も維持する。

通常手番の開始時に両者の手札は合計最大32枚。キューの処理中には配布しないため、
1手番で新しく使える190は全体で32枚以下となる。
元の通常行動が保持castなら手札を減らさないので、上限を31枚に狭めない。
生成済み未処理queueも32件以下であり、fixed queue capacity32はこの範囲で安全。
初期攻撃の上限を480とした場合、32段すべてで2倍となる保守的な上限`480LL << 32`もint64へ収まる。
配布・神器複製・保持counter・新しい倍率を加える場合は、この上限の証明を使い回さない。

将来187/190/191/192/193の5種を一度に入れるなら、まず**1応答でcounterは1枚まで**と明示限定する案がある。
DEFEND.base=選んだcounter手札slot、slots=普通DFS/195 prefixとして6byteを維持できる。
これは全ての複数反撃選択を表すschemaではなく、意図したsubset制限。

複数異種counterを無順序maskへ自由に入れると、元の反撃キュー順を失う。
192→187ならDarkCloudが先に対象へ付き187が強制命中するが、187→192では最初の抽選が残る。
死亡や次項目のtarget.deadにも影響するため、単なる最終属性指定で圧縮できない。
将来は順序付き反撃列などを追加する。同種複数でも別対象・死亡・抽選を含む挙動は個別に検証する。

## 必須のコア変更と受入条件

1. counter種別・固有属性・可変値・直接状態効果・率を台帳で区別し、通常DEFやWEAPONへ変換しない。
2. 使用時にcounter予約を作るが、元攻撃の解決前にcounterへPendingを上書きしない。
3. 元被弾の死亡確定、キュー継続、owner tick、配布、終局を分離する。予約済みdead-source攻撃を捨てない。
4. 元通常所有者・現在source/target・counter使用者・counter由来フラグを持ち、restore条件を拡張する。
5. 一回の反撃使用の消費・giftを二重計上せず、強制キューではMP/手札/保持を変更しない。
6. v13の190は必中で命中乱数を消費しない。死者sourceからの発動・死者targetのskip・同種複数の順を検証する。
7. r=0で不発、実HP差とrの相違、闇999とrの相違、元被弾状態付与との先後を検証する。
8. 195、Glory、MAGIC由来攻撃へのcounter、反撃への既存特殊拒否とC05規約を確認する。
9. 使用者が元被弾で死亡しても190が相手を倒す局面、193は死亡でスキップされる局面を検証する。
10. 旧12profileのAPI寸法・stride・乱数再現性を保ち、新しいqueue観測が隠れた手札や乱数を漏らさないようにする。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV#L188-L195
[item]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/item.py
[defense]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L510-L643
[queue]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L85-L316
[hit]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L317-L380
[damage]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L382-L494
[room]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py#L369-L474
[bot]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/bot.py#L638-L748
