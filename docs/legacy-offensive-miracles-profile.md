# 攻撃奇跡と保持15種 v9

`legacy-offensive-miracles-subset-v9` は攻撃奇跡13種の初使用・保持・再使用を追加する限定profile。
CLIの `offensive-miracles` と `go_*` はv9までの拡張を有効にし、170種・旧重み合計358で動く。
旧非公式エミュレーターを参考にした実装で、現行公式の完全再現ではない。

## 対象と出典

[旧版処理の調査](legacy-offensive-miracles-rules.md)と
[台帳・出典・指紋](../data/legacy-offensive-miracles-v1/README.md)を参照。
全て単独使用の攻撃奇跡で、必中6種・確率付き7種。旧ID・旧数値を使う。

| 旧ID | 属性 | 威力 | MP | 命中率 | 付随効果 | 共通保持slot |
| --- | --- | --- | --- | --- | --- | --- |
| 198 | 水 | 4 | 2 | 必中 | なし | 2 |
| 199 | 木 | 6 | 3 | 必中 | なし | 3 |
| 200 | 光 | 1 | 3 | 25% | 正の残余でGlory | 4 |
| 203 | 土 | 8 | 4 | 必中 | なし | 5 |
| 204 | 火 | 5 | 4 | 75% | なし | 6 |
| 205 | 光 | 10 | 4 | 25% | なし | 7 |
| 206 | 火 | 10 | 5 | 必中 | なし | 8 |
| 207 | 闇 | 5 | 5 | 必中 | なし | 9 |
| 218 | 水 | 9 | 7 | 75% | なし | 10 |
| 219 | 土 | 13 | 8 | 50% | なし | 11 |
| 222 | 水 | 25 | 12 | 必中 | なし | 12 |
| 223 | 火 | 16 | 12 | 75% | なし | 13 |
| 226 | 水 | 30 | 30 | 75% | なし | 14 |

既存の回復216・所持金220はslot0/1のまま。新13種のlocal index0〜12に2を加えた値をcastに使う。
追加神器の旧重みは各1。既存8台帳を変更せず、限定プール内で重みを正規化する。
公式の配布率とは扱わない。218/219/223等は現行の抽出数値と差があり、現行神器への置換は行っていない。
追加ATKの197/217、吸収221、状態変化だけの奇跡、防御奇跡などは未対応。
未対応の効果を剥がした簡略神器は台帳に含めない。

## 使用と解決

初回は手札の `use` でMPを支払い、その1枚を消費して同種類の保持数を1増やす。
再使用は `cast` のbaseで共通保持slotを指定し、MPだけを支払う。保持数と手札は減らさない。
両方とも配布予定+1。手札が満杯の再使用でも配布を抽選・消費し、上限16を超える分は捨てる。
既存と同じく、全ての応答を解決した後、手番末にまとめて配る。
武器追加との併用は許可せず、use/castのslotsは0。
攻撃奇跡を持つだけでは祈りを禁止しない。祈りの条件は手札の武器の有無である。

必中奇跡は自分または相手を指定でき、自分への使用は応答を省略する。
確率付き奇跡は旧版では全ての敵へ各1回で、1対1では相手1人に限定する。
自己指定でも相手へ正規化し、同じ相手への連撃とは扱わない。
missでもMP支払い・初回保持・配布予定は発生し、応答は待たない。
DarkCloud対象では抽選せず必中となり、命中乱数と指定tapeを温存する。

属性に適合する通常防具で防御でき、Gloryの1枚制限も適用する。
武器用の跳ね返し20・反射39に相当する既存特殊応答は使えない。
魔法用反応は別効果として未対応のままにする。
200のGloryは命中して残余ダメージが正のときだけ付与し、Heavenなら転落を先に処理する。
207の闇は最終残余が正ならHP0、完全防御なら生存する。
光への防御の衝突C01、Gloryの1枚制限を旧botから採った点、病気自然悪化の衝突C02は残す。

## C++・観測

```cpp
godfield::legacy::Env env({512, 10, true, true, true, true, true, true, true, true});
env.reset(42);
```

末尾の `Config::offensive_miracles` はデフォルトfalse。
有効化にはmiraclesとcelestialが必要で、celestial経由でelements・chance・harmsも必須。
miracles/reflection/disease等の全設定を保存して再現する。CLIとC APIは全拡張有効。
`Player::abilities` は固定長15、`Options::ability_slots` は15bitを使うuint16のまま。
`ActionKind::cast` のbaseは0〜14。実際に保持しておりMPを払える場合だけ合法。
`StepResult::spell_first_use/spell_recast` は新旧15種の当該stepの使用を表す。
Snapshotの共通versionは35。旧形式を自動移行する永続checkpointとは扱わない。

