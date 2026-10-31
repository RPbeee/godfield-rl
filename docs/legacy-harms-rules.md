# 旧版harms: Glory・DarkCloud、霧・幻覚の留意点

調査日: 2026-10-31。根拠はIgoorxの非公式エミュレーターと、第三者による公式画面抽出記録。
今回の調査では実装を変更していない。Cold/Fever/Hell/Heavenは[病気メモ](legacy-disease-rules.md)を参照。

## Gloryの1枚防御と、現在のFlashという名称

[turn.defenderCommand][turn]は選択列を処理するが、Gloryの所持を調べる1枚制限は確認できなかった。
[Session.commandHandler][session]も、その制限を検証せず反応処理へ渡す。
従って、旧サーバーの入力検証がGlory時の複数防御を必ず拒否するとは言えない。

一方で[bot.buildDefense][bot]にはGloryの有無を調べ、通常の複数防御候補を1枚に絞る処理がある。
属性除去の神器を他の防具と組み合わせる案も、Glory時には避ける。
したがって「1枚制限はUIだけにある」とも断定できず、**botの行動生成には制限があり、
サーバーの手動入力検証には見当たらない**、というのが確認できた範囲である。
旧FlashクライアントのUIは今回実行・解析していない。

[2026-09-20の公式画面抽出記録][bible]はFlashを防御1枚の状態と説明している。
botのGlory制限はその対応候補を支えるが、旧GLORYと現在のFlashの名称・神器IDを
直接照合した証拠にはならない。「旧GLORY＝現在Flash」と確定したデータ変換はまだ作らない。

実装する場合は、次のどちらを再現するか明記する。

- 旧サーバーの受理挙動: 複数防御を拒否する検証は根拠不足。
- 1枚制限のある意図されたゲーム規則: botの制限と現在の説明を根拠に採用するが、旧サーバーの完全再現とは区別。

後者なら、防具maskの枚数だけでなく特殊応答のbaseも1枚として数える必要がある。
「通常防具1枚＋反射1枚」を1枚防御として許可しない。

## DarkCloudと命中抽選

[turn.doAttack][turn]は対象がDarkCloudを持つ場合、missを直接falseにする。
通常の命中判定式を評価しないため、**命中乱数を消費しない**。

| 条件 | 結果 | 命中乱数 |
| --- | --- | --- |
| 正の確率武器、DarkCloudなし | 確率値で判定 | 1回 |
| 確率100、DarkCloudなし | 必中 | 1回 |
| 確率0の旧必中sentinel | 必中 | 0回 |
| DarkCloudあり | 確率にかかわらず必中 | 0回 |

DarkCloud確認は、そのキュー項目の対象への命中判定時点で行う。
命中後の防御・効果付与はそれより後なので、そこで状態が変わっても終えた抽選をやり直さない。
DarkCloudを攻撃時付与する神器自体も、付与前の対象状態で最初の命中を判定する。
防御中に付与される状況へ拡張する場合にも、その付与を現在の抽選へ遡及適用しない。
反射・跳ね返しはそもそも命中判定へ戻らないため、新対象のDarkCloudを理由に新しい抽選を行わない。
根拠: [turn.doAttack / defenderCommand / inflictDamage][turn]。

## 付与・重複・Heavenとの相互作用

[player.addHarm][player]ではharmsを病気とは別のリストに保持し、同じharmを重複追加しない。
通常は、新しいharmの追加でも同じharmの再付与でも、病気の悪化率カウンターをリセットしない。
GloryやDarkCloudそのものに病気のような手番末ダメージ・自然悪化の処理は確認できない。

ただしHeavenは最初に別分岐へ入る。**既に所持しているharmの再付与であっても**、
重複排除より前に999ダメージと悪化率0が適用される。
今回の守護神なし・HP上限99の範囲ならHP0になる。
「重複harmなら何もしない」と先に戻る実装では、この転落を失う。

攻撃のダメージ時付与は、防御後の残り威力が正のとき、通常ダメージより先に対象へ付ける。
防御で全て止めた場合は付かない。直接のADD_HARMはダメージ条件とは別。
反射後は現在の解決対象へ付ける。[turn.inflictDamage][turn]参照。

