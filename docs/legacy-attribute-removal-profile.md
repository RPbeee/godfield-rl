# 防御時の属性消去 v12

`legacy-attribute-removal-subset-v12` は属性消去の防具195を追加する限定profile。
CLIの `attribute-removal` と `gn_*` はv12までの拡張を有効にし、175種・旧重み合計365で動く。
旧非公式エミュレーターを参考にした実装で、現行公式の完全再現ではない。

## 対象と出典

[旧版の属性消去処理](legacy-attribute-removal-rules.md)、
[台帳・出典・指紋](../data/legacy-attribute-removal-v1/README.md)を参照。
既存11台帳を変更せず、旧重みを限定プール内で正規化する。公式の配布率とは扱わない。

| 旧ID | 種類 | 効果 | ATK / 追加ATK / DEF | MP | 旧価格 / 重み |
| --- | --- | --- | --- | --- | --- |
| 195 | 防具 | 現在の効果を無属性へ変更 | 0 / 0 / 0 | 0 | 15 / 3 |

元データには通常DFSも属性値もなく、REMOVE_ATTRIBUTEだけがある。
モデルでは専用台帳への所属で効果を保持し、単なるDEF0の普通防具へ置き換えない。
通常手番では捨てられるが、攻撃・回復・奇跡としては使えない。

## 消去と応答

195はdefend/defend_castのslotsで選び、特殊応答より前に処理する。
defendのbaseへ195を指定する操作は不可。
攻撃力・命中済みの結果・奇跡ID・元の付随効果を保ち、現在の属性だけを無属性へ変える。
既に無属性でも使え、同じIDの別個体を複数選んだ場合も各1枚を消費して配布予定+1。
無属性化自体は冪等で、MPを使わない。

| 組み合わせ | 挙動 |
| --- | --- |
| 有色/光/闇の攻撃へ195だけ | DEF0で受ける。闇の即死は消え、通常の残余ダメージになる |
| 195＋普通防具 | 無属性攻撃として全属性の普通防具で防御できる |
| 195＋20/39 | 元が有色・闇・命中済み確率武器でも武器用の跳ね返し/反射が可能 |
| 195＋手札または保持215 | 無属性化された武器を遮断できる |
| 奇跡へ195＋210 | 奇跡のまま跳ね返し、以後も無属性を維持する |
| 奇跡へ195＋20/39/215 | 不可。属性を消してもWEAPONにはならない |

195を所持しているだけでは反射や遮断を合法にしない。実際のslotsに195を含める必要がある。
一度消去して未解決の連鎖へ入った場合は、その後の応答でも無属性を維持する。
命中済み確率攻撃を反射しても、命中乱数・指定tapeは消費し直さない。
回復・MP増加・所持金増加・解除にも195を使えるが、元の効果量や解除対象は変えない。
Gloryでは195も1枚に数えるため、195単独は可、195＋普通防具や特殊応答は不可。

## 順序の意味的な表現

今回の範囲では、195と普通防具の前後は最終遷移に影響しない。
選択slotsの195を全て特殊より前に使い、通常防具の軽減・副作用と特殊応答を処理する表現で扱う。
特殊より後ろに送った195は旧実装では未使用なので、195を選ばないActionと状態遷移が同じになる。
旧XMLの列に未使用の神器も残る場合があるため、厳密なCOMMAND列トレースまで同一とは主張しない。
消費・配布・効果の遷移を保つ意味的Actionとして6byte形式を維持する。

反射では普通防具を先に消費しても元の威力を返す既存の挙動を維持し、属性消去だけは本体へ残す。
攻撃時に65/85で付けた `attribute_card` と、病気・Glory・DarkCloudも消さない。
元の奇跡の保持・MP支払いはturn_ownerに属し、現在のsourceへ付け替えない。

## C++・検証・観測

```cpp
godfield::legacy::Env env({512, 10, true, true, true, true, true, true, true, true, true, true, true});
env.reset(42);
```

末尾の `Config::attribute_removal` はデフォルトfalse。有効化にはelementsが必要で、他の拡張は個別に選べる。
CLIとC APIは全拡張有効。再現にはprofile名だけでなく全Configを保存する。
`Pending::attribute_removed` は現在の効果を195で無属性化した来歴。
trueなら現在属性は必ずNE。元のmiracle・hit_rate・attribute_card・status/harmは保持する。
未解決snapshotでは原子的なprefix＋特殊を経ているので、連鎖回数1以上・総配布予定が連鎖回数+2以上を要求する。
`StepResult::attribute_removed/attribute_removals/removed_from` は当該応答での使用有無・枚数・最初の消去前属性を返す。
既NEへの使用も使用数へ含める。
Snapshotの共通versionは35。同じビルド・設定のfixture用で、旧形式を自動移行する永続checkpointではない。

