# 旧版の光・闇とGlory武器

調査日: 2026-10-31。対象はIgoorxの非公式Flashサーバーエミュレーター。
以下はwebで取得したソースの読解であり、公式クライアント実行や通信による照合ではない。
参照先の `master` は可変なので、台帳化時は取得物のhashと取得範囲を記録する。
既知の[衝突C01](official-spec-gap.md)を解決済みとはしない。

## 防御と闇のダメージ

[Item.checkDefense][item]の属性対応は次の通り。
空の防御は[Turn.defenderCommand][defend]が属性照合を省略する別経路。

| 最終攻撃属性 | 許される最終防御属性 |
| --- | --- |
| 無属性 | すべて |
| 火 | 水、光 |
| 水 | 火、光 |
| 木 | 土、光 |
| 土 | 木、光 |
| 光 | 闇のみ |
| 闇 | すべて |

通常防具は先に防御力を減算し、最後の合成属性で適合を判定する。
防具1枚ずつの適合判定ではない。不適合な選択をしてから消費する旧サーバーの
assert経路を移植する必要はなく、既存C++と同様に変更前に拒否する。

[Turn.inflictDamage][damage]は残余ダメージが正のとき、最終属性が闇なら999のダメージを与える。
無守護神・HP上限99の今回の範囲ではHP0となる。残余0ならこの処理を通らない。
闇を含んだだけでは発動せず、合成結果が闇かを使う。属性混合で無属性なら通常の残余ダメージ。
闇分岐では通常の吸収・自傷分岐を通らず、その前に実行される被弾状態付与は残る。
守護神の吸収を考慮する全仕様では、999を無条件の死亡確定と読み替えない。

特殊反応にはさらに制約がある。[defenderCommand末尾][special]は通常の武器反射等について
闇とcounter由来攻撃を明示的に拒否し、合成防御属性も照合する。
万能反射・魔法反応等の例外があるため、今回の無属性武器向け反射を光闇にも無条件に広げない。

## 光を含む通常合成

[queueAttack][combine]と[defenderCommand][defend]の通常合成をまとめると、光は代用属性として働く。
未選択の初期状態と無属性は区別する。光だけなら光、光以外が1種類ならその属性、
光以外が異なる2種類以上なら無属性となる。ここでは無属性も光以外の1種類。

| 組み合わせ | 最終属性 |
| --- | --- |
| 光＋火、火＋光 | 火 |
| 光＋闇、闇＋光 | 闇 |
| 光＋無属性 | 無属性 |
| 光＋光 | 光 |
| 闇＋闇 | 闇 |
| 闇＋火 | 無属性 |
| 火＋水＋光、火＋水＋闇 | 無属性 |
| 闇＋無属性＋光 | 無属性 |

光＋闇も闇になる点は旧実装固有の根拠として記録する。現行抽出の説明は
光の代用先を基本4属性としており、光＋闇について明示していない。

通常INCREASE_ATKだけなら光闇を加えても順序不変で、既存の追加スロットmaskを維持できる。
ADD_ATTRIBUTEは順序付き上書きなので引き続き対象外。
通常合成については、独立した集合表現と逐次合成の比較で7属性・長さ1〜4の全2,800列が一致した。
これは数学的な順序不変の検算であり、旧サーバー実行との差分検証ではない。
例えば光武器70に無属性boostを足すと無属性、闇boost89を足すと闇となる。
Gloryの付与元であるという理由で光属性へ固定してはならない。

## ID70の正確なデータと発動

[CSVの0始まり行70][csv70]を[Item.fromData/loadItems][item]の列定義で解釈する。
名称はCSVにないため、ローカル識別子 `legacy-070` を使い、未確認の名称を旧公式名として付けない。

| フィールド | 値 |
| --- | --- |
| type / attackKind / attackExtra | WEAPON / ATK / GLORY |
| defenseKind / defenseExtra | 空 / 空 |
| attribute / value | LIGHT / 2 |
| subValue / hitRate | 空から0 / 空から0 |
| price / weight | 15 / 1 |

命中率0はこの通常武器の確定命中sentinel。防御力は0で、追加ATK能力はないため、
先頭武器として使い、他武器の後ろの追加部品にはできない。
他の許可済みboostを後ろにつけることはできる。[AttackData.isValidAttackItem][attack-valid]を参照。