## 解除と判定用関数

| 操作 | 病気 | harms | 悪化率 |
| --- | --- | --- | --- |
| 軽い解除 | Cold/Feverだけ解除 | Fog/Gloryだけ解除。Illusion/DarkCloudは残る | Cold/Feverを消したときだけ0。Hell/Heavenは保持 |
| 全解除 | 全て解除 | 全て解除 | 0 |

[player.hasLowerDisease][player]はCold/FeverかFog/Gloryがあると真、
`hasDisease`は病気か何らかのharmがあると真となる。
これらの補助判定を、薬が使える対象をサーバーが必ず制限する条件だとは読み替えない。
解除は`addHarm`ではないため、Heavenの全解除自体は転落を起こさない。

## 神器の候補と現在の対応範囲

[旧CSV][csv]では、DarkCloudの無属性武器ID43（ATK11）が単純な候補になる。
Gloryの確認できた付与元は、ID70の光武器、ID191の光反撃、ID200の光の確率奇跡。
現在の無属性＋4属性・反撃未対応・単純奇跡2種という範囲には、これらのGlory付与元は入らない。
そのためGlory状態の制限だけをfixture経由で実装するなら、自然な獲得・付与経路は未対応と明記する。
光属性や反撃の付随効果を消して、無属性の簡略神器として混ぜない。

## 霧: 後続実装の注意

作者は[READMEの既知の不具合][issues]で霧が不完全と記載している。
旧版をそのまま「公式の秘匿仕様」として採用しない。
攻撃元に霧があり自己対象でない行動では、旧`queueAttack`が対象を取り直すが、
敵を指定した場合は敵候補、味方を指定した場合は味方候補という区分がある。
自己対象は同じ再選択経路ではない。[turn.queueAttack][turn]、[roomの対象抽選][room]参照。
これは行動先の処理であり、何が画面から見えなくなるかの仕様を全て確定するものではない。

## 幻覚: 真値・表示値・合法手

[player.dealItem][player]は条件を満たす通常獲得で見かけの神器を別に保持し、
本人への表示にはその見かけを使う。`getIllusionForItem`は互換候補を探すので、
「50%なら必ず違う神器へ変わる」とは限らない。[item.py][item]参照。

行動時は[turn.convertPiecesToOwnedPieces][turn]が所持する実体へ戻す。
実体を見つける際には見かけのIDや同名の見かけの何番目かも扱う。[player.getOwnedPiece][player]参照。
その後の攻撃検証・効果解決は真の神器データを使う。
ゆえに「表示のIDをそのまま真のIDとしてstepする」実装では再現できない。

一方、ポリシーへ真値を渡したり、真値にだけ依存する合法手マスクを出したりすると、
本人が表示から知り得ない情報を漏らす可能性がある。
表示上の選択と実際の効果、表示から構成できる候補と最終的な合法性、履歴からの知識を分けて設計する。
見かけ候補の互換条件があるので全てのマスクが必ず変わるとも断定しないが、
「互換だから真値を使っても情報漏れしない」と無検証で扱わない。
今回のGlory/DarkCloud追加だけで霧・幻覚まで対応済みとは報告しない。

## 受入条件

1. DarkCloudでは命中tapeを消費せず、解除後に未消費の同じ抽選を使える。
2. ダメージ時付与と直接付与を区別し、現在の抽選へ遡及適用しない。
3. harmの重複付与は通常idempotentだが、Heavenの転落判定は先に実施する。
4. harm付与だけでは病気の悪化率を変えない。
5. 軽い解除と全解除の対象集合・カウンター更新を区別する。
6. Gloryの1枚制限を採る場合、根拠と旧サーバーとの差、名称対応の未確定を明記する。
7. 未対応のGlory付与神器、霧・幻覚、光・闇を台帳や合法手に紛れ込ませない。

[player]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py
[turn]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py
[room]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py
[session]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/session.py
[bot]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/bot.py
[item]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/item.py
[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[issues]: https://github.com/Igoorx/godfield-flash#known-issues
[bible]: https://github.com/BrandenXia/godfield-bot/blob/main/data/snapshots/2026-09-20/bible.json