共通C++観測は1317次元、`go_*` ABI 9は先頭334次元を返す。
v9では末尾983個が0になる。追加部分は[防御奇跡profile](legacy-defensive-miracles-profile.md)と[属性付け替えprofile](legacy-attribute-change-profile.md)と[属性消去profile](legacy-attribute-removal-profile.md)と[反撃profile](legacy-counter-profile.md)と[反撃拡張profile](legacy-counter-extensions-profile.md)と[所持金反撃profile](legacy-money-counter-profile.md)と[魔法反射profile](legacy-magic-reflection-profile.md)と[万能反射profile](legacy-any-reflection-profile.md)と[MP武器profile](legacy-mana-weapon-profile.md)と[魔法遮断profile](legacy-magic-block-profile.md)を参照。先頭301個は[光・闇profile](legacy-celestial-profile.md)を継承する。

| 位置 | 内容 |
| --- | --- |
| 301 | offensive_miracles有効なら1 |
| 302 | 未解決効果が奇跡なら1。新旧15種を含む |
| 303 | 未解決奇跡ID / 363 |
| 304 | 保持からの使用なら1 |
| 305〜317 | 自分の新13種の保持数 / 意思決定上限。local index順 |
| 318〜333 | 自分の各手札位置の攻撃奇跡威力 / 30。それ以外0 |

旧129〜130は旧2種の保持数だけを返し、既存flagを上書きしない。
手札の奇跡flag132〜147・MPコスト148〜163は新13種も表示する。
属性・命中率・Gloryの手札特徴は既存位置を使う。通常武器のATK欄へ奇跡を偽装しない。
相手の完全な保持数と手札、内部悪化率、未来の抽選値・乱数状態は出さない。
新13種が無効のprofileでは追加33特徴は0。
旧 `gl/gm/ge/gc/gr/gd/gh/gs` の128/164/186/210/236/258/283/301次元は維持する。

## C ABIとPython

[offensive_miracles_c_api.h](../include/godfield/offensive_miracles_c_api.h) の `go_*` と
[OffensiveMiraclesBatchEnv](../python/offensive_miracles_env.py) を使う。
6byteの `gl_action`、7種類の構成要素mask、固定座席報酬、全有効行を検証後に更新するバッチ契約を継承する。
終了行は固定・報酬0・自動resetなし。maskから構成した行動は `legal()` でも確認する。
Pythonの返却バッファーは再使用するので、履歴へ保存するときはコピーする。
同じハンドルを複数スレッドから同時操作しない。

```sh
python3 python/offensive_miracles_env.py
./build/godfield_legacy_bench 100000 1 42 offensive-miracles
./build/godfield_legacy_bench 100000 4 42 offensive-miracles
```

防御奇跡、追加魔法、吸収・資源交換・属性変更、霧・幻覚、守護神、蘇生、終末、複数人戦、
現行公式との動的照合と学習器は未対応。

## 検証結果（2026-10-31）

通常CTest 28件、ASan/UBSan 19件が通過。新旧9種類のPython suiteは計73テスト。
13種の初使用・再使用・MP不足、7種の確率境界、missでも保持される処理、通常防御と物理反応拒否、
Glory・Heaven・闇・所有者の病気tick、満杯手札での保持使用、秘匿・復元拒否、1000試合の再現を確認した。
Pythonからも通常resetを起点に初使用→再使用→MP不足、miss後の保持、Glory付与を検証した。

全機能有効・10万試合・seed42・意思決定上限512で、1/4スレッドの統計とchecksumが一致した。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 3,367,464 |
| 座席0 / 座席1勝利 | 49,687 / 49,767 |
| 勝者なし / 打ち切り | 546 / 0 |
| 攻撃奇跡の初使用 / 保持からの再使用 | 49,524 / 52,917 |
| 攻撃奇跡の発射 / 命中後の解決 | 102,441 / 77,169 |
| 攻撃奇跡のMP消費 / 新規保持数 | 451,404 / 49,524 |
| 攻撃奇跡による実HP損失 / 確率判定の試行 | 620,043 / 48,926 |
| 全奇跡の初使用 / 再使用 | 58,435 / 58,049 |
| Gloryの通常付与源 | 2 |
| checksum v9 | 4245841085504707796 |

実HP損失はHP上限処理後の減少で、自然tickを除く。完全防御も解決数へ含める。
発射にはmissを含む。必中sentinelは命中乱数を消費せず、DarkCloudによる強制命中も別に集計する。
参考速度は1スレッド約371万、4スレッド約1,376万意思決定/秒。
単発測定で、行動生成・合法性検証・遷移・統計・resetを含み、観測転送・モデル推論は含まない。
旧8profileのchecksum式を維持し、同条件の各10万試合で意思決定数・勝敗・checksumが以前と一致した。

共通観測の新しい追加部分は[HP吸収v28](legacy-hp-absorption-profile.md)を参照。旧ABIの幅・行動形式は維持する。
