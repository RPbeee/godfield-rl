# 旧版の病気・悪化・継続効果・解除

調査日: 2026-10-31。根拠はIgoorxの非公式エミュレーターの可変`master`。
対象をCold/Fever/Hell/Heavenに限定し、Fog/Illusion/Glory/DarkCloud、守護神、蘇生、死亡時攻撃は含めない。
現行公式との一致は未確認。自然悪化の方式は[仕様差分表C02](official-spec-gap.md)の衝突が残る。

## 手番末の順序

[room.beforeEndInning/endInning/nextInning][room]では、攻撃の応答・連鎖を終えてから病気を処理する。
その対象は**元の通常手番所有者**（`turn.attacker`）だけであり、最後に防御した人や、反射後の現在の攻撃元ではない。

1. 既にHP0の人物について死亡処理を行う。
2. 攻撃キューに残りがあれば先に解決する。
3. 通常手番所有者が死亡確定しておらず病気なら、1回だけ自然悪化と継続効果を処理する。
4. 病気でHP0になったら死亡処理へ戻る。ただし、その手番の病気処理済みフラグにより2回目のtickはしない。
5. 対象外の守護神処理などを経て、生存者への予定分の配布を行う。
6. 生存者数によるゲーム終了を判定する。

病気処理の条件に「他の敵がまだ生存していること」はない。
従って、敵を倒した手番でも、元の所有者が生きていて病気ならtickする。
例えば、所有者がHP1/Coldで最後の相手を倒すと、その後のColdで所有者もHP0になり得る。
反対に、反射ダメージで所有者が既に死亡した場合、その所有者のtickはしない。
攻撃先に今回新しく病気を付与しても、その人が通常手番所有者でなければ今すぐはtickしない。

## 病気の継続効果と自然悪化

[player.diseaseEffect][player]は、Coldで1、Feverで2、Hellで5のダメージ、HeavenでHP+5を適用する。
回復上限は99。守護神が存在すればダメージに干渉する別処理があるため、今回は守護神なしに限定する。

自然悪化は[room.beforeEndInning][room]が管理する。
病気の開始・悪化時に悪化率カウンターを0へ戻す。
対象者の病気tickごとに0〜99の整数を1回取り、現在のカウンターより小さければ悪化し、
悪化しなければカウンターを1増やす。その後に**悪化後の病気**の継続効果を適用する。

| 悪化率カウンター | 判定 | 乱数消費 |
| --- | --- | --- |
| 0 | 自然悪化しない。失敗後1へ | それでも1回 |
| 1 | 0なら悪化、1〜99なら失敗 | 1回 |
| 99 | 0〜98なら悪化、99なら失敗して100へ | 1回 |
| 100 | 必ず悪化し、カウンター0へ | それでも1回 |

悪化はCold→Fever→Hell→Heaven→転落。Coldが自然悪化した回はFeverの2ダメージになる。
Heavenの悪化でHP0になった場合は、同じ処理でHP+5を与えて復活させない。

[公式画面の2026-09-20抽出記録][bible]は自然悪化を5%と説明しており、旧版の増加方式とは一致しない。
今回のデフォルトを旧版方式とするならrulesetへ明記する。
固定5%を実装する場合は別のConfig/profileとして区別し、同じ学習済み方策を黙って移さない。

## 新規付与・重複付与

順序は[Item.ALL_DISEASES][item]と[player.addHarm][player]による。
病気なしならincomingをそのまま採用する。
病気ありなら、incomingの段階が現在より上の場合はincomingへ進み、同段階以下なら現在を1段進める。
付与・悪化でカウンターを0へ戻す。

| 現在 / incoming | Cold | Fever | Hell | Heaven |
| --- | --- | --- | --- | --- |
| なし | Cold | Fever | Hell | Heaven |
| Cold | Fever | Fever | Hell | Heaven |
| Fever | Hell | Hell | Hell | Heaven |
| Hell | Heaven | Heaven | Heaven | Heaven |
| Heaven | 転落 | 転落 | 転落 | 転落 |

