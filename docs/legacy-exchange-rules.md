# 旧版の資源交換・蘇生・死亡時攻撃の候補

調査日: 2026-10-31。Igoorxの非公式Flashサーバーエミュレータ公開masterを対象とする。
公式完全再現ではなく、現duel subsetへ独立追加できる範囲の判断資料。
売買、Fog、Illusion、第三者戦は今回の実装候補から外す。

## 正重量台帳

[ItemData.CSV][csv]と[読込/ATK/DEF処理][item]による。

| ID | type | attackKind | attackExtra | 属性 | value | subvalue | 率 | MP費用 | 価格 | 重み |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 2 | TRADE | EXCHANGE | 空 | NE | 0 | 0 | 0 | 0 | 10 | 20 |
| 117 | WEAPON | ATK | DYING_ATTACK | LIGHT | 1 | 30 | 75 | 0 | 10 | 1 |
| 244 | SUNDRY | 空 | REVIVE | NE | 10 | 0 | 0 | 0 | 10 | 1 |

いずれもdefenseKind/defenseExtraは空。117のsubvalue30はMAGICの費用ではない。
今回の検索対象EXCHANGE、REVIVE、DYING_ATTACKについて正重量はこの3種だけ。

## 2: 自分のHP・MP・所持金を再配分

[Session.commandHandler][session]はpowerのHP/MP/YENを整数として読み、
[queueAttack][exchange]は単独1枚・指定3値の総和と使用前総和の一致だけを検査する。
神器消費・deal+1の後、isAction=trueとなりtargetを自分へ正規化する。
空防御で[inflictDamage][assign]が3値を直接代入する。相手への応答機会、反射、
MP費用、奇跡保持化、通常boost/setter、FREE併用は発生しない。
COMMANDのpower3値は全userへ公開される。

**旧serverには各値の非負/99以下の検査とclampがない。**
例えば元(40,10,20)から(-1,71,0)も総和だけなら通る。
これを通常の合法資源値と同一視しない。クライアントUIが許す範囲はこのソースからは確定できない。
[同梱bot][bot]は概ねHP30→MP30→YEN20を優先し、余剰をHP99→MPへ割り当てるが、
botの方策はserverの合法手定義ではない。

推奨するcanonical subsetは各値0..99、総和保存、handbase2単独、自分対象とする。
この制約はserverの全入力受理範囲より狭いことを明示する。
価格10を総和から引く、YENを先に払う、MPを消費する、といった処理は誤り。
同じ3値への再配分もソース上は許可される。

Gloryは防御選択の制約なのでこの単独行動を妨げない。Heavenも維持され、
交換そのものはaddHarm/回復/解除ではない。HP0を選んだ場合はitem終了後の死亡確定へ進む。
HP正ならownerの病気tickが後から発生するので、最終観測の総和はtickで変化し得る。
例えばColdのownerがHP1へ交換すると、その後の1tickで死亡する。

## 244: 死亡確定前の自動蘇生

[handlePlayersDeath][death]はroom.players順にHP0・dead=falseを走査する。
dead化する前のgetAliveCount（HPでなくnot.dead数）が2以上で手札244を持つと、
[playerDyingAttack][dying]を呼んで最初の244を直接discardし、HPを10増やしてcontinueする。
deal+1はない。MP、YEN、保持奇跡、病気、harms、worseChanceは変更しない。
この経路は乱数・応答・反射・FREE・Gloryの選択制約を通らない。

通常USEはattackKind空のSUNDRYなので[初手妥当性][valid]が拒否する。
手動DISCARDはserver上可能（除外されるのは245MORTAR）。botは捨てない方策を取る。
同種244が複数あっても、1回の死亡境界で使うのは最初の1個だけ。

[beforeEndInning][finish]の死亡走査は、次queue itemやowner病気tickより前。
したがって蘇生後のHP10で予約counterを受ける。反射中の暫定HP0ではまだ蘇生しない。
病気tickでHP0なら再帰した死亡走査で蘇生し、handledDisease=trueなので同じtickを繰り返さない。
Heaven悪化で落ちてもHeavenを消さずHP10へ戻す。これは一旦deadとなった人物の復活ではない。

同時HP0には走査順の非対称がある。両者dead=falseのduelで、seat0に244がなければ
seat0がdeadとなり、seat1は244を持っていてもalive数1なので発動できない。
seat0に244があればalive数2を保てるためseat1にも蘇生機会が残る。
両者が244を持てば両者HP10になる。この条件をHP>0人数に置き換えてはいけない。

## 117: 通常武器と死亡時の二つの役割

通常はLIGHT ATK1・75%のWEAPON。指定対象は％武器処理で生存敵へ展開され、
通常使用は手札1消費・deal+1、MP不要。baseのhitRateが正なので追加神器は不可。
MAGIC反射/遮断には該当せず、既存の属性防御・195・196と物理特殊規則に従う。

死亡時は[death][death]が先にdead=trueを設定し、その後でalive数2以上なら[dying][dying]へ進む。
この順序から、**2人duelでは117の死亡時発動は到達不能**。
死者を除いた後も2人以上残る必要があり、3人以上戦の効果である。

発動する場合はforced queueでraw30へ上書きし、LIGHT・75%で生存敵全員へ積む。
手札117自体の消費もdeal+1もない。sourceはdeadのまま、既存FIFOの後ろへ予約される。
命中抽選は各対象で行い、DarkCloudは既存の短絡規則。
196で死者sourceへ返れば反射先deadの終了条件により効果を適用しない。
原通常itemと死亡時raw30を取り違えず、MPや保持奇跡の仕組みに入れない。

## 採用順の提案

v25は**交換2単種**を推奨する。入力3値を意味Actionへ持たせるだけで閉じ、
追加の保留item/乱数/保持容量を必要としない。費用なしの総和保存、自分即時解決、
死亡・病気・補充の既存終端経路を使える。正重量20の追加で配布分布が変わる点も記録する。
新profileだけに追加し、旧ABIのAction尾部と観測幅は固定する。

244も単種の独立scopeにはできるが、死亡走査を現server順に合わせることが必要。
蘇生消費を補充しない点、複数244、同時HP0、owner病気tick、Heaven、予約counterの
前後を受入条件に含めるべきで、交換と同時に導入する必然性はない。
117はduelでは死亡効果を持たない確率武器として閉じるものの、蘇生実装の代替にはならない。
第三者戦へ進む際は死亡専用FIFO・勝敗判定・deadsource応答を改めて設計する。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[item]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/item.py#L27-L42
[session]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/session.py#L275-L304
[exchange]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L110-L147
[assign]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L318-L403
[bot]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/bot.py#L600-L632
[death]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py#L369-L399
[dying]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L35-L48
[valid]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L78-L94
[finish]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py#L426-L474

## v25の確定Action契約

gg ABI25は既存88byte配置を維持し、EXCHANGE(kind14)を追加する。
baseは手札2のslot、targetは255、slots/counter fieldsは0、extra_countは3。
extras[0..2]を最終HP/MP/YENの各0..99とし、合計を現在資源の合計と一致させる。
未使用extras、65th tail、reservedは0。他kindのextrasは従来の参照として解釈する。
HP0も許し、手札1消費・gift1、相手応答なしで死亡・病気・補充の既存終端へ進む。
LastExchangeは使用前後3値とseatを公知履歴として保持する。
244と117は資料としての調査だけで、このprofileの台帳・自動死亡処理には追加しない。
