# 旧版の属性除去195と防御prefix

調査日: 2026-10-31。非公式Flashサーバーエミュレーターのwebソース読解。
現行公式の実測ではない。可変の`master`から取得した根拠なので、台帳は取得物のhashと取得範囲を記録する。
[属性setterメモ](legacy-attribute-change-rules.md)で別段階とした195を、この段階の唯一の追加対象とする案。
反射・防御奇跡・病気は既存profileの範囲を維持し、196の万能反射やcounterを同時に追加しない。

## 正確なデータと役割

[旧CSVの0始まり行195][csv]を[Item.fromData][item]の列で読む。

| 項目 | 値 |
| --- | --- |
| type | PROTECTOR |
| attackKind / attackExtra / defenseKind | 全て空 |
| defenseExtra | REMOVE_ATTRIBUTE |
| attribute | 空、すなわち無属性 |
| value / subValue / hitRate | 空から0 |
| price / weight | 15 / 3 |

195は通常DFSを持たず、通常防御力は0。ATKや通常boostもなく、mainの攻撃・utilityとしては使わない。
PROTECTORなので手札からDISCARDできる。奇跡ではないため保持化・MP消費・保持からの再使用はない。

## 適用タイミングと元本体への変更

[defenderCommand][turn]は最初に`currentAttack`を複製し、応答の選択列を順番に処理する。
195へ到達すると次の順になる。

1. 所有する手札195を使用して取り除き、使用者の配布予定を1増やす。
2. 応答用の攻撃属性が未設定のNoneではないことを確認する。
3. 応答用cloneの属性と、元の`currentAttack`の属性を両方無属性へ上書きする。
4. 防御列の通常属性合成へ195の無属性を含め、その後の通常DFSや特殊応答へ進む。

[AttackData.clone][clone]は属性と威力を複製するが、195は片方だけでなく両方を明示的に変更する。
従って、後続反射・跳ね返しが元の未減算攻撃へ戻っても属性除去は残る。
通常防具の威力減算が反射時に消えることとは異なる。

既に無属性の攻撃にも195を使える。Noneと無属性の空文字は別であり、無属性だから未設定扱いで拒否しない。
複数の195を使えば各1枚を消費し配布予定を増やすが、属性は何回除去しても無属性のまま。
195自身はDFS分岐を通らないので、威力は減らさない。

## 対象範囲

195の分岐は元神器のWEAPON/MAGICや正の威力を要求しない。条件は属性がNoneでないこと。
今回の通常攻撃・奇跡・HP/MP/所持金増加・解除は[queueAttack][queue]で属性が設定されるため、全て対象になる。
未対応の売買・特殊な強制行動など、属性未設定の経路へ一般化しない。

| 入力 | 属性除去後の結果 |
| --- | --- |
| 火・水・木・土・光の攻撃 | 無属性。普通防具を組み合わせて防げる |
| 闇の攻撃 | 無属性。残余被弾は通常威力で、闇の999処理をしない |
| 既に無属性の攻撃 | 威力・属性は同じだが、195の消費・配布は発生 |
| 命中済みの確率攻撃 | 属性だけ無属性。命中率・元イベント・命中済みという状態は維持 |
| HP/MP/所持金増加・解除 | 属性は無属性のまま。元の効果量・病気付与・解除効果は変えない |

[inflictDamage][damage]の闇判定は現在の最終属性を使うため、除去後に闇の致死を起こさない。
一方、Glory・DarkCloud・病気等の効果リストは消去しない。
200は195で無属性になっても、残余被弾が正ならGloryを付ける。
234のMP回復後Heaven付与も、属性除去で消えない。

命中判定は応答より前に終わっている。[doAttack][hit]へ戻らず、195を理由に再抽選しない。
最初のmissなら反応機会自体がない。

## 普通防具と既存特殊への接続

通常防御の最終属性照合は、除去後の攻撃が無属性なので通る。
195を使う組合せでは、元属性に単独では不適合な普通防具もprefixとして選べる。
195が選ばれない通常防具だけの応答は、従来通りの最終合成属性が必要。

| 使用列 | 遷移 |
| --- | --- |
| 火ATK10へ195のみ | 無属性ATK10を受ける |
| 火ATK10へDEF3と195 | 無属性の残余7を受ける |
| 光/闇WEAPONへ195→39 | 本体を無属性化して、武器反射へ進む |
| 光/闇WEAPONへ195→20 | 本体を無属性化して、武器跳ね返しへ進む |
| 光/闇WEAPONへ195→215 | 無属性化後の判定を通り遮断。闇禁止も現在属性が無属性なので抵触しない |
| 闇MAGICへ195→210 | 無属性化した本体を跳ね返す。本人へ当たっても通常威力になる |
| MAGICへ195→20/39/215 | 元のtypeがMAGICのままなので、武器向け特殊の対象にはならない |

旧特殊分岐と末尾検査は[defenderCommand][turn]による。
命中済み確率WEAPONも195後は無属性となり20/39/215の対象になる。
旧ソースのこれらの分岐はhitRate=0を要求しない。
従来profileの「無属性確率武器がない」ための`!pending.hit_rate`制約を、そのまま新profileへ適用してはいけない。

