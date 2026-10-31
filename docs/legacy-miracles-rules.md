# 旧版の単純奇跡: 保持・再使用・MP消費

調査日: 2026-10-31。対象はIgoorxの非公式エミュレーター。
今回扱うのは属性・追加効果のない回復と所持金増加の2種だけであり、現行公式の全奇跡対応ではない。
参照先は可変の`master`。台帳の出典管理は[dataの記録](../data/legacy-miracles-v1/manifest.json)も参照する。

## 対象データ

IDは旧`ItemData.CSV`の0始まりの行番号。[CSV][csv]と[読み込み規則][item]から確認した値。

| 旧ID | 種別・効果 | 値 | MPコスト | 価格 | 重み |
| --- | --- | --- | --- | --- | --- |
| 216 | MAGIC / INCREASE_HP | HP+10 | 6 | 15 | 1 |
| 220 | MAGIC / INCREASE_YEN | 所持金+15 | 8 | 25 | 1 |

この値を現行公式と呼ばない。[2026-09-20の公式画面抽出記録][bible]では
SpringはHP+10/MP7、Treasureは所持金+10/MP5で、旧データと一致しない。
今回の環境は旧版profileとして記録し、現行用の神器ID対応を確定したことにはしない。

## 使用時の処理

| 条件 | 確認した旧版の挙動 | 根拠 |
| --- | --- | --- |
| 手札から初使用 | 所有・MPを確認し、保持リストへID追加、手札から削除、MP減算の順。特殊な無消費効果がない場合はコストを払う | [player.usePiece][player] |
| 保持から再使用 | 保持していることとMPを確認し、MPだけ減らす。保持数は減らない | [player.useMagic][player] |
| 配布予定 | 手札使用でも保持からの再使用でも、通常の行動経路で配布予定数+1 | [turn.queueAttack][turn] |
| 回復・所持金の適用 | 指定対象へ加算し、資源上限99で止める | [turn.inflictDamage][turn]、[player.increaseHP / increaseYen][player] |
| 自分対象 | 反応選択を挟まず、空の応答で解決する経路 | [turn.doAttack][turn] |
| 他者対象 | 他者への行動として応答を待つ。対象へ与える効果であって使用者への効果ではない | [turn.doAttack / inflictDamage][turn] |
| MP不足・0MP | 特殊な無消費効果を使わない候補2種では支払不可。手札使用も保持使用もMP確認で拒否 | [player.usePiece / useMagic][player] |
| 死者 | 通常経路は行動元・対象が死亡確定していないことを要求。特殊な強制処理は別 | [turn.queueAttack][turn] |

初使用の手札数は消費で1減り、手番末の配布で通常は戻る。
再使用は手札を消費せず配布予定だけ増えるので、受入可能なら手札数が1増える。
配布タイミング・手札上限・満杯でも抽選する点は[中立処理メモ](legacy-neutral-rules.md)と同じ。
この挙動を「再使用は補充なし」と実装しない。

旧ソースはassertを使い、複数選択全体の原子的な拒否を保証しない。
C++側では全選択・MP・対象の検証を先に済ませ、失敗時に手札・保持・MP・配布予定・乱数を変えない。
今回の候補はそれぞれ単独使用とし、無消費化・追加効果の組み合わせを黙って許可しない。

## 重複・祈り・捨てる

| 項目 | 旧版から確認できる範囲 | 根拠 |
| --- | --- | --- |
| 重複取得 | 初使用時のappendに重複除外なし。保持リストに上限チェックもない | [player.usePiece][player] |
| 所持判定 | IDが保持リストに存在すれば使用可能。重複数をコスト倍率には使わない | [player.hasMagic / useMagic][player] |
| 祈り | 手札の武器の有無で判定。保持奇跡が使用可能かどうかは、この祈り判定に入らない | [turn.queueAttack][turn] |
| 手札の奇跡を捨てる | 通常の捨てる処理は手札から削除する。初使用ではないため保持追加・MP消費・配布予定増加はない | [turn.inflictDamage][turn] |
| 保持奇跡を捨てる | 通常の手札DISCARD経路とは別。旧版の忘却効果が保持リストから削除する | [turn.inflictDamage / REMOVE_ABILITIES][turn]、[player.discardMagic][player] |

