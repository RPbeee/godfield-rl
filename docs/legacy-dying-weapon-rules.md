# 旧版117 DYING_ATTACKのduelでの通常role

調査日: 2026-11-01。Igoorx非公式Flashサーバー公開sourceを一次資料とする。
room.py/turn.pyは固定commit `fcc83740a3d3cd9968e6e5cd2b7ed781f53abe80` の取得済み本文でも再確認し、
[manifest](../data/legacy-dying-weapon-v1/manifest.json)に取得hashを記録している。
v37は人物2席固定の環境へ117を追加し、現行公式や3人以上の対戦の再現とは区別する。

## 台帳

[CSV][csv]の117はWEAPON / ATK / DYING_ATTACK、属性LIGHT、value1、subValue30、
hitRate75、price10、weight1。MP・防御role・追加ATKroleはない。
[getAtk/getDef][item]に従い、通常ATKは1、DEFは0。subValue30は通常ATK/boost/DEF/MPではない。
別fieldの死亡時威力として台帳に保持し、通常roleへ加算しない。

## 通常の手札使用

[queueAttack][queue]の通常使用ではdecidedValueが無く、getAtkの1を使う。
DYING_ATTACKタグはextraへ入るが、[inflictDamage][effect]のstatus/吸収/自己被害に一致せず、
通常威力を変えない。手札1枚消費・gift1・MP0・保持取得なし。

印字hitRate75が全敵向け展開を選び、duelでは生存敵1人が対象となる。
requested targetが自分でも同じ敵へ正規化する。
通常命中は1..100の75以下でhit、76以上でmiss。DarkCloud対象は必中となりhit RNGを省略する。
missなら応答・HP効果を発生させないが、最初の使用消費とgiftは維持する。
追加boost・setter・197/217・214・209・238・FREEは元％WEAPONゆえ禁止。
117自身もINCREASE_ATK等の追加roleを持たないので別武器の追加品にできない。

命中後はLIGHT1の通常被害として解決し、30を加えない。
通常防御はLightへのDARK属性適合に従い、192のDARK counterも通常規則で選べる。
195でNE化した後は20/39/215等の武器用反応を既存の属性消去規則で選べる。
先頭196ならLight/命中済み％のまま返せる。いずれも命中抽選へ戻らない。
元typeはWEAPONであり、MAGIC反射・跳ね返し・遮断を発火させない。

117自身に防御roleはなく、canonical subsetでは応答に使わない。
一般旧serverの弱いdefenderCommand guardでは適合するLightゼロDEF消費が受理され得るが、
これは他の非防御WEAPONと同じ未採用境界とする。手札交換として意味を持ち得る受理差であり、
UI/公式の受理は未照合。
通常の武器DISCARD制約を維持する一方、SELL商品やBUY抽選/購入の対象にはなる。
商品としての価格は10で、売買時に命中判定・通常攻撃・死亡時攻撃を発火させない。

## 死亡時hookが2席では到達不能となる証明

[Room.handlePlayersDeath][death]の順序は次のとおり。

1. HP0・dead=falseの人物について、生存フラグ数2以上かつ244所持なら復活してcontinue。
2. それ以外は、その人物をdead=trueにする。
3. **dead設定後**の生存フラグ数2以上、かつ117所持なら死亡時攻撃を予約する。

[getAliveCount][alive]はHPでなくnot.deadの人数を数える。
人物が固定2席なら、手順2を終えた時点でnot.deadは最大1人であり、手順3の条件は必ず偽となる。
244で救われた人物は手順1のcontinueにより手順2/3へ進まない。
244が何枚あっても、復活後に病気等で再び死亡しても同じ人数上界が成立する。

両者が同時HP0でも、固定席順の各走査でこの証明は変わらない。
seat0が復活しseat1が死亡するときも、seat1をdead設定した後のaliveは1。
seat0が先に死亡すればalive1となりseat1の244も使えず、117の検査時にはalive0になる。
両者復活なら誰もdead化されず、どちらも117分岐へ進まない。
counter FIFO内の死亡確定やsourceが既deadの項目でも、人物数は2のままである。
117の追加によってこの前提を変える人物生成や助手・3人戦を同時に導入しない。

汎用[TurnHandler.playerDyingAttack][dying]は3人以上で呼ばれた場合、
decidedValue30を付けたforced攻撃を予約する。しかしduelではこの呼出し自体が到達不能なので、
そのqueue・死亡時RNG・DYING通知を環境へ新設する必要はない。
これは到達し得る固有效果を削った簡略品ではなく、固定duelにおいて全到達roleを扱うscopeである。

## v37と受入条件

既存chance/Light/195/typed反応・raw被害・売買を117へ通す。
新Action/Effect/Pending/history/tape/holding/death queueは導入しない。
LastChanceは元117と率75を保持し、反射後のcurrent source/targetと区別する。
active117観測はresponseかつ元LastChance117・hitRateあり・miracle/counterなしの項目だけ。
117由来のcounterへ進んだらactive117は0とし、最後の117命中履歴は公知情報として残す。
未来の命中/跳ね返しテープは非公開。旧36profileは新flag=falseで台帳/ABI/RNGを固定する。

受入条件は自然pool、ATK1/boost0/DEF0/MP0/sub30非加算、75/76境界、DarkCloud、
self指定の敵正規化、全extras/追加117禁止、Light通常防御/counter192、195/196と再抽選なし、
交換HP0でも117非発火、244優先後も117保持、売買価格10、旧37ABI境界・原子的拒否を含む。
同時死亡/病気/死者counter等の到達不能性はnative境界テストと人数上界で確認する。

[csv]: https://github.com/Igoorx/godfield-flash/blob/fcc83740a3d3cd9968e6e5cd2b7ed781f53abe80/server-src/ItemData.CSV#L118
[item]: https://github.com/Igoorx/godfield-flash/blob/fcc83740a3d3cd9968e6e5cd2b7ed781f53abe80/server-src/modules/item.py
[queue]: https://github.com/Igoorx/godfield-flash/blob/fcc83740a3d3cd9968e6e5cd2b7ed781f53abe80/server-src/modules/turn.py#L93-L338
[effect]: https://github.com/Igoorx/godfield-flash/blob/fcc83740a3d3cd9968e6e5cd2b7ed781f53abe80/server-src/modules/turn.py#L407-L527
[death]: https://github.com/Igoorx/godfield-flash/blob/fcc83740a3d3cd9968e6e5cd2b7ed781f53abe80/server-src/modules/room.py#L400-L435
[alive]: https://github.com/Igoorx/godfield-flash/blob/fcc83740a3d3cd9968e6e5cd2b7ed781f53abe80/server-src/modules/room.py#L108-L109
[dying]: https://github.com/Igoorx/godfield-flash/blob/fcc83740a3d3cd9968e6e5cd2b7ed781f53abe80/server-src/modules/turn.py#L38-L52
