# 旧Flash版・中立神器の処理メモ

調査日: 2026-10-30。対象はIgoorxの**非公式エミュレーターの挙動**であり、現行公式の確定仕様ではない。
中立の単純武器、攻撃追加、防御、HP/MP回復を実装する際の根拠を整理する。
引用元は可変の`master`なので、採用時には参照commitを固定する。

## 確認した規則

| 項目 | 旧版実装の挙動 | 根拠 |
| --- | --- | --- |
| 通常の行動順 | 全員の順序を周期ごとにシャッフルし、その順に行動する。2人でも周期境界では同じ人が続けて行動し得る。反応する防御者は通常順を決めない | [room.pyのresetAttackOrder / selectNewAttacker / startInning][room] |
| 初期状態・容量 | HP40、MP10、所持金20、初期の配布数10。手札の受入上限は16 | [player.pyのreset / dealItem][player] |
| 消費と配布 | 通常の攻撃用神器を使うたびに配布予定数が1増える。単純な防御でも選択した神器ごとに1増える。配布は空枠を満たす処理ではなく、手番末に予定数を抽選する | [turn.pyのqueueAttack / defenderCommand][turn]、[room.pyのdoPlayersDeals / endInning][room] |
| 満杯・終局時の配布 | 予定数の抽選後、16枚を超える受入を拒否する。満杯だから抽選自体を省くわけではない。死亡者には配らず、生存者の配布後に勝敗判定へ進む | [room.pyのdoPlayersDeals / nextInning][room]、[player.pyのdealItem][player] |
| 手札位置 | 通常の使用・捨ては所持リストから削除し、獲得は末尾へ追加する。したがって旧版は消費後に手札が詰まる | [player.pyのusePiece / discardPiece / dealItem][player] |
| 複数捨て | 固定の捨てる操作に1枚以上の所持品を付ける。選択品を削除するが、この操作は配布数を増やさない。サーバー側のこの経路は武器の捨て禁止を検証していない | [turn.pyのqueueAttack / inflictDamage][turn] |
| パス | `DO_NOTHING`は武器を持たない場合のみ。配布数を1増やす | [turn.pyのqueueAttack][turn] |
| 単純中立防御 | 選択順に消費して防御値を引き、残り攻撃力を0以上に保つ。0になった後も単純防具の選択列を最後まで処理する | [turn.pyのdefenderCommand][turn] |
| 攻撃組合せ | 先頭の武器と後続を区別する。必中武器の後ろに許可された追加効果を付ける。通常の基底攻撃2枚の合算は不可。一方、攻撃追加武器を先頭にすること自体は可能 | [attackData.pyのisValidAttackItem][attack] |
| 回復対象 | HP/MP回復は指定した対象へ適用する。自分対象なら即解決の経路、他者対象なら反応待ちの経路へ進む | [turn.pyのdoAttack / inflictDamage][turn] |
| 回復上限・兼用防御 | HP/MPの増加は99まで。武器の防御値は攻撃値と別のデータ欄を使う | [player.pyのincreaseHP / increaseMP][player]、[item.pyのgetDef][item] |

## 中立環境で必要な実装条件

- 通常行動者と反応中の行動者を別に保持する。
- 手札容量、初期配布枚数、未処理の獲得予定数を分ける。
- 取り消された非合法手が手札・獲得予定数・乱数を変えないよう、消費前に全選択を検証する。
- 攻撃先頭と追加分を表せる行動にする。単なる選択集合だけで順序の意味を失わない。
- 防御中に攻撃力が0になったことを理由に、選択済みの通常防具を温存しない。
- 旧版どおりの手札整列を採用するなら、処理中にスロットがずれないよう、元の選択からまとめて削除する。
- 回復を自分限定にする場合は、環境の制限として明記する。

## 照合ケースの候補

| ケース | 操作 | 期待する旧版の結果 |
| --- | --- | --- |
| R01 | 通常武器2枚を同時に使用 | 非合法。状態を変えない |
| R02 | 通常武器を先頭、攻撃追加武器を後続に使用 | 両方を消費し、加算された攻撃を開始。攻撃側の配布予定は2 |
| R03 | 攻撃追加武器1枚を先頭として使用 | 先頭が武器であるため、この理由だけでは拒否しない |
| R04 | 攻撃力を超える通常防具を2枚選択 | 両方消費、配布予定2、残り攻撃力0 |
| R05 | 武器あり／なしでパスを試す | ありは拒否、なしは配布予定1 |
| R06 | 非武器2枚を捨てる | 選択品を削除。捨てたことによる配布予定増分0 |
| R07 | 防御を終え、次の通常手番を開始 | 通常順の次の人物。常に防御者になるとは限らない |
| R08 | HP98へHP+5 | HP99 |
| R09 | 消費してから獲得 | 残った手札の相対順を保ち、獲得品を末尾へ追加 |

これらはソースから導いた期待値。実際にエミュレーターを実行して結果を保存したfixtureではない。
現行公式との一致を示すテストとも区別する。

## 採用前に残す確認

[仕様差分表](official-spec-gap.md)のC01（光防御）、C02（病気悪化）、C03（開始手札）は未解消。
また、旧版が検証していない「捨て禁止」を合法化しない。神器別制限はBible記録を別の根拠として使う。
神器分布と霧は作者自身の[既知の不具合][issues]に含まれるので、正解データとして無条件に採用しない。
旧版の初期値と容量を使う環境は、その旨をrulesetに明記する。

[room]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py
[player]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py
[turn]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py
[attack]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py
[item]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/item.py
[issues]: https://github.com/Igoorx/godfield-flash#known-issues
