# Glory・DarkCloud v7

`legacy-harms-subset-v7` は病気と独立したGlory/DarkCloud状態を追加する限定profile。
CLIと `gh_*` はv7までの拡張を有効にし、光・闇は無効のまま139種・旧重み合計327で動く。
旧Flash版の非公式実装を参考にしたもので、現行公式の完全再現ではない。

## 対象と根拠

[旧版の調査と留意点](legacy-harms-rules.md)、[台帳と出典](../data/legacy-harms-v1/README.md)を参照。
今回の追加神器は旧ID43、無属性ATK11、残りダメージが正なら対象へDarkCloudを付与する武器1種。
全防御では付与せず、反射された場合は現在の解決対象へ付与する。
旧重み1を既存プールへ追加して正規化する。公式の神器分布とは扱わない。

Gloryの付与元は光属性武器・反撃・奇跡などの未対応神器に限られる。
**通常抽選からGloryを獲得する経路はなく、C++のsnapshot fixtureで状態を設定して制限を検証する。**
その付随効果を省略した簡略神器は台帳に加えない。
光属性と通常のGlory付与元を扱う環境は別の[v8 profile](legacy-celestial-profile.md)として提供する。

Gloryの1枚防御制限は旧botの行動生成を根拠に採用した意図された規則。
旧サーバーの手動入力検証には同じ制限が確認できず、その受理挙動の完全再現とは区別する。
旧GLORYと現在のFlashという名称の対応も未確定。
霧・幻覚は未対応で、表示と真値、合法手マスクの秘匿設計は後続作業とする。

## 状態・命中・解除

Glory中の応答は、通常防御のslotsと特殊反応のbaseを合計して最大1枚。
防御なし、通常防具1枚、反射だけは可能。通常防具1枚と反射1枚の併用は不可。
この制限は現在の応答者に適用し、通常攻撃の組み合わせは制限しない。
`Options::defense_limit` とランダム方策も同じ制限を使う。

DarkCloudのある相手への確率武器は必中になり、命中乱数と指定命中tapeを消費しない。
自己指定を相手へ正規化した確率武器では、その実際の対象を調べる。
解除後の次の確率攻撃は、温存されていた命中値を使う。
反射・跳ね返しは命中済み攻撃を移動させるため、命中を再抽選しない。

harmの新規・重複付与は病気と悪化率を変えず、同じflagを重複して追加しない。
ただしHeavenなら重複判定より先に転落し、HP0・悪化率0になる。
この場合は新しいharmを追加せず、以前からのflagは残す。
Glory/DarkCloud自身に自然悪化や手番末の継続効果はない。

| 薬 | 病気 | 今回のharms |
| --- | --- | --- |
| 旧ID235・軽い解除 | Cold/Feverを消す。Hell/Heavenとその悪化率は保持 | Gloryを消し、DarkCloudは残す |
| 旧ID236・全解除 | 全て消し、悪化率0 | 両方消す |

軽い解除は病気がなくてもGloryを消せる。解除そのものはHeavenの転落を起こさない。
病気のtick、暫定HP0の反応連鎖、両者死亡時の学習規約は[病気profile](legacy-disease-profile.md)を引き継ぐ。

## C++と観測

```cpp
godfield::legacy::Env env({512, 10, true, true, true, true, true, true});
env.reset(42);
```

`Config::harms` はデフォルトfalse。他のflagとは独立で、harms単独構成も可能。
再現にはprofile名だけでなく全Configを保存する。上の構成とCLI・C APIはv7までの拡張有効。
Snapshotの共通versionは37。異なる設定・旧version、不正なharm・無効設定中の状態flagなどを拒否する。
これは同じビルド内のfixture形式で、永続checkpointの互換形式ではない。

Playerは公開状態の `glory/dark_cloud`、Pendingは `harm_card/harm` を持つ。
`LastChance::forced` は最後の確率攻撃がDarkCloudによる必中だったかを記録する。
過去の記録なので、後に解除された現在の状態と一致する必要はない。
`StepResult` は当該stepの `chance_forced`、harm付与試行数・新規flag数・Heaven転落数、
Glory中の応答だったかを返す。

