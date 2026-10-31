# C++学習ベースライン

`godfield_train` は依存ライブラリを追加せず、C++20で方策を学習・評価する実行ファイル。
対象は231種の `legacy-dying-weapon-subset-v37` で、[部分仕様](legacy-dying-weapon-profile.md)に従う。
現行公式やFog/Illusion・守護神・複数人戦の完全再現ではなく、本家での強さを示すものでもない。
ゲームのschema/既存37profileやC ABIを変更する学習機能ではない。

[学習API](../include/godfield/learning.hpp)を既存ゲームライブラリへ接続する
STATIC `godfield_learning` と、それを使う `godfield_train` をビルドする。
GPU・LibTorch・Pythonの機械学習パッケージは不要。PythonはCLI回帰テストにだけ使用する。

## ビルドと実行

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
./build/godfield_train --episodes 10000 --seed 42 --candidates 12 --batch 64 --opponent snapshot --out runs/baseline.gfl
./build/godfield_train --resume runs/baseline.gfl --episodes 10000 --out runs/resumed.gfl
./build/godfield_train --evaluate runs/resumed.gfl --eval-games 1000 --eval-seed 1000000000
./build/godfield_train --evaluate runs/resumed.gfl --eval-games 1000 --eval-seed 1000000000 --greedy
```

`--episodes` は追加する学習episode数。省略時は10,000、出力先の初期値は `runs/baseline.gfl`。
`--resume` は保存設定を使って追加学習し、`--evaluate` は保存モデルを評価するだけで更新しない。
resume/evaluate時は、`--seed`・候補数・学習率等の学習設定を明示上書きできない。同じ値でも拒否する。
評価seed/試合数/greedyは変更できる。evaluateとresume/episodes/outは併用しない。
`--greedy` は候補内最大スコアを選ぶ評価で、softmax抽選の評価とは分けて比較する。greedyでも候補を作る乱数は残る。
結果は `config.*`、`initial.*`、`final.*`、`stats.*` 等のkey=valueで出力する。
初期評価と学習後評価は同じ評価条件を使い、学習率/候補数/相手設定もログへ保存する。

## 方策と学習方法

方策は256特徴の線形スコアとsoftmax、価値関数は32特徴の線形結合にtanhを適用する。
終局報酬によるREINFORCEに価値baseline・entropy項を加え、Adamで更新する（β1=0.9、β2=0.999、ε=1e-8）。
隠れ層のあるネットワークやPPO、探索木は同梱しない。
再現用に10,000episode学習した参照checkpointを同梱するが、一般的に強い方策という保証はしない。

各意思決定で既存の合法手samplerから候補を作り、重複を取り除く。
デフォルト候補数は12、上限32。可変長の順序付き追加・防御・購入確認も既存Actionで表現する。
**全合法手を列挙したsoftmaxではない。** 候補に出なかった行動は選択できず、
既存のhierarchical random samplerの分布と候補上限が到達する方策を制約する。
候補数の8倍まで提案して打ち切るため、重複が多い局面では指定数より候補が少なくなる。
初期重み0では重複除去後の候補に一様であり、native randomと同じ行動分布ではない。
候補数を増やすと計算量と探索範囲が変わる。勝率を比較するときは候補数も記録する。

特徴量は現在actorの公開観測と、自分の合法な候補Actionから作る。
相手の非公開手札/保持、game RNG、将来の命中/配布ticket、内部の未公開抽選結果を方策へ入力しない。
候補ごとの将来遷移を隠匿状態で試して採点する方式ではない。
1368個の観測を全て使うモデルではなく、手作りの圧縮特徴に限定する。全特徴は[-4,4]へclipする。

| モデル | 主な特徴 |
| --- | --- |
| 方策256 | kindのone-hotとHP/MP/YEN・保留量・HP差との交差、自分/相手target、baseと物理追加品7欄の和/target交差、追加/保持/反撃件数、baseの属性/命中率/MAGIC/MP/攻撃奇跡量、攻防/回復の交差、保持参照、交換配分差、phase/owner/総保持数 |
| 価値32 | bias、観測0〜11、自分の手札7欄の平均/枚数、自分と相手の病気one-hot、総保持数 |

保持奇跡の行動はkind/base番号のscalarで識別し、物理baseの効果descriptorは0のまま。
順序付き追加も物理特徴の和と件数が中心で、全並びや固有效果の意味を完全には表せない。
この表現能力の制約と候補samplerの制約は、学習手法と分けて評価する。


対戦相手はnative random、または定期更新する固定snapshot方策を選ぶ。
snapshotは対戦中に同時更新される相手ではなく、保存された方策重みを使う。
学習側の座席はepisodeごとに0/1を交代する。固定座席と各意思決定のactorを記録し、終局の座席報酬を学習側へ対応させる。
防御後に同じactorが続くため、手番ごとの報酬符号反転は用いない。
終局勝敗と `truncated` を区別し、意思決定上限に達したepisodeは方策/価値/entropyの勾配から除外する。
終局報酬を環境の残り意思決定数だけgammaで割り引き、価値との差をadvantageとする。
batch内の学習側意思決定数で平均し、方策/価値を合わせたglobal normでclipする。
全episodeが打ち切りのbatchもbatchのupdate counterは進むが、重みとAdam momentsは更新しない。
Adamのbias補正は非空batchだけを数えるoptimizer_stepsを使い、全打ち切りbatchではこれも進めない。

`TrainConfig` の初期値はseed42、候補12、batch64、snapshot更新間隔1024、最大意思決定512、
方策/価値の学習率0.01、entropy係数0.01、gamma1、gradient norm上限5。
再現比較では設定・ルールセット・実行ファイルの版を固定する。

## 再開と評価

学習checkpointはモデル重みだけでなく、相手snapshot、Adamの一次/二次モーメント、
未更新batchの累積勾配/件数、episode/update数・非空batchのoptimizer_steps、学習用乱数状態と集計を保存する。
同じ設定・ビルドで連続学習と保存→再開学習を一致させるための形式である。
ゲームの `Env::snapshot()` とは別で、対局途中の環境状態を保存する機能ではない。
checkpoint version1、ルール/特徴fingerprint、FNV checksumを検査し、破損や不正な値を拒否する。
整数はuint64 little endian、doubleはIEEEのbit列を保存する。作業ファイルへ書いてからrenameする。
形式は明示的でも、異なるコンパイラー/数学ライブラリ/将来の実装とのbit単位一致は保証しない。

評価は学習を更新せず、別seed範囲でnative randomと対戦する。
学習相手がsnapshotの場合も、評価相手を同じsnapshotと取り違えない。
勝敗・引き分け・打ち切り・意思決定数を併記し、勝率だけで打ち切りを隠さない。
学習seedに使った局面だけへの評価を避け、評価seedと試合数を記録する。

学習方式の参照: [Williams, REINFORCE (1992)](https://doi.org/10.1007/BF00992696)、
[Kingma & Ba, Adam](https://arxiv.org/abs/1412.6980)。実装固有の特徴・候補制限・打ち切り処理は上記のとおり。

## 検証と測定

通常CTest116件、ASan/UBSan79件が通過。
最終の評価digest/counter上界補強後も、学習native＋CLIの2件をRelease/SANそれぞれ再実行して通過した。
方策＋entropy勾配の中央差分、softmax安定性、固定座席報酬/座席交換、
相手の隠匿手札・保持・未来ticket変更で候補/特徴/方策乱数が変わらないこと、
全打ち切りbatchのAdam非更新を確認した。
37episode時点の未更新batchを保存して59episode再開した結果は、連続96episodeと全checkpointが一致した。
CLIでも連続/再開のファイル一致、評価非更新、不正設定・破損checkpointの拒否を検証した。
同梱モデルを最終CLIで再評価し、下表の学習後5,000試合・全勝敗・checksumと一致した。

Release、学習seed42、候補12、batch64、snapshot間隔1024、上限512、その他defaultで実行した。
学習相手はsnapshot、評価相手はnative random。評価seedは学習と別の2,000,000,000とし、
学習前後とも5,000試合・softmax抽選・座席交代で比較した。次のコマンドの初期評価＋10,000学習＋最終評価は21.10秒だった。
これは評価を含む全体時間であり、学習だけのスループットとは扱わない。

```sh
./build/godfield_train --episodes 10000 --out runs/baseline-10k.gfl --eval-games 5000 --eval-seed 2000000000
./build/godfield_train --evaluate models/baseline-v37-10k.gfl --eval-games 5000 --eval-seed 2000000000
```

| native randomとの評価 | 学習前 | 10,000episode学習後 |
| --- | --- | --- |
| 勝利 / 敗北 / 引き分け | 2,213 / 2,753 / 34 | 4,213 / 763 / 24 |
| 打ち切り | 0 | 0 |
| 勝率（勝利数 / 全5,000試合） | 44.26% | 84.26% |
| 意思決定数 | 154,790 | 149,760 |
| 評価checksum | 6622785395644903669 | 4179362864389804844 |

学習の10,000episode自体は5,159勝・4,755敗・86引き分け・打ち切り0、
345,288意思決定、そのうち学習側171,976件。156batch/optimizer更新を行い、
未更新の16episode・351学習意思決定分をcheckpointへ残した。
学習checksumは `7457829257483848698`。
別の評価seed1,000,000,000でも1,000試合で422勝/570敗/8分から818勝/175敗/7分となった。

同梱[参照checkpoint](../models/baseline-v37-10k.gfl)は11,488bytes、SHA256は
`aee4c43f2994ecd1efa205faa41012e81382d293eb9aa594fc8c4941be8f6022`。
上の評価だけでなく `--resume models/baseline-v37-10k.gfl --episodes 10000 --out runs/continued.gfl` で追加学習できる。
元モデルを上書きせず、保存済み設定・未更新batchとともに継続する。

これは単一の学習seedとnative randomに対するベースライン改善であり、
複数学習seedでの頑健性・他の強い方策との対戦・自己対戦集団全体の強さ・現行公式での強さを示すものではない。