「武器を所持するがMPが不足している」と「保持奇跡だけ所持してMPが不足している」を区別する。
後者は奇跡が使えなくても、武器不所持なら祈り可能。
候補2種に防御用の役割はないため、今回それらを防御行動へ追加する根拠にはしない。

## 公開情報と保持の圧縮

再入室時の完全な保持リストは本人の`privatePlayer`へ書かれ、通常の公開プレイヤー情報には含まれない。
使用した神器のIDは行動として送られるので、「全保持情報が常に公開」と「何も推測できない」のどちらでもない。
観測は自分の保持情報と、相手について観察済みの履歴を分ける。
根拠: [room.addUser][room]、[player.writeXML][player]、[turn.doAttack][turn]。

今回の**2種・単独使用・保持の忘却なし・種類を指定する意味的行動**という範囲なら、
`count216/count220`で保持の多重集合を表せる。次の遷移で必要なのは所持判定と初使用時の増加であり、
取得順や同じ種類の何番目を使ったかは、今回の効果・コスト・配布結果を変えない。
boolだけにすると重複取得の記録を失うので、カウントを持つ。

これは旧XMLを完全再現する表現ではない。旧クライアントの`abilityIndex`は取得順の位置であり、
カウントだけから元の位置へ戻せない。[commandPieceの変換・出力][piece]と[room.addUser][room]に位置表現がある。
将来忘却を追加する場合、`getRandomMagic`は保持リストから1要素を無作為に取るので、
種類別の抽選確率には重複数を反映する必要がある。カウントによる抽選が同分布でも、
旧版と同じ乱数で同じ種類が出るとは限らない。[player.getRandomMagic / discardMagic][player]参照。

保持数を16などに勝手に制限すると旧版とは異なる。
内部カウンターには明示的な整数範囲を設け、オーバーフローを防ぐ。初期保持0から開始し、
意思決定上限を設けた本subsetでは、初使用回数は意思決定数以下なので、その上限に基づく検証が可能。
外部snapshotの復元には所有数の型・到達可能性も含めて検証する。

## 実装前の受入ケース

1. 手札ID216・MP6: 使用でMP0、保持216が1増え、手札消費1・配布予定1。HP加算は対象へ行く。
2. 保持216・MP6: 使用でMP0、保持数不変・手札消費0・配布予定1。
3. 手札／保持216・MP5: どちらも拒否し、全状態・乱数を維持。
4. 手札ID220・MP8: 所持金+15、99で上限、MP0、保持220追加。
5. 同じ奇跡を手札から2回取得: 保持カウント2。同じ奇跡の再使用でその数は減らない。
6. 武器不所持で保持奇跡あり／なし、MP0／十分: どの場合も保持の有無だけでは祈り可否を変えない。
7. 手札奇跡を捨てる: 保持0のまま、MPも変化せず、配布予定増加0。
8. 他者対象: 使用時にコスト・保持化を処理し、効果は相手の応答後に対象へ適用。
9. 相手の隠れた保持数だけが異なる状態: 公開情報が同じなら即時観測に差を出さない。
10. 手札満杯で保持奇跡を再使用: 所持手札は増えなくても、予定された抽選結果は消費する。

これはソース読解による受入条件であり、旧エミュレーターや現行公式を実行して採取したfixtureではない。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[item]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/item.py
[player]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py
[turn]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py
[room]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py
[piece]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/commandPiece.py
[bible]: https://github.com/BrandenXia/godfield-bot/blob/main/data/snapshots/2026-09-20/bible.json
