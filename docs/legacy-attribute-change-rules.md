# 旧版の属性付け替えと属性除去

調査日: 2026-10-31。非公式Flashサーバーエミュレーターのwebソース読解。
公式の現行実測ではない。可変`master`の参照なので、台帳には取得物のhashと取得範囲を残す。
通常合成は[属性メモ](legacy-elements-rules.md)、光闇は[光闇メモ](legacy-light-dark-rules.md)を参照。

## 狭い候補scope

[旧CSV][csv]のADD_ATTRIBUTEは65/85の2種。v11はこの2種だけを対象とする。
関連する応答時の属性除去195は別段階の参考として記録し、今回のプールには入れない。
[Item.fromData][item]の列解釈による。倍率・全体化等も加えない。

| 旧ID | 種別 / 主な列 | 属性 | 先頭ATK | 追加時のATK加算 | 価格 | 重み |
| --- | --- | --- | --- | --- | --- | --- |
| 65 | WEAPON / ATK / ADD_ATTRIBUTE | 火 | 2 | 0 | 15 | 1 |
| 85 | WEAPON / ATK / ADD_ATTRIBUTE | 水 | 5 | 0 | 15 | 1 |
| 195 | PROTECTOR / defenseExtra=REMOVE_ATTRIBUTE | 無属性 | なし | なし | 15 | 3 |

65/85は防御Kind・防御Extra・防御値を持たず、通常防具ではない。
武器なので先頭攻撃として使用でき、必中武器の後ろへ属性setterとして追加できる。
通常DISCARDの対象にはならない。195はATKもDFSもなく、応答で属性を除去する役割。
195を通常DEF付き防具へ変換しない。

## ADD_ATTRIBUTEの威力と処理順

[queueAttack][attack]では、INCREASE_ATKだけが明示的に威力を加算する。
ADD_ATTRIBUTEはその加算分岐に入らず、ATKの初期値が未設定なら先頭の値を設定するだけ。
したがって追加部品として使った65/85は、表示に対応しそうな値2/5を威力へ足さない。
これは旧サーバーの実装結果であり、意図した旧ゲーム仕様か不具合かは未検証。

各要素を消費・配布予定+1してから属性を合成し、ADD_ATTRIBUTEを見た位置で属性を強制上書きする。
通常boostが後ろにあれば、その新しい属性へ通常の混合規則を再適用する。
混合済みの無属性でもsetterで火/水へ復活する。

| 選択列 | 威力の例 | 最終属性 |
| --- | --- | --- |
| 無属性ATK10 → 65 | 10 | 火 |
| 65だけ | 2 | 火 |
| 85だけ | 5 | 水 |
| ATK10 → 65 → 無属性+ATK3 | 13 | 無属性 |
| ATK10 → 無属性+ATK3 → 65 | 13 | 火 |
| ATK10 → 65 → 85 | 10 | 水 |
| ATK10 → 85 → 65 | 10 | 火 |

[AttackData.isValidAttackItem][valid]は、追加側にADD_ATTRIBUTEを許す条件を
「元の先頭がWEAPON、元の先頭hitRateが0」とする。
確率武器やMAGICへsetterを付けてはいけない。
65/85を先頭にした後は普通のboostや別setterを追加できる。
先頭の値2/5と、追加位置では非加算という違いを保持する。

## 1種類のsetterだけでも無順序maskでは不足

同じATK10・65・無属性boostでも、65を前後させると火と無属性の2結果になる。
従って「setterを火だけに絞れば既存のslot昇順で正確」とはならない。
最後に65を処理する等の固定順にすれば実装はできるが、旧版で選べる別結果を失うsubset制限となる。
一般的な実装は順序付きの追加列を持つ別Action/schemaを使う。

ただし現在の限定プールでは、6byteを維持できる意味的圧縮も可能。
通常boostは純粋なINCREASE_ATKだけで、setterは属性上書きだけ。
各使用数・手札消費・威力は並び順で変わらず、最終属性だけが順序に依存する。

