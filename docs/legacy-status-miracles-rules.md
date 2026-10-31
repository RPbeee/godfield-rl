# 旧版の状態・解除奇跡5種

調査日: 2026-11-01。Igoorx非公式Flashサーバー公開masterを一次資料とする。
v36は202/208/211/212/224だけを追加し、Fog/Illusionの付与や助手を含めない。
現行公式との完全一致は主張しない。

## 台帳と型

全5種ともMAGIC、value0・hitRate0・weight1・防御roleなし。[CSV][csv]参照。

| local | ID | 属性 | attackKind / extra | MP | 価格 |
| --- | --- | --- | --- | --- | --- |
| 0 | 202 | NE | REMOVE_LOWER_HARMS | 3 | 10 |
| 1 | 208 | DARK | ADD_HARM / DARK_CLOUD | 5 | 10 |
| 2 | 211 | NE | REMOVE_ALL_HARMS | 5 | 15 |
| 3 | 212 | NE | ADD_HARM / COLD | 6 | 10 |
| 4 | 224 | NE | ADD_HARM / HEAVEN | 15 | 15 |

これらは通常の属性合成まで進むため、属性Noneの237/240/241/取引とは異なる。
208のDARKは直接状態付与の属性であり、ダメージ攻撃ではない。
[inflictDamage][effect]のDark即死は正damage条件下だけなので、208で通常HPを即死させない。
ただし相手がHeavenなら後述のaddHarm転落が起きる。

## 消費・保持・使用順序

[usePiece][player]の初回手札使用はMP所持を検査し、保持listへappend、手札remove、
無料でなければMP減算、その後[queueAttack][queue]がgift+1する。
同IDの複数初取得は保持copyを増やす。保持からのuseMagicは所持とMPを検査し、
MP減算だけを行い、保持copyを消費せずgift+1となる。
反応されて効果が届かなかった場合も初取得・MP支払い・gift予定を取り消さない。

[isValidAttackItem][valid]により、5種は単独MAGIC基底として使える。
selfは空応答で即解決し、他者は通常応答を待つ。病気/harmsが無い相手への解除も合法で、
hasDisease等を強制的な使用条件へ読み替えない。
通常boost・setter・追加197/217/214/209はMAGIC基底へ付けられず、5種を武器の追加魔法としても使えない。

FREEだけは既存のMAGIC使用後例外により追加できる。
最初の非先頭FREEが直前のMAGICを無料化し、初回保持取得/giftは無料でも発生する。
5種に列長earlybreakはなく、先頭MAGIC後のFREE列は実消費/gift後にcontinueする。
既存canonicalの手札基底最大15枚・保持基底最大16枚のFREE carrier上限をそのまま適用する。
初期MP10で224の有料使用は不可だが、手札224＋FREEなら使用できる。

## 防御・反射・跳ね返し・遮断

[defenderCommand][defense]で普通DFSを選ぶこと自体は属性適合下で合法だが、
もともと正のdamageを持たない5種を軽減せず、付与/解除は最終targetへそのまま実行する。
NE4種へ普通防具の属性は不問、DARK208も旧checkDefenseでは普通防御適合となる。
195は208をNEへ変えるが、DarkCloudというpayloadを削除しない。
反応せず通常防御したことからcounterを生成しない。

MAGIC型なので魔法反射・魔法跳ね返し・魔法遮断・196を全payloadに使える。
prefix普通防具の属性適合はMAGIC特殊の最終検査skipに従い、prefixの病気付与/消費は残る。
反射等は元payloadと元奇跡ID、from_ability、無料来歴を保持し、MP/保持取得/giftを再実行しない。
新targetの応答後に付与/解除する。bounce自己targetなら追加応答なしで解決する。
rate0なので命中抽選はなく、DarkCloudでもこの点は変わらない。