共通C++観測は1368次元、`gh_*` ABI 7は先頭283次元を返す。v7では末尾1085個が0になる。
追加部分は[光・闇profile](legacy-celestial-profile.md)と[攻撃奇跡profile](legacy-offensive-miracles-profile.md)と[防御奇跡profile](legacy-defensive-miracles-profile.md)と[属性付け替えprofile](legacy-attribute-change-profile.md)と[属性消去profile](legacy-attribute-removal-profile.md)と[反撃profile](legacy-counter-profile.md)と[反撃拡張profile](legacy-counter-extensions-profile.md)と[所持金反撃profile](legacy-money-counter-profile.md)と[魔法反射profile](legacy-magic-reflection-profile.md)と[万能反射profile](legacy-any-reflection-profile.md)と[MP武器profile](legacy-mana-weapon-profile.md)と[魔法遮断profile](legacy-magic-block-profile.md)を参照。先頭258個は[病気観測](legacy-disease-profile.md)を維持する。

| 位置 | 内容 |
| --- | --- |
| 258〜261 | 自分Glory、自分DarkCloud、相手Glory、相手DarkCloud |
| 262 | harms有効なら1 |
| 263 | 有効な応答中の防御枚数上限 / 16。それ以外0 |
| 264 | 最後の確率攻撃が強制命中なら1 |
| 265 | 保留効果のharm enum / 2。none=0、glory=1、dark_cloud=2 |
| 266 | 保留中のharm神器ID / 363 |
| 267〜282 | 自分の各手札位置のharm enum / 2 |

相手の手札・内部悪化率・未来の抽選値・乱数状態は含めない。
従来profileの追加25特徴は0。`gl/gm/ge/gc/gr/gd` の観測サイズは128/164/186/210/236/258のまま。

## C ABIとPython

[harms_c_api.h](../include/godfield/harms_c_api.h) の `gh_*` と
[HarmsBatchEnv](../python/harms_env.py) を使う。GitHub CLIのghコマンドとは無関係。
6byteの `gl_action`、7種類の構成要素maskを継承する。
maskだけで複数防御の制限は表せないため、283観測の上限を参照して構成し、`gh_legal` でも確認する。
`gh_sample_actions` はこの制限を守る。

バッチの全有効行を検証してから更新し、不正行があれば全行を更新しない。
終了行は固定・報酬0・自動resetなし。報酬は固定座席 `[player0, player1]`。
Pythonの返却バッファーは再使用するので、履歴に保存するときはコピーする。
同じハンドルを複数スレッドから同時操作しない。
命中・跳ね返し対象・病気悪化のfixture prefixを設定できるが、公開APIにGlory注入は追加していない。

```sh
python3 python/harms_env.py
./build/godfield_legacy_bench 100000 1 42 harms
./build/godfield_legacy_bench 100000 4 42 harms
```

## 検証結果（2026-10-31）

通常CTest 22件、ASan/UBSan 15件が通過。
Gloryの防御制限・合法手生成、DarkCloudの付与・命中乱数非消費・解除、重複とHeaven、反射、
観測の秘匿、復元拒否、harms単独構成、1000seedの再現を検証した。
Pythonからも通常resetを起点にID43による付与→指定100でも必中→全解除→温存100でmissまで確認した。

全機能有効・10万試合・seed42・意思決定上限512で、1/4スレッドの統計とchecksumは一致した。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 4,354,794 |
| 座席0 / 座席1勝利 | 49,575 / 49,714 |
| 勝者なし / 打ち切り | 711 / 0 |
| 確率攻撃 / DarkCloud強制命中 | 136,143 / 2,935 |
| harm付与試行 / 新規flag / Heaven転落 | 10,847 / 10,394 / 224 |
| Glory応答 / 通常プールのGlory付与源 | 0 / 0 |
| checksum v7 | 16876163863414638204 |

参考速度は1スレッド約539万、4スレッド約1,873万意思決定/秒。
単発測定で、行動生成・合法性検証・遷移・統計・resetを含み、観測転送・モデル推論は含まない。
既存6profileのchecksum式は維持し、同条件の各10万試合で意思決定数・勝敗・checksumが以前と一致した。

共通観測の新しい追加部分は[HP吸収v28](legacy-hp-absorption-profile.md)を参照。旧ABIの幅・行動形式は維持する。
