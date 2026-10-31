# 確率攻撃を有効にした旧版ルールセット v4

`legacy-elements-miracles-chance-subset-v4` は、中立60種・単純奇跡2種・基本4属性54種・
単純な確率武器12種の計128種で動く限定profile。現行公式の完全再現ではない。

## 範囲と旧版の意味

[旧版の確率攻撃の調査](legacy-chance-rules.md)、[台帳と出典](../data/legacy-chance-v1/README.md)を参照。
旧版の `%ATK` は生存する全敵へそれぞれ1件の攻撃を作る。今回の1対1では相手への1件になる。
同じ相手への連続攻撃という解釈を導入していない。連撃は参照ソースから確認できず、未対応のまま残す。

| 項目 | 追加範囲 |
| --- | --- |
| 確率武器 | 12種。全て火・水・木・土、旧重み各1 |
| 命中率 | 25%が2種、50%が3種、75%が7種 |
| 兼用防御 | 旧ID113は木属性の防御値6も保持する |
| 全抽選プール | 128種、重み合計293。制限プール内の旧重み正規化であり公式分布ではない |
| 除外 | 光・闇、状態異常、吸収、特殊防御、追加属性等を持つ確率武器 |

旧IDと技術ラベルを使い、現行の公式IDや神器名との対応を確定したとは扱わない。
未対応効果を削って通常武器として使うこともしない。

## 状態遷移

1. 武器・対象・追加選択を検証する。確率武器には追加ATK等を組み合わせられない。
2. 基底1枚を消費して手札を詰め、使用者の配布予定数を1増やす。
3. 1〜100の整数を抽選し、`roll <= hit_rate` なら命中とする。
4. 命中なら相手の防御応答を待つ。命中判定済みとして保存し、防御や復元で再抽選しない。
5. 失敗なら防御応答を挟まず、ダメージなしで手番末の配布と次の手番へ進む。

通常の必中武器は命中率0を旧版のsentinelとして扱い、命中用抽選を消費しない。
今回の確率台帳は25/50/75だけ。旧版における100の乱数消費の意味は調査記録に分けて記載している。
確率武器を防御として使う場合も命中抽選は行わない。

確率武器は全敵への攻撃なので、対象に自分を指定しても実行先は相手へ正規化する。
指定値0/1はどちらも受理し、存在しない対象は拒否する。必中武器の自分への攻撃は従来どおり別の挙動。
神器を失敗時に手札へ戻したり、防御を選ばせたりしない。
1対1に限定しているため、複数の敵へのキュー処理を完了したという主張はしない。

## C++ API・検証用乱数

```cpp
godfield::legacy::Env env({512, 10, true, true, true});
env.reset(42);
```

`Config::chance` はデフォルトfalse。chanceを有効にするにはelementsも有効にする必要があり、
`chance=true/elements=false` は構築時に拒否する。
奇跡を省いた `{512,10,false,true,true}` もC++では利用可能で、
識別名は `legacy-elements-chance-subset-v4`。CLIと `gc_*` は奇跡も有効にする。

Actionの形式は変更しない。基底が確率武器ならslotsは0に限る。
構成要素のmaskだけではこの制約を表せないので、組み立てた行動を `legal()` で検証する。
ランダム方策は基底の種類を見て、確率武器の追加選択を空にする。

`StepResult::chance_attempt/chance_hit` は、そのstepで実際に確率攻撃を開始したかと命中結果を返す。
通常の防御stepや終了済み環境のstepではfalse。ベンチマークはこの値を集計する。
`State::last_chance` は最後の確率攻撃の公開記録であり、その後の通常行動でも保持する。
神器ID、使用者、実際の対象、表示命中率、命中/失敗を記録し、抽選値そのものは公開しない。

`script_hit_rolls()` は最大64件、1〜100の抽選値を指定するfixture用入口。
指定部分ではゲーム乱数を進めず、使い切るとseed付き抽選へ戻る。
通常攻撃や防御はこのprefixを消費しない。無効な値・長さは原子的に拒否する。
chance無効profileでは空のprefixによるクリアだけを許可する。
指定した未来の値・現在位置・乱数状態は観測や合法手maskへ出さない。

Snapshotは反撃拡張と共通のversion37。公開記録、命中済みの保留攻撃、未使用の抽選prefixと現在位置を含む。
復元時はprofile、形式、率と神器の対応、source/target、配布予定などを検証する。
旧versionをそのまま受け付けない。永続checkpointや異なるビルド間の互換形式はまだ未実装。

## 観測とC ABI

