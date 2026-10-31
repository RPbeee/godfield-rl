# 基本4属性を有効にした旧版ルールセット v3

`legacy-elements-miracles-subset-v3` は、中立60種・単純奇跡2種・4属性神器54種の計116種で
自己対戦する限定profile。現行公式の全神器・全属性への対応ではない。
属性付きの攻撃奇跡、光・闇、状態異常、属性の強制付け替え、特殊防御などは含めない。

## 追加範囲と根拠

[旧版の属性処理の調査](legacy-elements-rules.md)と
[台帳・出典](../data/legacy-elements-v1/README.md)を参照。
台帳は旧ID・技術ラベルで管理し、現在の公式ID対応表とは扱わない。
元のデータ行、正規化データ、生成器、fingerprintを保存する。

| 対象 | 追加した内容 |
| --- | --- |
| 属性 | 無属性に加え、火・水・木・土 |
| 属性武器 | 単純な確定攻撃23種。そのうち追加ATK武器7種、攻防兼用武器1種 |
| 属性防具 | 通常防御31種 |
| 重み | 追加54種は旧データの重み各1。中立225＋奇跡2＋属性54の合計281で復元抽出 |
| 除外 | 光・闇、ADD_ATTRIBUTE、確率付き攻撃、状態異常、反射・跳ね返し・COUNTER等 |

除外された神器から特殊効果だけを取り除いて通常攻撃に変えることはしない。
旧版の神器分布自体に不確かさがあり、対象プール内で正規化するため、公式の配布分布とは異なる。

## 攻撃の属性合成

先頭の武器の属性を入れ、追加ATKを持つ神器の属性も含めて合成する。
今回の5属性では「全て同じならその属性、1つでも異なるなら無属性」となる。
無属性は「まだ属性を選んでいない」という意味ではない。

| 組み合わせ | 結果 |
| --- | --- |
| 火＋火の追加ATK | 火 |
| 火＋水の追加ATK | 無属性 |
| 火＋無属性の追加ATK | 無属性 |
| 無属性＋火の追加ATK | 無属性 |
| 火＋水＋火の追加ATK | 無属性 |

異なる属性を選ぶこと自体は合法。追加ATKを先頭と同属性だけに制限しない。
今回の範囲では順序に依存しないため、既存の追加スロットmaskで意味を失わずに扱える。
属性を強制上書きするADD_ATTRIBUTEは順序に依存するので対象外。将来対応時は行動表現を拡張する。

## 防御

| 攻撃の最終属性 | 適合する最終防御属性 |
| --- | --- |
| 無属性 | 無属性・火・水・木・土 |
| 火 | 水 |
| 水 | 火 |
| 木 | 土 |
| 土 | 木 |

選んだ防御神器の属性も合成し、最終属性で適合を判定する。
火攻撃に水防具だけなら合法だが、水防具＋無属性防具は最終無属性となり違法。
防御力の合計が攻撃力を超えても、属性の不適合は解消しない。
0枚の防御は常にそのまま受ける選択として扱う。

この限定profileでは、属性攻撃に適合する防具は全て同じ相克属性になる。
そのため構成要素maskから不適合な単品を除いても、合法な組み合わせを失わない。
加えて `legal()` は最終合成属性も検証する。
光の代用・強制属性変更・特殊防御を追加した後も、この単品filterが正しいとは仮定しない。

## C++ APIとprofileの分離

```cpp
// 中立・奇跡・基本4属性。CLIとge_*が使う構成。
godfield::legacy::Env env({512, 10, true, true});
env.reset(42);

// 奇跡を含めない、中立・4属性だけのC++構成も可能。
godfield::legacy::Env elements_only({512, 10, false, true});
```

`Config::elements` はデフォルトfalse、`miracles` とは独立。
属性のみの識別名は `legacy-elements-subset-v3`、奇跡併用時は
`legacy-elements-miracles-subset-v3`。`Env::profile()` で取得する。
手札容量・資源・通常手番・消費と配布の処理は既存の旧版profileを引き継ぐ。

`Pending::element` に未解決攻撃の属性を保持する。回復・所持金増加等では無属性に限る。
Snapshotの共通形式は反撃拡張後のversion36。異なるprofile・古い形式・未対応ID・不正な属性enumを拒否する。
属性無効のprofileへ、同じ設定を装って色付き神器や攻撃を入れることも拒否する。

## 観測とABI