被弾時の状態付与は[inflictDamage][damage]で残余ダメージが正のときに発生し、通常のHP減算より先。
したがって完全防御でGloryは付かず、属性混合しても残余があればGloryが付く。
Heavenへの付与・重複・薬の解除は既存の[harms規則](legacy-harms-rules.md)へ従う。
自己対象は通常の応答を省略して自分へ解決する。

Gloryの1枚防御制限を継続する場合、それは既存v7と同じくbotで確認した意図を採用する限定profile。
旧サーバーの手動入力経路が複数枚を拒否しない点は修正済みの差として残す。

## C01と現行抽出記録の位置づけ

[2026-09-20の第三者抽出][bible]は、光攻撃を属性では防御できず、闇は残余被弾でHP0とする。
現在の画面を今回直接検証した資料ではない。旧表の光→闇との違いを公式変更の事実と断定しない。

[旧CSV][csv]には闇の通常DFS防具がなく、闇の防御神器はCOUNTER192がある。
今回counterを除外するなら、実際に構成できる通常防具は光攻撃に適合せず、
表の差を通常プールの局面へ持ち込まずに光武器を実装できる。
この到達範囲の一致はC01そのものの解消ではない。

全旧版サーバーの入力受理では、192と光DFS158を併用すると合成属性は闇となり、
防御値6を差し引いてから光への適合に通る経路が読める。
残余0ならcounterも起動しない。これはソースから導いた局面で、Flash UIがその選択を許すかは未検証。
将来counterを入れる際は、この具体例を公式で照合する必要がある。

抽出のFlash Daggerは光・ATK2・価格15・被弾時Flashという特徴を持ち、旧70との対応の強い傍証となる。
Flashの参照説明も防御1枚であり旧botのGLORY制限と一致する。
ただし抽出に旧数値IDとの対応表はなく、ID同一性や全効果の同一性の確定にはしない。
抽出の配布率1/500と旧weight1も同じ尺度とはみなさない。

## v8のscope提案

新しい旧版限定profileを追加し、旧profileの属性enum値0〜4、台帳、乱数列、観測寸法を保つ。
enumの追加値はlight=5、dark=6を提案する。新観測では7属性を識別できる場所を用意し、
旧観測の5要素onehotへ光闇を無理に押し込まない。

候補は次の18種。確率武器を今回省く場合は最初の15種まででもよい。
いずれも元の属性・効果・価格・重みを保持し、限定プール内で重みを正規化する。

| 追加対象 | 旧ID |
| --- | --- |
| 単純な光闇武器、INCREASE_ATK武器 | 62, 63, 75, 76, 82, 83, 87, 88, 89, 93, 96, 97 |
| 光のGlory武器 | 70 |
| 通常光防具 | 158, 176 |
| 単純確率武器: 闇ATK2/50%、光ATK3/25%、光ATK6/75% | 101, 104, 111 |

属性追加、倍率、全体化、PESTLE99、死亡時効果117、counter191/192、属性付き奇跡は対象外を維持。
闇counterを仮想防具へ変換したり、光への防御を作るために新しい闇DFSを捏造しない。

## 受入条件

1. 固定した取得物から候補ID・列・hashを再生成し、除外をmanifestへ記載する。
2. 通常合成の7属性全組み合わせと順序不変を検査する。光＋闇、光＋無属性も含める。
3. 火に光防具は可、火に光＋無属性防具は不可、火に光＋水防具は可を確認する。
4. 光攻撃には今回の通常防具を選べず、0枚受けは可。闇攻撃には全属性防具が使える。
5. 闇の完全防御で生存、1以上の残余でHP0、属性混合後は通常ダメージとなる。
6. 通常resetから70を入手し、相手にGloryを付与して次回の複数防御を拒否する。235/236で解除する。
7. 70＋無属性boostの反射後は新対象へGloryを付与し、70＋89は闇とGloryの順序を保つ。
8. 光闇chanceの境界値とDarkCloud必中・乱数非消費を維持する。闇は命中かつ残余正のときだけHP0。
9. 不正行動はbatch全体を変更せず拒否し、旧7 APIの出力境界と再現性を維持する。

[item]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/item.py#L27-L41
[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[csv70]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV#L71
[combine]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L283-L286
[defend]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L510-L603
[special]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L630-L643
[damage]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L382-L494
[attack-valid]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L78-L94
[bible]: https://github.com/BrandenXia/godfield-bot/blob/main/data/snapshots/2026-09-20/bible.json