元の武器・奇跡のID、先頭のtype、命中率、状態付与、属性setter由来IDは195自体では変えない。
現在source/targetも除去だけでは変えず、後続の反射・跳ね返しが変更する。
元の奇跡へのMP支払・初回保持化もやり直さない。

## 順序をprefix選択へ圧縮できる範囲

現在の意味的Actionの`slots`へ195を含め、**選んだ全prefixは特殊応答より前に処理する**と定義できる。
baseは既存の20/39/210/215の手札slot、またはDEFEND_CASTの保持indexのままでよい。
新しい順序付き行動やreserved byteの転用は不要。

この圧縮が状態遷移を保持する理由は次の通り。

- 195と普通DFSだけなら、195の位置にかかわらず攻撃属性は最後に無属性となる。
  通常DFS減算の合計・手札消費・配布予定・病気等の副作用は同じ。
- 特殊の前に195があれば、元本体に属性除去が残り、普通DFSの威力減算だけが特殊分岐で失われる。
  prefix内の195と普通DFSの順序を固定しても、現scopeの結果は変わらない。
- 特殊が先に発火するとループを抜け、後ろの195は未使用。
  その資源・手札・効果の遷移は、未使用の195を選ばない行動と同じになる。

ただし旧COMMANDの完全な列トレースを再現したという意味ではない。
旧ソースは特殊でbreakした後のXML出力に元のpieceListを使う箇所があるため、
送ったが使われなかった195と、送らなかった195で表示列が違う可能性がある。
意味的な状態遷移へまとめる制限をschemaへ記録する。
将来、複数特殊・新しい順序依存の副作用・完全XML再生を追加するときは改めて検証する。

## Glory、病気、配布

195は防御使用1枚として数える。Gloryの意図規則では195単独は可能だが、
195＋普通防具や195＋反射/防御奇跡は2枚なので拒否する。
旧手動入力サーバーにGloryの枚数検証がない点は、既存[harmsメモ](legacy-harms-rules.md)と同じ差として残す。

prefix175のFever付与でHeavenからHP0へ転落しても、195や後続特殊は死亡確定前に処理を続ける。
195は病気・HPを回復しない。回復奇跡の自己bounceが後ろにあれば、既存の暫定HP0救済と同じく解決する。

使用した195ごとに手札消費1・配布予定1。連鎖中は補充しない。
元の通常turn_ownerの病気tickと手番末の配布・死亡・終局順序は既存処理のまま。
属性除去のために追加の通常手番や病気tickを生成しない。

## 必須のコア変更

1. 台帳へ195だけを通常DEF0の属性除去役として追加し、mainではDISCARDだけを提供する。
2. 195を持つ応答では、普通prefixと特殊候補を条件付きで広く公開する。
   legalは選択maskに195がある場合だけ無属性化した仮のPendingで判定し、195なしの同じ不適合選択を拒否する。
3. 使用処理では195の消費・配布予定を他のprefix同様に積み、元Pendingの属性を無属性へ変更してから特殊へ進む。
4. 明示的な属性除去来歴をPendingへ持たせ、restoreで元のDARK/LIGHT/属性付きmiracle/chanceが無属性へ変わる条件を検証する。
   原因を持たない任意の属性書換えを許さない。既存のattribute_card等の来歴は保持する。
5. 元が確率WEAPONでも除去後に物理反応できるようにし、元命中イベントのsource/targetと抽選prefixを変えない。
6. 複数195、Glory単枚、暫定HP0、DEFEND_CASTのprefix、retained_dealsを既存の制約と両立させる。
7. 観測suffixには公開された除去の来歴を追加できるが、隠れた手札や抽選値を露出しない。
   旧11profileの台帳・乱数・出力寸法とstrideは保つ。

## 受入条件

1. 通常resetから195を入手し、火攻撃へ195単独で威力不変の無属性を受ける。
2. 光/闇攻撃に195＋普通防具を使い、残余通常ダメージ・完全防御・消費枚数・配布を確認する。
3. 火/光/闇WEAPONを195→20/39/215へ接続し、195なしなら拒否する。
4. 命中済み属性付き確率WEAPONを195→39で返し、再応答でも無属性が残り命中再抽選しない。
5. 闇MAGIC207を195→210で自己bounceし、999でなくATK5を受ける。
6. 200や病気武器の効果は残し、回復216・所持金220等の量を変えない。
7. 既に無属性・非damageへの195使用、複数195、Glory時195単独/複数拒否を確認する。
8. 新たな合法手であっても、未所有・base重複・不適合な195なし選択をbatch変更前に拒否する。
9. 除去Pendingのsnapshot往復、旧11APIの境界、再現性、done freezeを確認する。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV#L196
[item]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/item.py
[turn]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L510-L643
[clone]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L58-L77
[queue]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L85-L316
[damage]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L382-L494
[hit]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L317-L380