### 6byteの意味的Action案

新しいkindを`attack_set_fire=7 / attack_set_water=8 / attack_set_neutral=9`とする。
base=先頭武器slot、slots=選んだ全extraのmask、target=座席を維持する。
extraにsetterを1枚以上要求し、普通boostとは別のsetter候補maskをOptionsに公開する。
先頭がsetterでextraにsetterがなければ、既存の通常ATTACKでよい。

| 指定した最終属性 | 到達可能な条件と並びの証拠 |
| --- | --- |
| 火 | extraに65がある。普通boost等を前、65を最後へ置く |
| 水 | extraに85がある。普通boost等を前、85を最後へ置く |
| 無属性 | extraにsetter Sがあり、通常boostにSでも光でもない属性の物がある。そのboostをSの後ろへ置く |

最後のsetterより前の属性は全て上書きされる。最後のsetterより後では通常合成しか起きないので、
結果はそのsetterの属性または無属性のどちらか。
65/85の複数選択や同じsetterの重複でも、この到達条件で全結果を表せる。
setter同士が異属性だからという理由だけで無属性を許さない。最後のsetterが必ず火か水へ戻す。

独立検算では、7属性の通常boostと2setterから作る長さ1〜5の全順列をまとめた。
setterを含む1,210個の多重集合、先頭属性7種、先頭ATK2/5/10を組み合わせた25,410局面・
983,262順列について、(最終属性、威力、消費数、配布増分)の到達集合が3kindと一致した。
先頭属性は無属性の到達条件には数えない。検算の普通boost威力は属性番号+1を使い、
setterは追加威力0とした。正の加算だけなので、任意の個別boost値でも同じ式が成立する。
選択maskが同じなら手札の残りと消費神器も同じであり、乱数消費はこの組合せ処理にない。
被弾副作用は先頭由来の効果と最終属性・威力が同じなら同じとなる。
これは限定合成の数学的検算で、旧サーバー実行や公式fixtureとの比較ではない。

以下は外部コードを呼ばない独立検算の再実行例。

```python
from itertools import combinations_with_replacement, permutations
NE, FIRE, WATER, LIGHT = 0, 1, 2, 5
for n in range(1, 6):
    for extras in combinations_with_replacement(range(9), n):
        setters = {x - 6 for x in extras if x >= 7}
        if not setters:
            continue
        reachable = set(setters)
        if any(x not in (s, LIGHT) for s in setters for x in extras if x < 7):
            reachable.add(NE)
        for base_color in range(7):
            for base_amount in (2, 5, 10):
                actual = set()
                for sequence in set(permutations(extras)):
                    color, amount = base_color, base_amount
                    for x in sequence:
                        if x >= 7:
                            color = x - 6  # 7/8 are Fire/Water setters.
                        else:
                            amount += x + 1
                            color = x if color == LIGHT else color if x in (color, LIGHT) else NE
                    actual.add((color, amount, n + 1, n + 1))
                expected = {(color, base_amount + sum(x + 1 for x in extras if x < 7), n + 1, n + 1)
                            for color in reachable}
                assert actual == expected
```

Pendingの`attribute_card`は正規化した最後setterの代表IDとする。
火/水指定はその色の選択setterの最小slot、無属性指定は上の条件を満たすSの最小slotを使う案。
そのsetterを最後のsetterとして、必要なら異色boostを後ろへ置けば、代表に対応する実在の順序を作れる。
これは全ての元の選択順を保存した来歴ではなく、同じ遷移の証人を1つ固定したもの。

この案は旧XMLの選択順そのものを保存せず、同じ状態遷移の列を同じ意味的Actionへまとめる。
その制限をschemaへ明記すれば現在の範囲で遷移を再現でき、reservedやtargetの未使用bit流用は不要。
将来DOUBLE_ATK・WIDE_ATK・複数奇跡・無消費化・順序依存の副作用を加えると証明は使えない。
その時は順序付き表現へ拡張する。