C++の共通観測は反撃拡張後の1368次元で、`gc_*` ABI 4は先頭210次元を返す。
この確率profileでは末尾1158個は0。定義は[反応profile](legacy-reflection-profile.md)、
[病気profile](legacy-disease-profile.md)、[harms profile](legacy-harms-profile.md)と[光・闇profile](legacy-celestial-profile.md)と[攻撃奇跡profile](legacy-offensive-miracles-profile.md)と[防御奇跡profile](legacy-defensive-miracles-profile.md)と[属性付け替えprofile](legacy-attribute-change-profile.md)と[属性消去profile](legacy-attribute-removal-profile.md)と[反撃profile](legacy-counter-profile.md)と[反撃拡張profile](legacy-counter-extensions-profile.md)と[所持金反撃profile](legacy-money-counter-profile.md)と[魔法反射profile](legacy-magic-reflection-profile.md)と[万能反射profile](legacy-any-reflection-profile.md)と[MP武器profile](legacy-mana-weapon-profile.md)と[魔法遮断profile](legacy-magic-block-profile.md)を参照。
既存 `gl_*` は128、`gm_*` は164、`ge_*` は186の出力を維持する。
各旧prefixが返す行の幅でコピーするため、コアの拡張で既存バッファーを超えて書き込まない。

| 位置 | 内容 |
| --- | --- |
| 0〜185 | [属性profile](legacy-elements-profile.md)の定義を維持 |
| 186〜201 | 自分の各手札位置の命中率 / 100。必中sentinel・空き・武器以外は0 |
| 202 | 命中済みの未解決攻撃の命中率 / 100。通常攻撃等は0 |
| 203 | chance profile有効なら1 |
| 204 | 公開の最後の確率攻撃記録があれば1 |
| 205 | 最後の確率攻撃が命中なら1 |
| 206 | 最後の確率武器ID / 363 |
| 207 | 最後の確率武器の命中率 / 100 |
| 208 | 記録があり、その使用者が現在の行動プレイヤーなら1 |
| 209 | 記録があり、その対象が現在の行動プレイヤーなら1 |

chance無効profileでは追加24個は全て0。
相手の隠れた手札の命中率や、未来の抽選結果は含めない。
成分mask6種類、共通6byteの `gl_action`、固定座席の報酬という契約は維持する。

[ChanceBatchEnv](../python/chance_env.py) は外部依存なしのctypesラッパー。
`gc_script_hit_rolls` は指定した環境行へ検証用prefixを設定する。
不正なバッチ行動は全行を更新せず、終了行は状態固定・報酬0・自動resetなし。

## 実行と対応限界

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
ctest --test-dir build --output-on-failure
python3 python/chance_env.py
./build/godfield_legacy_bench 100000 1 42 chance
./build/godfield_legacy_bench 100000 4 42 chance
```

CLIのchanceはv4までの機能を有効にし、反応・病気・harmsは無効のまま、命中の試行・成功・失敗を出力する。
chanceのchecksumはversion4。既存neutral/miracles/elementsの計算式は変更していない。
速度には行動生成・状態更新・統計処理を含み、モデル推論・観測転送は含まない。

検証には各確率の境界、hit/missの応答、消費と補充、追加選択の禁止、対象の正規化、
途中状態の復元、不正手の原子性、秘匿、打ち切り、1000seed再現を含む。
命中用prefixは旧版や公式と同じseedを再現するためのものではない。
現行公式との動的照合、連撃、複数人への確率攻撃、反射・状態異常等は未対応。

### 今回の検証結果（2026-10-31）

通常ビルドのCTestは13件、ASan/UBSanビルドは9件が通過。
chance profileの10万試合・seed42・意思決定上限512で、1スレッドと4スレッドの次の値が一致した。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 4,928,832 |
| 座席0 / 座席1の勝利数 | 50,022 / 49,978 |
| 打ち切り | 0 |
| 確率攻撃の試行 / 命中 / 失敗 | 172,367 / 103,157 / 69,210 |
| 奇跡の初使用 / 再使用 | 16,614 / 20,974 |
| checksum v4 | 9538068937042251482 |

参考速度は1スレッド約581万意思決定/秒、4スレッド約2,180万意思決定/秒。
単発の計測で、モデル推論・観測転送は含まない。
neutral/miracles/elementsの同条件10万試合も、以前の意思決定数・勝敗・checksumを維持した。

共通観測の新しい追加部分は[HP吸収v28](legacy-hp-absorption-profile.md)を参照。旧ABIの幅・行動形式は維持する。