Heavenでは、病気以外のharmsを含む`addHarm`への入力も先に転落分岐へ入る。
旧版は999ダメージを与えてカウンター0にし、その分岐で戻る。
今回の守護神なし・HP上限99ならHP0になる。Heavenの文字列自体を先に消す処理ではない。
Fog等は今回付与しないが、後で追加するときにもこの先行分岐を忘れない。

## 付与・回復・解除の処理順

| 操作 | 旧版で確認できる挙動 | 根拠 |
| --- | --- | --- |
| ダメージ時付与の武器 | 防御後の残り攻撃力が正のとき、対象へ病気を付けてから通常ダメージを与える。全防御なら付かない | [turn.inflictDamage][turn] |
| 吸収等との順序 | 病気付与の後で、吸収による攻撃元の回復や自己ダメージ、対象へのダメージを処理する。今回これらの追加効果は対象外 | [turn.inflictDamage][turn] |
| 直接の病気付与 | ADD_HARMはダメージの有無とは別に対象へ付与 | [turn.inflictDamage][turn] |
| MP回復＋Heaven | MPを加算した後に病気を付与。旧ID234がこの形 | [turn.inflictDamage][turn]、[CSV][csv] |
| HP回復だけ | 病気や悪化率を消さない | [player.increaseHP][player] |
| 軽い病気の解除 | Cold/Feverなら病気とカウンターを消す。Hell/Heavenではどちらも保持する | [player.removeAllHarms(true)][player] |
| 全解除 | 全ての病気とカウンターを消す。Heaven解除は付与ではないため転落しない | [player.removeAllHarms(false)][player] |

ダメージ時付与の条件は、守護神による吸収後の実HP減少ではなく、その前の残り攻撃力である。
今回は守護神を含まないが、後で追加するときに条件を取り違えない。
反射・跳ね返しで解決対象が変わった場合、病気は**現在の対象**へ付く。
吸収効果の攻撃元も現在の攻撃元であり、元の通常手番所有者と同一とは限らない。

回復・解除の対象は行動で指定された人物。[turn.inflictDamage][turn]は対象へ適用する。
自分対象なら即解決、他者対象なら応答経路へ進む点は[中立処理メモ](legacy-neutral-rules.md)と同じ。
自分のColdを手番中に解除すれば、手番末には病気がないので自然悪化の乱数もtickもない。
Hellに対して軽い解除を使った場合はHellと悪化率を保ち、通常どおり手番末の処理を受ける。

旧CSVには、軽い解除の雑貨ID235、全解除の雑貨ID236、軽い解除の奇跡ID202、全解除の奇跡ID211がある。
雑貨と保持可能な奇跡は消費・MP・獲得処理が違うので、効果が同じでも同じ使用経路にまとめない。
今回の台帳へ採用する神器は、ほかの効果列も含めて別途照合する。

## 公開情報と初期化

[player.writeXML][player]は病気の種類を公開プレイヤー情報へ出す一方、悪化率カウンターは出さない。
Fog等なし・開始状態から全ての病気イベントと手番を観測する今回の限定環境なら、
カウンターはその履歴から追跡できる。
ただし、復帰途中の観測に真のカウンターを無条件で渡すことと、履歴から保持した値を渡すことは区別する。
将来隠蔽効果を含める場合、知識状態の扱いを再検討する。

初期状態は病気なし・カウンター0。効果付与後の手番末にも同じ処理を適用する。
直接自分をHeavenにした手番は、その終了時に自然悪化チェックとHP+5があり得る。

## 防御中の転落と、暫定HP0で続く連鎖

**HP0と死亡確定フラグは旧版では別である。**
[player.takeDamage][player]はHPを更新するだけで、死亡確定は[room.handlePlayersDeath][room]が行う。
従って、上の手番末の死亡処理は「HPが変化するたびに自動実行される」ものではない。

次の経路は旧サーバーのソース上、継続可能である。

