# 奇跡を有効にした旧版ルールセット v2

`legacy-neutral-miracles-subset-v2` は、中立60種に単純な奇跡2種を追加した限定profile。
旧版の奇跡保持・再使用・MP消費・配布を再現するための段階であり、現行公式の全仕様対応ではない。
奇跡を含めない既存profileと `gf_*` / `gl_*` のABI・観測寸法は維持する。

## 対象と出典

[調査記録](legacy-miracles-rules.md)と[奇跡台帳](../data/legacy-miracles-v1/README.md)を参照。

| 旧ID | 効果 | MP | 重み | 保持種類index |
| --- | --- | --- | --- | --- |
| 216 | HP+10 | 6 | 1 | 0 |
| 220 | 所持金+15 | 8 | 1 | 1 |

旧IDと技術ラベルで管理し、現在の公式ID・名前の対応表とは扱わない。
現行画面の抽出記録にはSpringのMP7、Treasureの所持金+10/MP5が記録され、旧版の値と異なる。
抽選は従来60種の重み225と奇跡2種の重み2を合わせた227で正規化する。
この制限プールは公式の神器出現分布とは異なる。

## 追加した処理

- 手札の奇跡は `use` で初使用し、MPを支払い、手札から取り除いて保持数を増やす。
- 保持した奇跡は `cast` で何度でも使える。MPは毎回払い、保持数は減らさない。
- 同じ奇跡を複数枚使えば、その種類の保持数が増える。再使用の効果・コスト倍率にはしない。
- 初使用・再使用ともに配布予定数+1。再使用は手札を消費しないので、空きがあれば手札が1枚増える。
- 満杯の手札からの再使用も可能。配布時は抽選結果を消費して、容量を超えた神器を破棄する。
- MP不足なら合法手マスクから除外し、明示的に渡された行動も状態・乱数を変えず拒否する。
- 自分なら即解決、相手を指定した場合は応答を待ち、空の防御応答で効果を適用する。
- 手札の奇跡を捨てても保持化・MP消費・配布は発生しない。保持奇跡を手札の捨てる操作では消せない。
- 保持奇跡の有無は祈りの可否に影響しない。従来どおり手札の武器で判定する。

保持は種類ごとの `uint32_t` カウントで表す。保持奇跡の取得順や旧XMLのabilityIndexは保存しない。
この2種類・忘却なし・単独再使用の範囲では、同種のどの個体を再使用しても結果が同じ。
忘却や取得順に依存する効果を追加する前に、表現と抽選を見直す必要がある。

内部の保持合計は意思決定上限以下に検証する。初期保持0で初使用1回に1種類しか増えないため、
自然な実行ではこの上限を超えない。外部fixtureが上限に達している場合、さらに初使用する行動を拒否する。
人工的な保持容量16などの上限をゲームルールとして導入してはいない。

## C++ API

```cpp
godfield::legacy::Env env({512, 10, true});
env.reset(42);
```

`Config::miracles` のデフォルトはfalse。`Env::profile()` で実際のrulesetを取得する。
`use` のbaseは手札位置、追加した `cast=5` のbaseは保持種類index（0/1）。
castのslotsは0、targetは固定座席0/1。旧版の取得順に基づくabilityIndexとは異なる。
`Options::ability_slots` は保持しておりMPを払える種類のbitmask。
初使用可能な手札の奇跡は `utility_slots` に含める。

`Pending` に奇跡IDと、保持からの使用かどうかを持たせた。
保持使用では「手札16枚＋配布予定1」が正当なので、手札から使う場合と同じ容量検証にしない。
Snapshotは反撃拡張と共通の形式version35。profile、設定、保持数、保留効果を検証し、異なるprofileや旧versionは拒否する。
これは同じビルド内のfixture形式であり、永続checkpointの互換形式ではない。

## 観測・ABI