`Options::removal_slots` は使用可能な195の手札位置mask。
195所持時は、組み合わせ次第で使える不適合防具・反射・遮断も候補へ含める。
maskだけで合法性を判断せず、構成したActionを `legal()` で検証する。
ランダム方策も必要な195を実際に選び、Gloryの枚数制限を守る。

共通C++観測は1317次元、`gn_*` ABI 12は先頭393次元を返す。
v12では末尾924個が0になる。追加部分は[反撃profile](legacy-counter-profile.md)と[反撃拡張profile](legacy-counter-extensions-profile.md)と[所持金反撃profile](legacy-money-counter-profile.md)と[魔法反射profile](legacy-magic-reflection-profile.md)と[万能反射profile](legacy-any-reflection-profile.md)と[MP武器profile](legacy-mana-weapon-profile.md)と[魔法遮断profile](legacy-magic-block-profile.md)を参照。先頭375個は[属性付け替えprofile](legacy-attribute-change-profile.md)を継承する。

| 位置 | 内容 |
| --- | --- |
| 375 | attribute_removal有効なら1 |
| 376〜391 | 自分の各手札位置が195なら1 |
| 392 | 未解決効果のattribute_removed |

195の通常手札ATK/追加ATK/DEFは0。相手の手札・完全な保持数、未来の抽選値・乱数状態は出さない。
旧profileの追加18特徴は0。

## C ABIとPython

[attribute_removal_c_api.h](../include/godfield/attribute_removal_c_api.h) の `gn_*` と
[AttributeRemovalBatchEnv](../python/attribute_removal_env.py) を使う。
6byteの `gl_action` とkind0〜9を維持し、mask幅10の最後にremoval_slotsを追加する。
旧 `gl/gm/ge/gc/gr/gd/gh/gs/go/gb/ga` の128/164/186/210/236/258/283/301/334/356/375次元とmask幅・kind範囲は維持する。
バッチの原子的拒否、固定座席報酬、終了行固定・自動resetなしを継承する。
Pythonバッファーは再使用するので、履歴へ保存する際はコピーする。同じハンドルを複数スレッドから同時操作しない。

```sh
python3 python/attribute_removal_env.py
./build/godfield_legacy_bench 100000 1 42 attribute-removal
./build/godfield_legacy_bench 100000 4 42 attribute-removal
```

新しい武器/MAGICへの変換、万能反射の全種類、反撃、追加魔法、吸収、霧・幻覚、守護神、蘇生、終末、複数人戦、
現行公式との動的照合と学習器は未対応。資料間衝突C01〜C04は残す。

## 検証結果（2026-10-31）

通常CTest 37件、ASan/UBSan 25件が通過。新旧12種類のPython suiteは計100テスト。
全属性・重複195、実選択に依存する反射/遮断、確率武器の再反射、奇跡・setter・付随効果の維持、
Glory、組み合わせ候補のsampler、snapshotの来歴・配布予定拒否、秘匿、1000試合の再現を確認した。
防御奇跡を無効にした独立構成でも、確率武器を二度反射したsnapshotから復元できる。

全機能有効・10万試合・seed42・意思決定上限512で、1/4スレッドの統計とchecksumが一致した。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 3,400,358 |
| 座席0 / 座席1勝利 | 49,826 / 49,664 |
| 勝者なし / 打ち切り | 510 / 0 |
| 消去を使った応答 / 消費枚数 | 18,091 / 18,271 |
| 既に無属性への使用 | 10,417 |
| 武器 / 奇跡への使用 | 17,040 / 1,051 |
| 消去と同時の武器反応 / 魔法跳ね返し / 武器遮断 | 1,333 / 14 / 189 |
| 消去済みの武器反応 / うち確率武器 | 1,387 / 140 |
| checksum v12 | 8858179301953077720 |

消去と同時の反応と、既に消去済みの未解決効果への後続反応は別に集計する。
ベンチの速度出力は行動生成・合法性検証・遷移・統計・resetを含み、観測転送・モデル推論は含まない。
旧11profileのchecksum式を維持し、同条件の各10万試合で意思決定数・勝敗・checksumが以前と一致した。

共通観測の新しい追加部分は[HP吸収v28](legacy-hp-absorption-profile.md)を参照。旧ABIの幅・行動形式は維持する。