原典の一般defenderCommandには通常の防御role検査がなく、非防御MAGICをゼロDEFとして
消費し、MPを払い、保持取得/giftを得る入力が受理され得る。これも手札交換や保持獲得の
戦略的意味を持ち得るが、UI/公式の受理は未照合である。
v36では従来の非防御MAGICと統一してこの応答用途を除外し、main手札/保持castだけを採用する。

## 付与とHeaven

[Player.addHarm][player]の最初の分岐は対象の既存Heavenである。
208/212/224のいずれでもHP0・悪化率0となり、Heavenを保持してreturnする。
既にDarkCloudを持っていても転落し、incoming DarkCloudを新追加せず、incoming Cold/Heavenへ置換しない。

Heaven以外へ208を使うとDarkCloudを追加し、既に持つ場合は重複させない。
他の病気と悪化率は変更しない。次の％攻撃からDarkCloudによる命中抽選短絡が働く。
212/224の病気付与は既存病気rankとincoming rankを比較し、より高いincomingを採用し、
同じ/低いincomingなら既存病気を1段階進めて悪化率0にする。
従ってCold再付与はCold→Fever→Hell→Heaven、その後は転落となる。
224はHeaven以外ならHeavenへ上げ、既存Heavenへの再使用は転落する。

効果解決後の死亡/244を確認してから元turn ownerの病気tickへ進む。
self212初付与はその手番末にCold1ダメージ、self224初付与はHeaven5回復となる（悪化率初期0）。
反射後でもtick対象は元ownerであり、最終targetと同一とは限らない。
175prefix等でHP0となった人物の反応継続/244は既存の暫定HP0契約を維持する。

## 解除は付与と別処理

202はCold/Feverを消して悪化率0、Fog/Gloryを消す。Hell/Heaven/DarkCloud/Illusionは保持し、
Cold/Feverでなければ悪化率を勝手に0へ変えない。
211は全病気/harmsと悪化率を消す。解除はaddHarmではないため、Heavenへ使用しても転落しない。
元serverにはFog/Illusion解除時の表示再送もあるが、v36は未実装の付与源/UIを追加しない。

ordinary prefixで付いた状態も、後続の解除対象に入る。
例えば175でHeavenからHP0になり、211が解決してもHP自体は回復しない。
244があれば病気を解除した状態で復活し、その後のowner tickで旧Heaven効果を再適用しない。
selfのColdを202で解除すれば、その手番末のColdダメージを止める。
selfのHeavenへ202を使うとHeavenが残り5回復し、211なら残らず5回復しない。

## v36の能力枠と忘却

旧main能力16bit/既存indexを固定し、新しいstatus_abilities[5]を別配列とする。
CAST_STATUS(kind19)のbase0..4は表のlocal、target0/1、他fields0。
FREE保持使用はCAST_ORDERED base48..52とし、旧base32..47等を変更しない。
非防御MAGIC応答は追加しないので、反応中の有料MP支払いは従来どおり210だけ。
新5種の最安費用3は既存追加奇跡の最安2を下回らず、49有料保持使用の既存上界を拡大しない。

旧22種の忘却順定数を固定し、新flag時だけ新27種を昇順で重み付き抽選する:

`197,198,199,200,202,203,204,205,206,207,208,209,210,211,212,214,215,216,217,218,219,220,221,222,223,224,226`

各保持copyを等確率に選ぶ分布を保つ。取得順listのindexや元Python seedとの完全一致は従来どおり主張しない。
新counts0の旧profileでは旧順位/乱数経路を維持する。売買は手札copyを対象とし、
MP/保持取得/効果を発火させず、表の価格だけを新flag時のfallbackへ追加する。

受入条件: 全5種の自然pool、各MP・初取得/再cast・無料取得/保持FREE、普通DFS/195非軽減、
208Dark非即死、反射/bounce/blockと元費用保持、Heaven転落/低・全解除/tick順、
新能力の忘却、非防御応答scope差、旧36ABI境界・旧kind19拒否・バッチ原子的拒否。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV#L203-L225
[player]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L147-L301
[queue]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L96-L138
[valid]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L78-L95
[effect]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L477-L494
[defense]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L510-L643