`gm_*` ABI v2は164次元の観測と6種類の構成要素マスクを返す。
harms拡張後のC++コアの観測は1317次元で、`gm_*` は先頭164個に出力を制限する。
この奇跡のみのprofileでは末尾1153個は0。定義は[属性profile](legacy-elements-profile.md)、
[確率profile](legacy-chance-profile.md)、[反応profile](legacy-reflection-profile.md)、[病気profile](legacy-disease-profile.md)、
[harms profile](legacy-harms-profile.md)と[光・闇profile](legacy-celestial-profile.md)と[攻撃奇跡profile](legacy-offensive-miracles-profile.md)と[防御奇跡profile](legacy-defensive-miracles-profile.md)と[属性付け替えprofile](legacy-attribute-change-profile.md)と[属性消去profile](legacy-attribute-removal-profile.md)と[反撃profile](legacy-counter-profile.md)と[反撃拡張profile](legacy-counter-extensions-profile.md)と[所持金反撃profile](legacy-money-counter-profile.md)と[魔法反射profile](legacy-magic-reflection-profile.md)と[万能反射profile](legacy-any-reflection-profile.md)と[MP武器profile](legacy-mana-weapon-profile.md)と[魔法遮断profile](legacy-magic-block-profile.md)を参照。
既存 `gl_*` は奇跡無効のprofileのみを生成し、従来の128次元・5種類のマスクを維持する。
`gl_action` の6byte構造は共通だが、`gl_*` はcastを受け付けない。

| 位置 | 内容 |
| --- | --- |
| 0〜127 | 従来の[中立観測](legacy-neutral-profile.md)。手札の奇跡はIDとHP回復量を記録し、通常の神器種別スカラーは0にする。奇跡の判別には後述の専用flagを使う |
| 128 | 保留中の効果が所持金増加なら1。従来のnone/damage/heal/mana用one-hotはこの場合全て0 |
| 129〜130 | 自分の保持数（種類index順）/ 意思決定上限 |
| 131 | 奇跡profileなら1 |
| 132〜147 | 自分の各手札位置が奇跡なら1 |
| 148〜163 | 自分の各手札位置の奇跡MPコスト / 99。通常の神器は0 |

相手の完全な保持数は観測にもマスクにも載せない。旧版の完全な保持一覧は本人向け情報であり、
使用履歴から分かる情報とは区別する。本環境は履歴からの知識推定を自動で状態へ加えない。
未実装の状態異常による情報変化も、現時点の対応範囲には含めない。

[MiraclesBatchEnv](../python/miracles_env.py) は依存追加なしのctypesラッパー。
返却するバッファーは再使用されるので、学習履歴に保存するときはコピーする。
不正行を含むバッチは全行を更新せず、終了行は凍結・報酬0・自動resetなしという契約を維持する。

## 実行と検証

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
ctest --test-dir build --output-on-failure
python3 python/miracles_env.py
./build/godfield_legacy_bench 100000 1 42 miracles
./build/godfield_legacy_bench 100000 4 42 miracles
```

ベンチマークのprofile引数を省略すると従来のneutral。
checksumはversion2として所持金・MP・保持奇跡も含めるため、以前報告したchecksum値とは直接比較しない。
初使用・再使用回数やMP消費を出力し、新機能が実際に実行されたことを確認できる。
速度には行動生成・状態更新・統計処理を含み、モデル推論・観測転送は含まない。

台帳生成の一致、局面ごとの保持・MP・配布、満杯からの再使用、秘匿、snapshotの復元、
バッチ境界、seed再現を検証する。旧Pythonとの自動比較や現行公式での動的照合は未実施。
属性付き攻撃奇跡、追加効果、反射・跳ね返し、守護神、状態異常、売買・資源交換等は未対応。

### 今回の検証結果（2026-10-31）

ReleaseのCTestは7件、ASan/UBSanビルドは5件が通過。
通常のPythonへsanitizer付き共有ライブラリーを直接読み込む検査は分けている。
3種類のPythonラッパーのsmokeも通過した。

10万試合・seed42・意思決定上限512のmiracles profileで次を確認した。

| 結果 | 1スレッドと4スレッドで一致した値 |
| --- | --- |
| 意思決定数 | 4,894,138 |
| 座席0 / 座席1の勝利数 | 50,075 / 49,925 |
| 打ち切り | 0 |
| 奇跡の初使用 / 再使用 | 20,378 / 31,578 |
| 合計MP消費 | 351,730 |
| 奇跡によるHP / 所持金増加 | 313,912 / 294,637 |
| checksum v2 | 11731554787046983901 |

参考速度は1スレッド約783万意思決定/秒、4スレッド約2,751万意思決定/秒。
同じマシンでの単発測定であり、モデル推論・観測転送は含まない。
neutral profileも以前と同じ4,724,594意思決定、勝利50,309/49,691となることを確認した。