1. Heavenの人物が無属性武器攻撃を受け、防御列の先頭に旧ID175を使う。
2. 175の防御時Fever付与がHeavenの転落を起こし、HP0になる。
3. 防御ループはHP0を理由に中断せず、後続の旧ID39も消費して反射する。
4. 無属性の元攻撃に対し、175の火と39の無属性の最終防御属性は無属性となり適合する。
5. 新しい対象の応答を待つため`defenderCommand`はfalseを返し、手番末の死亡処理にはまだ進まない。
6. 相手が反射し返すと、HP0だが死亡未確定の人物へもう一度応答が求められ得る。

根拠は[turn.defenderCommand][turn]。入口は現在の防御者との一致を確認するがHPを確認しない。
使用処理にもHP検査はなく、特殊反応後の判定は対象の死亡確定フラグを見る。
`getRandomAlive`もHPを調べず死亡確定フラグだけで候補を絞るので、
この暫定HP0の人物が跳ね返し先に入ることもある。[room.getRandomAlive][room]参照。

[Session.commandHandler][session]は反応がfalseで終わる場合に`nextInning`を呼ばない。
[User.parseXml][user]と`Session.onRequest`にもHP0を除外するガードはない。
従って、HP0でもサーバーの反応処理を続けられることは確認できる。
ただし、旧FlashクライアントのUIがその場面でどの入力を提示・送信できたかまでは、今回検証していない。

C++側では、連鎖中の暫定HP0を即終局とせず、解決境界で死亡を確定する必要がある。
snapshotの「非終局なら双方HP正」という条件も、この病気＋反応連鎖に限っては成立しない。
一方、任意の通常手番開始状態でHP0の人物を行動させてよいという根拠ではない。
暫定状態を許す範囲と、復元時の検証条件を明示して区別する。

## 同時全滅と引き分けの根拠

[room.handlePlayersDeath/checkEndGame/endGame][room]は、死亡者を処理し、生存者数が1以下なら終了を通知する。
0人生存を終了条件に含むため、病気tickによる両者HP0は終了し得る。
一方で、旧ソースにはこの場合を「引き分け」と名付けたり、RLの報酬0を定義したりする処理は見つからない。
SINGLE戦では死亡者ごとにlostを付けるため、両者lostにもなり得る。

C++側で`terminated=true, winner=-1, rewards=[0,0]`を採用するなら、
**生存者なしの終局を表現する学習環境の規約**として明記する。
公式の引き分けルールを確定した根拠としては扱わない。
自然に到達した両者HP0の終局と、任意の両者HP0 snapshotを開始状態として許すかも別の設計事項。
sourceはHP0だけで即勝敗判定せず、元所有者の病気tickまで解決してから終了を決める。

## 受入条件

1. 通常手番所有者だけが1回tickし、反射によるsource交換では対象を変えない。
2. 相手が既に倒れていても所有者のtickを行い、生存者なしの終局を表現できる。
3. 病気初回0%でも抽選値を1個消費。悪化時と失敗時のカウンター更新を区別する。
4. 重複付与を上の全表で検証し、Heaven転落を単なる回復状態の上書きにしない。
5. ダメージ時付与の後に通常ダメージを与え、全防御時は付与しない。
6. 軽い解除はHell/Heavenと悪化率を保持し、全解除は全て消す。
7. 病気tickによるHP0ではHeavenの回復を誤って続けず、死亡者へ補充しない。
8. 旧版の増加方式と固定5%をruleset・snapshot・評価結果で区別する。
9. Heaven＋175→39の連鎖中にHP0を暫定状態として保持し、再反射の応答・跳ね返し候補から早期に除外しない。

この調査ではC++の実装を変更していない。

[player]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py
[room]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py
[turn]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py
[item]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/item.py
[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[bible]: https://github.com/BrandenXia/godfield-bot/blob/main/data/snapshots/2026-09-20/bible.json
[session]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/session.py
[user]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/user.py