C++の `Env::observe()` は反撃拡張後の1345次元で、`ge_*` ABI v3は先頭186次元を返す。
この属性profileでは末尾1159個は0。定義は[確率profile](legacy-chance-profile.md)、
[反応profile](legacy-reflection-profile.md)、[病気profile](legacy-disease-profile.md)、[harms profile](legacy-harms-profile.md)と[光・闇profile](legacy-celestial-profile.md)と[攻撃奇跡profile](legacy-offensive-miracles-profile.md)と[防御奇跡profile](legacy-defensive-miracles-profile.md)と[属性付け替えprofile](legacy-attribute-change-profile.md)と[属性消去profile](legacy-attribute-removal-profile.md)と[反撃profile](legacy-counter-profile.md)と[反撃拡張profile](legacy-counter-extensions-profile.md)と[所持金反撃profile](legacy-money-counter-profile.md)と[魔法反射profile](legacy-magic-reflection-profile.md)と[万能反射profile](legacy-any-reflection-profile.md)と[MP武器profile](legacy-mana-weapon-profile.md)と[魔法遮断profile](legacy-magic-block-profile.md)を参照。
既存 `gl_*` は128次元、`gm_*` は164次元の出力を維持する。
先頭の既存特徴を移動せず、末尾に属性情報を追加した。

| 位置 | 内容 |
| --- | --- |
| 0〜163 | [奇跡profileの観測](legacy-miracles-profile.md)を維持。属性神器も通常の役割・攻撃・追加ATK・防御等をこの部分へ出力 |
| 164〜168 | 未解決攻撃属性のone-hot。無属性・火・水・木・土の順。属性有効profileの通常フェーズも無属性の位置が1 |
| 169〜184 | 自分の各手札位置の属性enum / 4。空きと無属性は0 |
| 185 | 属性有効profileなら1 |

属性無効profileでは追加22個は全て0。
相手の隠れた手札の属性は出力しない。公開の未解決攻撃の属性と、自分の手札だけを追加する。
`Options` は既存どおり武器・追加ATK・防御・使用・捨て・保持奇跡の6mask。
意味的Actionと6byteのC構造のレイアウトは変えない。

[ElementsBatchEnv](../python/elements_env.py) は外部依存なしのPythonラッパー。
`ge_create` は中立＋奇跡＋4属性の構成を生成し、ABI 3・186特徴・6maskを返す。
不正なバッチ行動は全行を更新せず、終了行は凍結・報酬0・自動resetなし。

## 実行と検証

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
ctest --test-dir build --output-on-failure
python3 python/elements_env.py
./build/godfield_legacy_bench 100000 1 42 elements
./build/godfield_legacy_bench 100000 4 42 elements
```

CLIの第4引数elementsは属性と奇跡を両方有効にし、それより後の拡張は無効にする。
他のprofileの選択肢は[README](../README.md)を参照。
ベンチマークは属性攻撃回数と有効属性防御を数える。
elementsのchecksumはversion3で属性を含む。他の2profileはchecksum version2を維持する。
速度は行動生成・状態更新・統計処理を含み、モデル推論・観測転送は含まない。

検証には5×5相性、属性の混在、攻防兼用武器、全16枚選択、不正手の原子性、
観測秘匿、奇跡併用、profile境界、1000seedの再現・復元を含む。
Python/C ABIでも通常の対戦状態から、不適合な混合防御の拒否と出力境界を確認する。
これらは旧版を参考にした限定環境の検証であり、現行公式との動的照合の実績ではない。

光・闇の既知の仕様衝突は未解消。属性付き奇跡、確率攻撃、状態異常、反射、守護神、
売買・資源交換、蘇生、終末、複数人戦は引き続き未対応。

### 今回の検証結果（2026-10-31）

通常ビルドのCTestは10件、ASan/UBSanビルドは7件が通過。
属性profileの10万試合・seed42・意思決定上限512で、1スレッドと4スレッドの次の値が一致した。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 5,047,358 |
| 座席0 / 座席1の勝利数 | 50,094 / 49,906 |
| 打ち切り | 0 |
| 属性攻撃 / 無属性攻撃 | 285,612 / 1,294,536 |
| 有効属性防御 / 防いだダメージ | 13,007 / 37,030 |
| 奇跡の初使用 / 再使用 | 17,566 / 23,832 |
| checksum v3 | 9271156850181878977 |

参考速度は1スレッド約699万意思決定/秒、4スレッド約2,497万意思決定/秒。
単発の計測であり、モデル推論・観測転送は含まない。
neutralとmiraclesの同条件10万試合も、それぞれ以前の意思決定数・勝敗・checksum v2を維持した。