## 別段階の参考: 195の属性除去と防御列

以下は調査結果だけを記録する。195はv11の台帳・合法手へ追加しない。

[defenderCommand][defense]は195を処理した位置で応答用の複製と元の`currentAttack`を両方無属性にする。
後で反射するときもこの変更は残る。通常防具の防御値減算だけが反射時に消えることと区別する。
195自体はDFSでないので威力を減らさない。

通常防具と195だけなら、処理後の攻撃が無属性なので属性適合に通る。
195の前後で普通防具の威力減算・副作用・消費数は変わらず、同じprefix maskで表現できる。
既存の「prefix＋特殊1枚」に195を含めるときは、選択したprefixが特殊の前に処理される意味とする。

| 使用列 | 結果 |
| --- | --- |
| 火ATK10へ195だけ | 無属性ATK10を受ける |
| 火ATK10へ195＋DEF3 | 無属性の残余7を受ける |
| 光/闇武器へ195 → 39 | 本体が無属性になり、既存の武器反射を使える |
| 光/闇武器へ195 → 215 | 本体が無属性になり、武器遮断を使える |
| 闇魔法へ195 → 210自己bounce | 無属性化が本体に残り、通常の威力を受ける。闇の999処理は発生しない |
| 39等の発火後に195 | 特殊分岐でループを抜け、195は未使用。その遷移は195を選ばない行動と同じ |

元のMAGIC/WEAPONの種別は195で変わらない。MAGICへ39を使えるようにはならない。
命中率・Glory等の被弾効果も消さない。例えば200は属性除去後も残余があればGloryが付く。
命中済み確率攻撃へ195を使っても命中を再抽選しない。
Gloryの意図規則では195も1枚なので、195＋反射/遮断は2枚として拒否する。

## 現行説明との不一致C04

[2026-09-20の第三者抽出][bible]では、火のWand of Ignitionは+ATK2と属性設定、
水のWand of Mystic Waterは+ATK5と属性設定を表示する。
旧65/85に対応する強い候補だが、旧数値IDの一致を確定するものではない。
旧追加時にATKが増えない処理は、この表示の加算値と衝突する。これを新しい衝突C04として扱う。
旧版profileでは非加算を記録し、現行値へ暗黙に修正しない。旧エミュレーターの不具合という断定もしない。
195の対応候補はRainbow Curtainの属性除去・価格15・配布率3で、名称Wingsとは確認していない。
光防御C01等の既存衝突も未解消のまま。

## 必須のコア変更と受入条件

1. 台帳で先頭ATK・通常boost・属性setterを別の役割として持ち、65/85へboost2/5を入れない。195は除外する。
2. 攻撃はordered追加か上記の到達結果指定を選び、同じmaskで選べる火/水/無属性を失わない。
3. setterは必中WEAPON先頭の後ろだけに許す。確率武器・攻撃奇跡・防御へ同じ役割で流用しない。
4. setterも実際に使った1枚ごとに消費・配布予定+1。手札詰めの前に全slotを検証する。
5. 新kindは選択extraにsetterが必要。先頭setterだけの場合は通常ATTACKとする。
6. Pendingの代表setter来歴と最終属性を検証し、普通boostだけで火/水を上書きした状態を捏造できないようにする。
7. 不正なsetter結果、未所有、base/extra重複、Glory超過をbatch変更前に拒否する。
8. 65/85の先頭値、extra非加算、同一maskの異なる最終属性、複数・重複setterと到達不能な無属性指定を確認する。
9. 新kind/Options/観測を別schemaで公開し、既存10profileの台帳・寸法・保持index・乱数列を保つ。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[item]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/item.py
[attack]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L226-L286
[valid]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L78-L94
[defense]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L510-L643
[bible]: https://github.com/BrandenXia/godfield-bot/blob/main/data/snapshots/2026-09-20/bible.json
