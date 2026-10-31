# 旧版の忘却 REMOVE_ABILITIES

調査日: 2026-10-31。対象はIgoorxの非公式Flashサーバーエミュレータの公開master。
現行公式の完全再現とは区別する。v23までの保持奇跡に、241だけを追加する判断資料。

## 神器と直接効果

[旧CSV][csv]でREMOVE_ABILITIESを持つ正重量神器は241だけ。

| ID | type | attackKind | ATK | Card属性 | value | MP | 命中率 | 価格 | 重み |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 241 | SUNDRY | REMOVE_ABILITIES | 0 | 空欄/NE | 1 | 0 | 0 | 10 | 3 |

attackExtra・defenseKind・defenseExtra・subvalueは空。250は同じ効果のCREATUREだが重み0なので除外。
MAGICや正被害時追加効果としてのREMOVE_ABILITIESは、この台帳にはない。

[queueAttack][queue]は241を1枚だけの列に制限し、消費・配布予定+1の後で早期終了する。
MP支払・保持化・FREE併用はない。自分指定も可能で、その場合は空応答で即時解決する。
[inflictDamage][forget]は正被害判定とは独立に1個だけ忘却する。
value回のループ、raw威力、残HPやHPclipによる個数変更は存在しない。

## 抽選・重複・公開結果

[Player][player]は初使用MAGICのIDを取得順にmagicsへappendする。
getRandomMagicは空リストなら0を返し、非空ならrandom.choiceを1回呼ぶ。
同種重複は別要素なので、種類の抽選確率はその所持個数/総所持個数。
削除は選んだIDの最初のcopyに対するlist.removeで、後続copyは1個ずつ残る。
複数削除処理はなく、将来の別の241は減少後の集合から改めて抽選する。
非空1個でもrandom.choiceの呼出しを省略しない。空なら忘却抽選の乱数消費はない。

結果IDと削除前の最初の同IDのabilityIndexをcommandChain.pieceに設定する。
[非self応答][response]は全体broadcast、self解決は[doAttack][attack]の全user向けCOMMANDに載る。
空集合なら結果pieceはなく、本人だけに秘密通知される仕様ではない。
[CommandPiece.writeXML][piece]がIDとabilityIndexのwire形式を定める。
一方、再入室時の保持全リストは[RoomのprivatePlayer][private]へ取得順で送られる。

## 防御・反射・状態の境界

重要: Cardの空欄属性と、実行中AttackDataの属性なしは別。
[AttackData初期値][attack-data]はdamage=-1、chance=0、attribute=None。
241の早期終了は通常の属性合成より前なので、実行中もNoneのままになる。
[checkDefense][attribute]はNoneを常に拒否し、195もNoneにassertする。
したがって通常防具、counter、195、普通prefix付き196は合法にならない。
[特殊判定][response]では先頭196だけが最終属性検査を免除して反射できる。
SUNDRYなのでWEAPON用20/39/215、MAGIC用40/210/BLOCK_MAGICは一致しない。
反射はsource/targetだけを交換し、忘却抽選は実際の最終対象への解決時に行う。
241自体はchance0なので命中抽選なし。195によるNE化を許可してはいけない。

忘却はharm付与ではなく、Heavenの即HP0・病気counter reset・解除を起こさない。
忘却でMP/HPは変わらない。196の消費・配布、解決後の死亡確定・owner病気tick・補充は
既存の[Room終端処理][finish]に従う。現scopeでは合法な病気防具prefixを作れない。

## 個数配列で再現できる範囲

現在の保持21種類（旧main15、防御2、追加2、倍化1、全体化1）を統一して抽選対象にする。
手札中でまだ初使用していないMAGICは対象外。ある種類の個数が0になれば、
その保持参照は有料・無料のいずれでも無効になる。FREEは所持確認を免除しない。
忘却対象を旧main15だけに限定するのは誤り。

個数ベクトルをc、総数をTとし、0..T-1を均等抽選して固定順の累積個数で種類を選び、
当該個数だけ1減らせば、種類別の状態遷移分布は旧リストと同じになる。
例: [197,216,197]は197が2/3、216が1/3。どちらの197を引いても残る種類個数は同じ。
次の241も更新後の個数から抽選するため、同一copyを二重削除しない。

ただし**元Pythonと同じseedで同じ種類になること、取得順、abilityIndexの完全再現は不可**。
旧リストでは[197,216]と[216,197]が同じ個数でも、抽選index0の種類が異なる。
これらを契約に含めるなら取得順のリスト/圧縮列を別途保存する必要がある。
今回の推奨scopeは種類・重複数・遷移確率と公知結果IDを再現し、wireインデックスは含めない。

pending_deals/retained_dealsは既に獲得した補充予定の会計であり、保持個数そのものではない。
忘却で保持を1減らしても過去の保持再使用による配布予定を取り消さない。
取得時刻やcopy個体を効果に使う未対応神器を追加する場合は、この圧縮根拠を再検討する。

## 最小受入条件

- 241を通常poolへ追加し、自分/相手の保持0でも使用可能、手札1消費・配布+1。
- 空は抽選なし、非空は1抽選。全21種類を個数重みで選び、重複は1個だけ削除。
- 最後のcopy削除後は有料cast・FREE付きcast・ordered内保持参照を拒否し、手札初取得は引き続き可能。
- 通常防具・195・prefix付き196を拒否、先頭196の往復では最終対象だけ抽選。
- 忘却結果IDは公開し、将来の抽選値や相手保持全リストは観測に追加しない。
- 旧profileの乱数系列・観測寸法・ABIを変えず、新profileの独立抽選器/テープは検証用に秘匿する。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV#L242
[queue]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L86-L163
[forget]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L383-L445
[player]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L233-L301
[response]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L511-L644
[attack]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L318-L380
[piece]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/commandPiece.py#L49-L57
[private]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py#L140-L151
[attack-data]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L31-L51
[attribute]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/item.py#L129-L148
[finish]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py#L370-L474
