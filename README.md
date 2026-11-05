# Godfield RL core

**公式公開資料のv2同期（2026-11-02）:** [official-v2](data/official-v2/README.md) に最新版JA/JSの出典を別世代として固定し、ID19/26/117の画像名と117「石板」を反映した。296モデルの数値・能力・curse・giftRate・ID順はv1から変更0。[v1→v2比較](data/official-v2/upgrade-review.json) を保存し、旧official-v1のpins・crosswalk・legacy37 fingerprintは変更しない。新JSの抽出関数/分岐の再照合は下記v27節に記録する。全JS・全UI・権威的serverの同値性を保証するものではない。過去のfresh一致記録と各版の計測は、当時の取得物に対する履歴として保持する。

God Fieldを強化学習で扱うための、C++20製ローカルシミュレーターです。
描画・通信・外部ライブラリに依存せず、自己対戦を高速に回すための基盤を用意しています。

現行公式については、公開台帳296モデル、公開クライアントの計算・選択関数、
最大9人のデコード済みスナップショットからの状態欄・手札表示の部分投影を独立実装しました。
公式台帳の通常配布は正のgiftRateを持つ237モデル・重み合計500です。
`godfield::official` は順序付き威力/属性/MP表示、反応・属性防御、見えているモデル列への追加可否を計算し、
実際の公式JSをオフライン実行した50,270件のgolden vectorと一致しました。
状態表示についても、実関数と抽出した分岐による22,140件が一致しました。
後者はtooltip等の描画部品をstubに置き換えた照合を含み、全UI操作や対局の再現ではありません。
`gfo_*` ABI1とPython `OfficialData` は台帳・合成計算、追加の `gfo_state_*` と
Python `official_state.py` は入力スナップショットの読み取り・view・選択照会を公開します。
これらのread-only APIに加え、`godfield::official::Match` / `gfo_match_*` / Python `Match` で
既定236モデル・重み499、`include_fate=True` で237モデル・重み500の2〜9人/チーム対局を実行できます。
配布・死亡・手番のscheduleは実験規約であり、公式サーバー同等性は未検証です。

[公式一次調査](docs/official-current-rules.md)、[状態と表示の契約](docs/official-state-view.md)、
[限定実行器の契約](docs/official-execution-subset.md)、
[適合計画と未完了条件](docs/official-conformance-plan.md)、
[296件の出典・取得hash・旧版比較](data/official-v2/README.md)を参照してください。
限定範囲では主動作・応答・効果・死亡・補充・次手番・チーム終局を進め、物理反射・属性消去・遮断・跳ね返し・FREE・属性付替・倍化・単純確率攻撃・攻撃/資源反撃4種・HP吸収/同被害・閃光/暗雲の付与/反撃/解除・未使用手札/使用済み奇跡の除去・MP武器58・二回攻撃14・全敵化232・HP増減202・自動復活208・杵94/臼209・両替3・売却4・購入確認付きBUY5・病気4状態と付与6種・自己病気防具168/MP+Heaven雑貨198・Fog付与4種と観測/対象選択・112の通常/死亡時攻撃・Dream4種・178のredrawと互換クラス限定maskを扱います。201/239による召喚とcanonical42の自動行動・離脱を実験規約で扱い、外部指定行動に加え、typed Earth/Moon payloadと限定ローカル選出・任意設定の自動発動、global現象5種・応答付きのsourceなし現象3種・明示permutationによる磁気嵐295の別入口も扱います。混乱289と外部指定主動作に加え、既存ローカル方策を使う混乱用サンプラーも扱います。公式の自動選択分布は未確認です。所有する207と外部指定結果を通常手番として解決するFate専用入口と、全10種の結果を生成するローカルchance helperも扱います。悪魔5種は外部指定の直接効果として扱います。終末GFの閾値と「ささげる」は限定した通常操作として扱います。opt-inでは予約補充をphase4の外部chanceで進められ、公開率に従うローカル `sample_grant` も使えます。
237種の通常配布は任意設定で扱います。全固有効果、一般effect queue、公式Dream全候補分布・実体不適合入力、取引server挙動の未確定部分、Earth/Moonの公式一般神器/奇跡選出・207の公式現象分布・混乱の公式自動選択分布・全補充経路の悪魔出現/再授与連鎖等の完全な終末処理には未対応です。
旧版37profileの対戦Env・C ABI・Pythonは保存し、公式版へ改称しません。学習器と学習済みモデルは含めません。

**旧Flash版を参考に、全7属性・回復/攻撃/状態・解除/追加ATK/倍化/全体化奇跡の保持・順序付き追加・物理品238の追加ATKとMP無料化・確率攻撃・反撃6種・HP吸収・HP増減賭け・購入確認と強制売却・杵/臼・自己被害付き武器・手札除去・忘却・資源交換・自動蘇生・物理品による魔法跳ね返し・魔法/万能反射・病気・一部harmsまで実装しています。現行公式の完全再現ではありません。**

| ルールセット / API | 用途 | 神器・接口 |
| --- | --- | --- |
| `official-catalog-client-rules-v2` / fresh state view | 現行公式の台帳・公開clientの表示計算、最大9人の状態の部分投影と見かけに基づく選択照会。対局実行不可 | 296モデル、`gfo_*` ABI1と `gfo_state_*` / Python `OfficialData`・`official_state.py` |
| `official-execution-subset-v60` | 公式データによる実験規約の2〜9人/チーム対局。公式serverとの同等性未検証 | 既定236/499・任意設定237/500、全7属性・単純回復・奇跡保持・反射/消去/遮断/跳ね返し・FREE/属性付替/倍化・単純確率攻撃・攻撃/資源反撃4種・HP吸収/同被害・閃光/暗雲・除去2種・MP武器58/二回攻撃14・全敵化232・HP増減202・自動復活208・杵94/臼209・両替3・直近除去結果の公開・売却4・自己照会を含む購入5・病気4状態/付与6種・168/198・Fog4種・死亡時攻撃112・Dream4種・178redrawと互換クラス限定mask・201/239召喚・canonical42自動行動/離脱と外部指定行動/typed Earth・Moon payload・別入口のglobal現象5種/sourceなし現象3種/typed磁気嵐295/混乱289・外部指定主動作・ローカル混乱サンプラー/外部結果付き207/外部指定の悪魔5種/終末閾値とささげる/opt-in補充chance phase4と公開率によるローカルサンプラー、`gfo_match_*` ABI2 / Python `Match` |
| `legacy-dying-weapon-subset-v37` | 死亡時効果117の到達不能性を固定したduelで、光ATK1・75%の通常使用を追加 | 231種、1368特徴、88byte行動、`g37_*` |
| `legacy-status-miracles-subset-v36` | 解除202/211とDarkCloud208・Cold212・Heaven224の保持・FREE・27種忘却を加えた旧版の部分実装 | 230種、1345特徴、88byte行動、`g36_*` |
| `legacy-sundry-boost-subset-v35` | SUNDRY238の物理boost10と順序付き追加・57リセット・売買を加えた旧版の部分実装 | 225種、1317特徴、88byte行動、`g35_*` |
| `legacy-magic-bounce-subset-v34` | MAGICを跳ね返す物理6種と、21のATK/boost・5防具の普通DEFを加えた旧版の部分実装 | 224種、1300特徴、88byte行動、`g34_*` |
| `legacy-hp-gamble-subset-v33` | 237の使用時±10抽選・公開符号・反射後の現在targetへの実HP変化を加えた旧版の部分実装 | 218種、1278特徴、88byte行動、`g33_*` |
| `legacy-buying-subset-v32` | BUY4の手札1コピー抽選・現金条件・購入確認phaseと物理配送を加えた旧版の部分実装 | 217種、1251特徴、88byte行動、`g32_*` |
| `legacy-selling-subset-v31` | SELL3による商品の先行消費・強制売却と、満杯配送失敗でも支払う資源処理を加えた旧版の部分実装 | 216種、1221特徴、88byte行動、`g31_*` |
| `legacy-pestle-mortar-subset-v30` | 杵99の臼所持者検索/raw999・無臼時ランダム対象と、臼245の捨てskipを加えた旧版の部分実装 | 215種、1175特徴、88byte行動、`g30_*` |
| `legacy-self-damage-subset-v29` | 武器53の現在source先行被害・target被害と自己対象の二段減算を加えた旧版の部分実装 | 213種、1135特徴、88byte行動、`g29_*` |
| `legacy-hp-absorption-subset-v28` | 武器26/42/103と魔法221の残余全量HP吸収・現在sourceへの先行回復を加えた旧版の部分実装 | 212種、1110特徴、88byte行動、`g28_*` |
| `legacy-item-removal-subset-v27` | SUNDRY240による手札コピー1枚の無作為除去・対象補償なしを加えた旧版の部分実装 | 208種、1085特徴、88byte行動、`g27_*` |
| `legacy-revival-subset-v26` | 死亡確定前の244自動HP10回復と、固定座席順/病気/反撃の境界を加えた旧版の部分実装 | 207種、1062特徴、88byte行動、`g26_*` |
| `legacy-exchange-subset-v25` | 交換2によるHP/MP/YEN総和保存・自己再配分を加えた旧版の部分実装 | 206種、1043特徴、88byte行動、`gg_*` |
| `legacy-forgetting-subset-v24` | SUNDRY241による保持コピー1個の加重忘却と、属性None/196反射を加えた旧版の部分実装 | 205種、1017特徴、88byte行動、`gj_*` |
| `legacy-magic-free-subset-v23` | MAGIC_FREE5種と隣接無料化・未消費suffix・無料の保持応答を加えた旧版の部分実装 | 204種、993特徴、88byte行動、`gi_*` |
| `legacy-wide-miracles-subset-v22` | MP5の全体化209と100%抽選・保持反復を加えた旧版の部分実装 | 199種、945特徴、88byte行動、`gt_*` |
| `legacy-doubling-miracles-subset-v21` | MP6の倍化214と保持反復・順序付き合成を加えた旧版の部分実装 | 198種、924特徴、88byte行動、`gq_*` |
| `legacy-additive-miracles-subset-v20` | 追加ATK奇跡197/217の単独使用・保持反復・順序付き武器合成を加えた旧版の部分実装 | 197種、903特徴、88byte行動、`gp_*` |
| `legacy-magic-block-subset-v19` | 魔法遮断8種を加え、攻撃・通常防御・55の追加ATKも保つ旧版の部分実装 | 195種、750特徴、16byte行動、`gw_*` |
| `legacy-mana-weapon-subset-v18` | 全MPを威力へ変える武器57と、MP0の威力0応答を加えた旧版の部分実装 | 187種、732特徴、16byte行動、`gv_*` |
| `legacy-any-reflection-subset-v17` | 先頭196の万能反射・反撃の返却・死亡対象への取消を加えた旧版の部分実装 | 186種、713特徴、16byte行動、`gu_*` |
| `legacy-magic-reflection-subset-v16` | 魔法反射4種を加え、奇跡の来歴と効果を保って返す旧版の部分実装 | 185種、696特徴、16byte行動、`gz_*` |
| `legacy-money-counter-subset-v15` | 不足額をMP・HPで支払う所持金反撃194を加えた旧版の部分実装 | 181種、679特徴、16byte行動、`gy_*` |
| `legacy-counter-extensions-subset-v14` | 火確率・直接Glory/DarkCloud・自己MP回復と混合反撃順を加えた旧版の部分実装 | 180種、661特徴、16byte行動、`gx_*` |
| `legacy-counter-subset-v13` | 土の倍返し190・項目キュー・死亡確定を加えた旧版の部分実装 | 176種、515特徴、6byte行動、`gk_*` |
| `legacy-attribute-removal-subset-v12` | 防御時に攻撃属性を消す神器195を加えた旧版の部分実装 | 175種、393特徴、6byte行動、`gn_*` |
| `legacy-attribute-change-subset-v11` | 攻撃属性を付け替える2種を加えた旧版の部分実装 | 174種、375特徴、6byteの3結果指定、`ga_*` |
| `legacy-defensive-miracles-subset-v10` | 防御奇跡2種の初使用・保持応答を加えた旧版の部分実装 | 172種、356特徴、保持17種、`gb_*` |
| `legacy-offensive-miracles-subset-v9` | 攻撃奇跡13種の初使用・保持・再使用を加えた旧版の部分実装 | 170種、334特徴、保持15種、`go_*` |
| `legacy-celestial-subset-v8` | 光・闇の攻防、闇の即死、通常のGlory付与を加えた旧版の部分実装 | 157種、301特徴、`gs_*` |
| `legacy-harms-subset-v7` | Dark CloudとGloryの状態処理を加えた旧版の部分実装 | 139種、283特徴、`gh_*`。Gloryの付与元は未対応 |
| `legacy-disease-subset-v6` | 病気・悪化・継続効果・解除を加えた旧版の部分実装 | v6構成で138種、258特徴、`gd_*`。harmsは無効 |
| `legacy-weapon-reactions-subset-v5` | 無属性武器の反射・跳ね返しを加えた旧版の部分実装 | v5構成で130種、236特徴、`gr_*` |
| `legacy-elements-miracles-chance-subset-v4` | 確率攻撃を加えた旧版の部分実装 | 128種、210特徴、`gc_*` |
| `legacy-elements-miracles-subset-v3` | 基本4属性の攻撃・防御を加えた旧版の部分実装 | 中立60＋奇跡2＋属性54の116種、186特徴、`ge_*` |
| `legacy-neutral-miracles-subset-v2` | 保持奇跡とMP消費を加えた旧版の部分実装 | 60種＋単純奇跡2種、164特徴、`gm_*` |
| `legacy-neutral-subset-v1` | 旧版の処理順を反映した1対1の部分実装 | 出典付き60種、手札容量16、意味的Action、128特徴、`gl_*` |
| `synthetic-neutral-duel-v1` | 最初に作った独自ルールの比較用環境 | 独自10種、9スロット、521行動、73特徴、`gf_*` |

legacy各ルールは旧版の重みを各profileの対象プール内で正規化して抽選します。公式の配布分布とは扱いません。
ここで学習した方策が本家でも強いとは主張できません。
legacy対戦Envは合法手のランダム実行・ベンチマークができます。学習器と学習済みモデルは同梱していません。

旧版系のC++コアは共通1368特徴・Snapshot version37です。既存ABIの出力幅は変更せず、
`gl_*` / `gm_*` / `ge_*` / `gc_*` / `gr_*` / `gd_*` / `gh_*` / `gs_*` / `go_*` / `gb_*` / `ga_*` / `gn_*` / `gk_*` / `gx_*` / `gy_*` / `gz_*` / `gu_*` / `gv_*` / `gw_*` / `gp_*` / `gq_*` / `gt_*` / `gi_*` / `gj_*` / `gg_*` / `g26_*` / `g27_*` / `g28_*` / `g29_*` / `g30_*` / `g31_*` / `g32_*` / `g33_*` / `g34_*` / `g35_*` / `g36_*` は128 / 164 / 186 / 210 / 236 / 258 / 283 / 301 / 334 / 356 / 375 / 393 / 515 / 661 / 679 / 696 / 713 / 732 / 750 / 903 / 924 / 945 / 993 / 1017 / 1043 / 1062 / 1085 / 1110 / 1135 / 1175 / 1221 / 1251 / 1278 / 1300 / 1317 / 1345特徴を維持します。
各profileで無効な拡張の末尾は0になり、旧ABIは対応する先頭部分だけを返します。
Snapshotは同じビルド・Config内のfixture用であり、旧形式を自動移行する永続checkpointではありません。

## ビルド・実行

macOS/Linux、CMake 3.20以上、C++20コンパイラーが必要です。パッケージのダウンロードは不要です。

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
ctest --test-dir build --output-on-failure
python3 python/official_data.py
python3 python/official_state.py
python3 python/official_match.py
./build/godfield_official_bench 1000 2 42 1
./build/godfield_official_bench 1000 9 42 1
./build/godfield_legacy_bench 100000 4 42 dying-weapon
python3 python/dying_weapon_env.py
./build/godfield_legacy_bench 100000 4 42 item-removal
python3 python/item_removal_env.py
./build/godfield_legacy_bench 100000 4 42 revival
python3 python/revival_env.py
./build/godfield_legacy_bench 100000 4 42 exchange
python3 python/exchange_env.py
./build/godfield_legacy_bench 100000 4 42 forgetting
python3 python/forgetting_env.py
./build/godfield_legacy_bench 100000 4 42 magic-free
python3 python/magic_free_env.py
./build/godfield_legacy_bench 100000 4 42 wide-miracles
python3 python/wide_miracles_env.py
./build/godfield_legacy_bench 100000 4 42 doubling-miracles
python3 python/doubling_miracles_env.py
./build/godfield_legacy_bench 100000 4 42 additive-miracles
python3 python/additive_miracles_env.py
./build/godfield_legacy_bench 100000 4 42 magic-block
python3 python/magic_block_env.py
./build/godfield_legacy_bench 100000 4 42 mana-weapon
python3 python/mana_weapon_env.py
./build/godfield_legacy_bench 100000 4 42 any-reflection
python3 python/any_reflection_env.py
./build/godfield_legacy_bench 100000 4 42 magic-reflection
python3 python/magic_reflection_env.py
./build/godfield_legacy_bench 100000 4 42 money-counter
python3 python/money_counter_env.py
./build/godfield_legacy_bench 100000 4 42 counter-extensions
python3 python/counter_extensions_env.py
./build/godfield_legacy_bench 100000 4 42 counter
python3 python/counter_env.py
./build/godfield_legacy_bench 100000 4 42 attribute-removal
python3 python/attribute_removal_env.py
./build/godfield_legacy_bench 100000 4 42 attribute-change
python3 python/attribute_change_env.py
./build/godfield_legacy_bench 100000 4 42 defensive-miracles
python3 python/defensive_miracles_env.py
./build/godfield_legacy_bench 100000 4 42 offensive-miracles
python3 python/offensive_miracles_env.py
./build/godfield_legacy_bench 100000 4 42 celestial
python3 python/celestial_env.py
./build/godfield_legacy_bench 100000 4 42 harms
python3 python/harms_env.py
./build/godfield_legacy_bench 100000 4 42 disease
python3 python/disease_env.py
./build/godfield_legacy_bench 100000 4 42 reflection
python3 python/reflection_env.py
./build/godfield_legacy_bench 100000 4 42 chance
python3 python/chance_env.py
./build/godfield_legacy_bench 100000 4 42 elements
python3 python/elements_env.py
./build/godfield_legacy_bench 100000 4 42 miracles
python3 python/miracles_env.py
./build/godfield_legacy_bench 100000 1 42
./build/godfield_legacy_bench 100000 4 42
python3 python/legacy_env.py
./build/godfield_bench 100000 1 42
./build/godfield_bench 100000 4 42
python3 python/godfield_env.py
```

旧版ベンチマーク引数は `試合数 スレッド数 seed [profile]`。profileは省略時neutralで、
miracles / elements / chance / reflection / disease / harms / celestial / offensive-miracles /
defensive-miracles / attribute-change / attribute-removal / counter / counter-extensions / money-counter / magic-reflection / any-reflection / mana-weapon / magic-block / additive-miracles / doubling-miracles / wide-miracles / magic-free / forgetting / exchange / revival / item-removal / hp-absorption / self-damage / pestle-mortar / selling / buying / hp-gamble / magic-bounce / sundry-boost / status-miracles / dying-weaponも選べます。
初期のsyntheticベンチマークは従来どおり3引数です。
同じ試合数・seedなら、スレッド数を変えても結果・checksumは一致します。
各ワーカーが独立した環境を持ち、ホットパスにロックはありません。
速度には方策の行動サンプリング・合法性検証・状態更新・終局後のresetを含みます。
観測の出力、Python通信、モデル推論は含みません。新旧のベンチマークはルールと方策が異なります。

メモリ・未定義動作の検証:

```sh
cmake -S . -B build-sanitize -DCMAKE_BUILD_TYPE=RelWithDebInfo -DGF_SANITIZE=ON
cmake --build build-sanitize -j 4
ctest --test-dir build-sanitize --output-on-failure
```

## 現行公式の台帳・計算・スナップショットAPI

[official_catalog.hpp](include/godfield/official_catalog.hpp)の296件は公式配信JSONの全モデルを保持します。
IDはその配列位置+1で、legacy IDとは別です。台帳のgiftRate・守護神行動率・悪魔出現率を混ぜて配布しません。
旧231件との対応はmetadata候補表であり、名称・画像を根拠にした全件の同一性確定はまだしていません。
[official_rules.hpp](include/godfield/official_rules.hpp)のpure functionsは、表示上の合成やvisible選択の追加可否を返します。
夢では見かけモデルを渡す契約で、隠れた真の神器によるserver合法性判定とは区別します。

[official_c_api.h](include/godfield/official_c_api.h)の `gfo_*` ABI1と
[Python `OfficialData`](python/official_data.py)は、read-only台帳と順序付き合成previewを提供します。
この台帳APIは状態を更新せず、合成計算を返します。
合成が計算できる列でも、それだけで合法な対戦行動とは認めません。

[official_state.hpp](include/godfield/official_state.hpp)では、最大9人・Team ID0〜4、
カードの固有token・真のmodel・fake model・usedを別々に保持します。
追加の [official_state_c_api.h](include/godfield/official_state_c_api.h) の `gfo_state_*` と
[Python `OfficialState`](python/official_state.py)は、
入力フレームを読み取り、状態欄・自己手札・使用済み奇跡のviewを作ります。
先頭の攻撃表示は独立した `active_attack()` で読み取れます（v48節）。受信後のpane/GF更新は別型 `ActiveDisplay` で扱います（v50節）。
C APIは型付き配列を検査して所有コピーを作り、PythonはdictまたはJSON文字列をimportします。
Pythonの `raw_snapshot` は未知フィールド・attacks・events等も含めてdeepcopyした元JSONを返し、
viewを取得しても元の入力を変更しません。rawには非公開値が含まれ得るため、方策の観測にはviewを使います。
入力の型・数値域は [state-schema.json](data/official-v1/state-schema.json) に記録しています。

```python
from official_state import OfficialState

frame = {"players": [{"id": 1, "userId": "local", "name": "A", "mp": 10,
                      "items": [{"id": 101, "modelId": 13}]}], "gf": 1}
with OfficialState(frame) as state:
    print(state.view(user_id="local"))
    print(state.can_append_hand([], 101, player_id=1))
```

Pythonスクリプトは `PYTHONPATH=python` で実行するか、`python` ディレクトリをimportパスへ追加します。
保存済みJSONは `OfficialState.from_file(path)` でも読み込めます。
viewでは `player_id`・`user_id`・`spectator=True` のいずれか一つを明示します。
`ResponseContext` は既に公開された応答の分類・属性を渡すための値であり、raw攻撃列から自動推定しません。

viewは参加者の別名、霧で覆われる生存敵のstatus、別パネルから見られる使用済み奇跡を区別します。
自己手札のfaceは見かけのまま、死亡・終局後のtooltip参照だけ真のモデルへ切り替えます。
他人の未使用手札やその枚数、user ID、transport更新番号はviewへ出しません。
自己手札は表示モデル順・used後方へ安定ソートし、18個までのfresh表示範囲を扱います。
この18は公式サーバーの未使用手札容量を確定した数値ではありません。

instance tokenによるappend/selection-ready照会は、重複tokenを拒否し、見かけのモデル、自己MP、
祈りの可否、捨てる/売る等の最低選択枚数、本人の閃光による1枚制限を確認します。
隠れた真の神器で判定を先取りしません。actor・phase・手番・target・交換配分・server受理は未検証です。
攻撃欄やrating欄、履歴まで含む完全な画面観測でもありません。
[表示契約と未投影項目](docs/official-state-view.md)を参照してください。

この入力フレーム用ハンドルは対局Envとは異なり、step/reset、報酬、乱数抽選、ターン進行は提供しません。
旧37profileのルール・ABI・Pythonは変更せず、学習器も追加していません。

C01は公式教典とclientで光への属性防御拒否を確認し、公式kernelにも反映しました。
C02の自然悪化5%は教典の記述を確認した段階です。
C04の属性ワンドATK2/5は実際のclient合成関数でも加算されます。
これらの公開client/説明の証拠と、権威的serverの解決順・HP遷移の未検証を分けています。

過去のv2では全Release CTest129件（60.39秒）・ASan/UBSan89件（91.53秒）が通過しました。両suiteは同時実行しています。
その後、35反射連鎖とSnapshot v1拒否のnative検証を追加し、Release/SANの対象各1件も通過しました（0.33/0.65秒）。coreの追加変更はありません。
v1の検証記録は[契約文書](docs/official-execution-subset.md#v1の過去の検証と計測)へ残しています。
公式fact296件・旧37台帳hash・候補対応表、計算/選択50,270件、状態helper/分岐22,140件、
実clientのHP/MP/CP更新7,898件が一致しています。今回再取得した公式assetのhashと296モデルのfactsも一致しました。
Pythonは台帳15件・状態9件・Match9件、
C ABI合成4,878ケース、native境界/非干渉、48seedの各stepのsnapshot/restore、後段失敗のrollbackも検証しました。
[状態oracleのmetadata](data/official-v1/state-vectors.manifest.json)にあるとおり、
実関数と抽出分岐・描画stubを使った照合であり、全画面を操作したoracleではありません。
[資源oracle](data/official-v1/resource-vectors.manifest.json)もローカル算術の照合で、配布・死亡・手番のサーバー順序を証明しません。
この節の検証時点では公式対局traceは未取得で、公開clientから権威的serverの全実行器が得られたわけではありません。
対局シミュレーターとしての公式完全対応は未達です。

2026-11-04のv51では[公式サイトの実閲覧とCPU修行](docs/official-live-observations.md)を追加しました。JS・JA・ENと台帳296件は既存資料に一致し、この時点では初期資源・本人手札の2局面までの観測でした。[最新の主要残件表](docs/official-spec-gap.md)には、178の終末補充、94＋232、Dream全候補、Earth/Moon一般処理を整理しています。現行の互換制限では、Dreamを持つ受取人への237種poolの通常授与時の偽装率は設計期待値32.1%で、教典の50%に未達です。

続くv52では、[公式CPU戦の公開遷移3件](data/official-v2/live-transition-witnesses-v52.json)を取得し、通常武器による明示自己攻撃を、防御入力を待たずDEF0・1commandで解決するよう修正しました。エンジンは `official-execution-subset-v48` / Snapshot48 / view-v46、Match ABI2です。対象は先頭が通常武器・能力なし・確率なし・Wideなしのmain playに限定し、C++/C/Pythonの `self_attack_rule` で範囲を公開します。光3の空防御、自己32の9damage、吸収26にDEF7という3例を、カードから事前計算するnativeテストで照合します。未転記の手札を変えた216通りの局面でも比較部分の一致を検査し、実際の隠れた手札・補充・server乱数を再現したとは扱いません。

v53では[闇属性での自己死亡と反射剣の自己使用](data/official-v2/live-transition-witnesses-v53.json)を確認し、明示自己武器の経路を `ability=none` または `reflect_weapon` へ広げました。現行は **`official-execution-subset-v49` / Snapshot49 / view-v47**、Match ABI2です。完了した公開遷移は累計4件、反射剣の追加応答不要という経路観測は別に1件です。反射剣の補充・CPU次行動は未転記のため、その最終資源を単独の効果goldenとは扱いません。nativeは公開4件の288投影比較と、独立した72経路検査を分けて実行します。

## 公式データによる限定対局実行器

[official_match.hpp](include/godfield/official_match.hpp) の `godfield::official::Match` は、
既定236モデル/重み499（`include_fate=True` で237/500）を対象に、全7属性、通常武器・普通防御、HP/MP/CPの単純回復、
奇跡のused保持・再使用、物理反射/消去/遮断/跳ね返し、FREE5種・属性付替2種・倍化231、単純確率攻撃21種・攻撃/資源反撃181/184/187/188・HP吸収/同被害5種・閃光付与/反撃/解除7種・暗雲・除去・MP武器58/二回攻撃14・全敵化232・HP増減202・自動復活208・杵94/臼209・両替3・売却4・購入5・病気4状態/付与6種・168/198・Fog4種・死亡時攻撃112・Dream4種/178redrawと互換クラス限定mask・201/239召喚とcanonical42自動行動/離脱、2〜9人とチームを扱います。
rulesetは `official-execution-subset-v47`、scopeは `experimental:236-default-or237-opt-in;external207;external-devils;threshold-sacrifice;external-reserved-grants;not-official-server-equivalence` です。
207のunused/unmasked特例を除く対象外のモデル・互換クラス外または不正なfake・病気4種/Fog/Dream/閃光/暗雲以外の災い・不正な守護神identity・不正な混乱count・負/unsafeな終末閾値を持つ局面は、黙って通常品へ置換せず拒否します。
read-onlyな `OfficialState` が表示できる局面を、全てこの実行器へ渡せるわけではありません。

主動作から応答へ進み、body効果・死亡の後に自然終局/cutoffを優先します。全action完了時に元ownerの病気tickと再死亡/終局判定を挟み、継続なら敵守護神の自動stageを反応/FIFOまで解決してから補充・次ownerへ進みます。
通常の自己攻撃もresponseを作り、自己回復は即時、他人への単純回復は空応答または適合する反射・跳ね返し・魔法遮断を待ちます。
通常品の消費には補充を予約し、奇跡はusedとして残して空きがある場合だけ予約します。捨てるには補充を付けません。
初期9枚/HP40MP10CP20/総枠18、固定入力順の主手番、残余 `max(0,ATK−DEF)`、
配布・死亡・終局scheduleは明示したローカル実験規約です。
公式clientで照合した算術・表示と、権威的serverで未照合の順序を[契約文書](docs/official-execution-subset.md)で分けています。

プレイヤーsourceを持つ反射は元のpower/category/abilityとownerを保ち、現在のsource/targetだけを交換します。sourceなし現象の190は返却先なしとして取り消す別の実験規約です。
属性消去は反射後も無属性を維持し、原発奇跡の再課金・新sourceへの取得は行いません。
各応答者が使った品の補充は連鎖後へ遅延し、現行Snapshot version47の全action journalで消費・保持・MP・補充と抽選/反撃来歴を検証します。旧version1〜46は拒否します。
普通防具→反射は現行clientの追加条件で不受理で、反射前に許すのは連続した属性消去だけです。
自己反射にもresponseを作るのは明示的な実験規約です。公開viewに未来RNG・消費token来歴は出しません。

現行設定は `external_grants` / `include_fate` / `local_guardian_payloads`（全て既定false）です。phase4の補充と正の混乱countを持つmainは専用chance driverが必要で、既存 `sample_action` / `random_rollout` は自動接続しません。207も専用Fate入口を使い、通常samplerが207使用を生成するわけではありません。新しい `chance_rollout` / `rollout_step` では専用処理をつなげられます（v49節）。現在の入力と制限は[限定実行器の契約](docs/official-execution-subset.md)・末尾の版別追記を参照してください。

### 実装経緯と過去の規約

以下は各版で追加した処理の経緯です。版名付きの件数・拒否条件・rule文字列は当時の記録で、現行の自己BUY・満杯SELL・Fate等の利用可否とは区別してください。

v5は跳ね返し8種21/34/114/125/137/151/164/234を追加します。以前の防御者を新sourceにし、元payloadを保ちます。
対象は**実験規約**として全aliveから入力seat順で一様抽選し、自己・味方・旧sourceも含めます。候補1人でも1draw、反射は0draw。自己対象もresponseを続けます。
公開viewの `bounce_target_rule` は `experimental:uniform-all-alive-in-input-order;includes-self-and-teammates` と明示します。
234の＜乱気流＞は保持再使用5MP、直後FREEなら0MP。全選択を消費/used保持し、1対象のredirect連鎖中は死亡確定・補充を挟みません。
restoreは主動作時の乱数anchorから各redirectを再生してtargetと最終RNGを照合し、anchor/token ledgerは観測へ出しません。
公開clientはtargetをeventから受け取り、「誰かに向かう」とだけ説明するため、この抽選分布を公式serverの仕様とは呼びません。

matching遮断は現在のpayload全体を取消し、原発者のMPや既消費品を戻しません。
魔法遮断は回復奇跡も止めます。現行プールではHP235とCP236であり、MP回復雑貨は奇跡ではありません。
233の＜壁＞は無属性武器のみを止め、有色武器へはfilterを前置できます。
＜壁＞は通常は初回/再使用とも6MP、初回はused保持し、空きがあれば補充1・満杯なら0。v4では直後のFREEで費用0にもできます。
遮断後は未終局・未打切りの場合だけまとめて補充してownerを巡回します。
blockイベントが元効果をどう取り消すかはclientの演出だけで証明できず、この実行器の明示規約です。
公開JSONは `official-execution-view-v44`、Snapshot46、MatchのC ABIは2です。read-only catalog/state ABI1は維持します。`pending.reflections` は反射、`pending.bounces` は跳ね返し件数、`pending.hit_rate` は命中率、`pending.counter` は反撃payloadかを示します。181の75は反撃開始後の表示であり、初期全敵chanceとは別です。

FREEは直前のMCだけを無料化し、複数MC→FREEペアも全て免除します。MP不足のMCを含む途中prefixでは拒否せず、完成列の最終費用を検証します。
壁233→FREEは全選択を消費/used保持する契約で、旧版の「特殊後FREE未消費」は採用しません。
追加FREEはATK/攻撃属性からskipし、先頭のFREE武器46は通常ATK12、防具143/163/173は通常DEFを保ちます。
setter66/83は追加ATK2/5を加えて属性上書き、Aura231は累積ATK×2と無属性合成。順序を保持します。
46先頭が後続MCを自動無料化することはなく、204/231の単独主動作や物理反射後のFREEも不可です。
この表示計算・選択条件の一次根拠と、補充・解決順の実験規約を[契約文書](docs/official-execution-subset.md)で分けています。

v6の確率攻撃playはtarget無しのみ受理し、launch時の生存敵を入力seat順で処理します。自己・同非Soloチームを除き、各対象がdeadなら乱数無しでskip、生存者へ有界100抽選の `roll < hit_rate` を1回適用します。missは応答無し、hitだけresponseを作り、反射/跳ね返しで再抽選しません。
原発の消費/MC費用/used保持は1回。確率MC→FREEは可能、確率武器への追加boost/setter/Auraは不可、108は通常木DEF6も保ちます。
自然終局→decision cutoffは受理コマンド末尾で優先判定し、自動miss/skipはdecisionを増やしません。launchは最初hitか全missまで自動処理を進めてから判定するため、上限1でも複数rollが起き得ます。response完了後は未打切りの場合だけ次対象を抽選し、元owner死亡なら初期chance対象の残り取消、既予約の生存counterは継続します。補充は全action完了後だけです。
この全敵展開・命中抽選・途中死亡順はserver未検証で、top-level `chance_rule='experimental:targetless-all-alive-enemies-in-input-order;darkcloud-forces-without-draw;roll-before-response;redirects-no-reroll;skip-dead;cancel-on-origin-death'` と明示します。
v7は全pending playを特権 `ActionBatch` (`.batch`) へ統一し、launch前状態とresponse journalから内部executorで完全再生してrestoreを検査します。将来roster/cursor・anchor/journal/RNG・counter FIFOとnative集計は公開viewへ出しません。

v7は生存した防御者が受けた実HP損失を基準に、選択した181/184を順に各1反撃へ予約します。181は火・実損失1倍・75%、184は土・2倍・必中。0被害や死亡時は生成しません。categoryはarmorのままで、通常属性防御/filterと190万能反射だけが適合します。
FIFOは次の初期chance対象より先に処理し、childcounterは末尾へ追加。開始時source/target死なら無draw skip、181は旧source一人への命中を1回抽選し、redirect後再抽選無しです。実HP基準で生成powerは198以下となり、旧raw×2の指数的反撃を移しません。
これらのtrigger/量/順序/death skipはserver未検証で、top-level `counter_rule='experimental:actual-hp-loss;survivors-only;selection-order-fifo-before-next-target;skip-dead-source-or-target;single-target;darkcloud-forces-without-draw;roll-before-response;redirects-no-reroll'` と明示します。

v8はMP反撃187とCP反撃188を加えます。187は実HP損失×2を生成者selftargetへ向けるpower0の明示responseとし、passで現在targetのMPを99capで回復。188は実HP損失額を旧incoming.sourceへ向け、現在targetへCP全額請求→現在counter.sourceへ全名目額を99capで加算します。CP不足はMP→HPへ波及し、自己aliasでも逐次処理、CP加算でHP0は復活しません。
いずれもarmor nonattackで190万能反射だけが適合し、資源debtのHP損失から新counterは作りません。guardian欄neptune/venusはカードmetadataであり、playerへ守護神を召喚しません。
対象・自己応答・生成量は実験規約で、top-level `resource_counter_rule='experimental:mp-self-target-with-response;cp-target-incoming-source;debit-current-target-then-full-credit-current-source;resource-debt-no-new-counter'` と明示します。CP primitiveの順序/不足連鎖には公開clientの根拠があります。
MP回復で旧paid234上限19回/人は使えないため、response上限は `2*n*hand_limit + floor(99*n*(hand_limit+1)/5) + targets.size()`（最大9人/18枠/targets9で3,718）へ更新し、同executorの全replayで会計します。

v9はHP吸収27/45/98/216と同被害53を追加します。category内訳はweapons93/armor72/sundries9/miracles20。正nonDark残余rawを現在sourceへ先にheal（吸収）またはdamage（53）し、続いてtargetへrawdamage、自己aliasでも逐次HP0〜99clipします。死flagは全body後、counter基準はtarget段階の実HP減少だけで、source側変化からcounterは作らず派生counterへ能力を継承しません。
98木75%は既存targetless chance、216光ATK10/MP10は保持/FREEを扱います。現scopeで合法なDark吸収/53は到達せず、その例を実行可能として扱いません。HP副作用の量・順序は参照Flashを参考にした実験規約で、現行clientのHP算術と確定度を分けます。
公開 `damage_effect_rule='experimental:positive-nondark-remainder;current-source-side-effect-before-target-damage;counter-basis-target-damage-only'` に明示し、新RNGやMP予算は追加しません。v9は状態を含めず、閃光はv10で追加します。

v10は閃光を持つ応答者の選択を1枚に制限します。主手番の奇跡+FREEは許可し、解除で複数枚の応答に戻ります。付与・致死時の状態・持続条件は実験規約です。[契約](docs/official-execution-subset.md#v10の閃光と解除)に一次根拠と未確認部分を記載しています。

v11は暗雲41/186/223を加えます（95武器/74防具/11雑貨/24奇跡）。41は正実HP被害で付与、186はarmor/闇/power0の純状態counter、223は闇の非攻撃奇跡MP5で、状態付与を闇即死へ読み替えません。
percent初期対象と181counterの活性化時、暗雲なら無drawでforced hitにし、元hit_rateを保ち `pending.forced_hit=true` とします。redirect後は再判定・再抽選しません。`chance_hits/counter_hits` は実draw成功のみ、forcedは別集計、miss=rolls−hitsを維持します。forcedは無drawの命中活性化数で、被害解決数ではありません。launch直後cutoffなら件数だけ残り未解決被害はありません。累計forcedや将来journal/RNGは公開viewへ出しません。
閃光→暗雲のcanonical順・dedupで同居を許し、mild199/237は閃光だけ、all200/238は両方を解除します。暗雲はall解除まで持続しtick無し。公開 `darkcloud_rule='experimental:force-percent-hit-without-draw-at-target-activation;redirects-no-recheck;idempotent-until-all-cure;no-tick'` とし、chance/counter_ruleにもdarkcloud-forces-without-drawを加えます。
教典は％攻撃の必中を説明し、clientは受信hitの演出を分けますが、無draw・活性化時点・持続/解除順はserver未確認の実験規約です。Snapshot11のanchorにもforced累計を保持し、全execute replayでRNG非消費まで照合します。

v12は夜空のホウキ205と女神の石けん206を加えます（95武器/74防具/13雑貨/24奇跡）。205はused除外の現在手札（未使用奇跡を含む）から最大3、206はused奇跡コピーから最大2を除去します。台帳abilityValueは0ですが、pending.amountの3/2は現行説明から派生した上限で、実除去数ではありません。
毎回inventory順にeligibleを組み直して逐次uniform without replacement/stable erase、emptyは除去draw0、soleでも1bounded drawとする実験規約です。自己は使用品消費後に即時処理、他者はnonattack sundryとして190だけが適合。除去gift0でも、使用/190の既予約giftは残るため、emptyがコマンド全体のRNG0を意味しません。
使用済み223を失っても既に付与された暗雲は解除せず、過去の費用/使用を巻き戻しません。`removal_rule='experimental:unused-hand-max3;used-miracles-max2;uniform-without-replacement-in-inventory-order;bounded-draw-per-removal;empty-no-draw;no-removal-gifts'` と明示します。対象数とused除外以外の抽選・順序はserver未確認です。
除去本体はatomicで中間snapshot無し。pending journalは除去前の消費/反射/RNG/payloadを検査し、完了後の任意rawstateの公式到達履歴を証明しません。v12〜18で未投影だった公開除去品は、v19のlast_removalで直近の解決結果を返します。token/slot・候補母数・RNG・非除去手札は公開しません。

[official_match_c_api.h](include/godfield/official_match_c_api.h) の `gfo_match_*` ABI2と
[Python `Match`](python/official_match.py) はreset、legal、step、status、observe、
外部policy RNGによるsample_actionを提供します。rewardは入力プレイヤー列に固定し、勝者+1/他者−1/draw0。

v13はMP武器58と二回攻撃14を追加し、208モデル/重み411（97武器/74防具/13雑貨/24奇跡）にします。58は完成列のMC費用を引いた残MP×2を基底に、追加boost/Auraを順適用し、launch時に元MPを一度だけ全消費します。MP0でも0powerの武器としてresponseを作り、redirectや後続MP回復で威力を再計算・再課金しません。現行clientのpreview/useAttackItemsに計算根拠があり、旧MAGICALの各有料MC後resetは流用しません。
14単体は同じ指定targetへ二つのfresh payloadを順に作り、消費/費用は一度だけ、counter FIFOを次hitより先に処理します。filter/redirectは各hitだけへ作用し、dead target skip・origin死亡で残り取消・自然終局/cutoff優先・全action後補充とします。二回という説明以外のscheduleはserver未確認の実験規約であり、旧Flashにも同対象2hitの根拠はありません。6damage一括へ置換しません。
公開 `mana_weapon_rule='experimental:base-2x-mp-after-composed-miracle-cost;ordered-boosts;consume-all-mp-once-at-launch;redirects-no-repayment'` と `repeat_attack_rule='experimental:two-fresh-payloads-to-same-target;single-consumption-and-payment;counter-fifo-before-next-hit;dead-target-skip;origin-death-cancels-rest;gifts-after-whole-action'` に規約を明示します。Snapshot13は旧1〜12を拒否し、view-v11/C ABI1。新RNGやMP増加源は無く、既存journal予算を維持します。

v14は追加専用奇跡232＜蜃気楼＞（MP5/NE/ATK0）を加え、209モデル/重み412（97/74/13/25）にします。完成weapon列に含む場合、launch時の生存敵を入力seat順に固定して各targetへ展開します。target無しと任意の既知alive target指定を受理し、指定は表示用anchorとしてroster/順を変えない規約です。samplerはtarget無しを選びます。公開UIは武器で選んだtargetを232追加後も保持するため、target指定禁止を公式規則とはしません。
14+232は全rosterを二巡（A,B,A,B）し、毎回fresh payload、消費/MPは一度だけ。58は完成列MC費用後のlaunch MP基底を全payloadへ固定します。複数232は拡大について冪等ですが、別copyごとの費用/保持/消費は残り、直後FREEだけ免除します。旧版の100%化を使わず、chance roll/暗雲forcedを増やしません。counter FIFO先・dead skip/origin死亡取消・全action後giftを維持し、最大16 target entries、response予算最大3,725へ拡張します。
公開 `wide_attack_rule='experimental:initial-alive-enemies-in-input-order;optional-alive-target-does-not-alter-roster;repeat-by-full-roster-pass;single-payment;counter-fifo-before-next-target;dead-target-skip;origin-death-cancels-rest;gifts-after-whole-action'` と `repeat_attack_rule='experimental:two-fresh-passes-over-initial-targets;single-consumption-and-payment;counter-fifo-before-next-hit;dead-target-skip;origin-death-cancels-rest;gifts-after-whole-action'` に明示します。Snapshot14は旧1〜13拒否/view-v12/C ABI1。全敵の説明以外のtarget受理・順序・2pass・無draw等はserver未検証です。

v15はドキドキ涙202（雑貨/NE/ATK0/値10/価格1/重み2）を加え、210モデル/重み414（97/74/14/25）にします。launchでbounded2を一度引き、0なら+10/1なら−10を固定して、他者への応答前から `pending.amount` へ公開します。selfは即時、他者はpower0のnonattack応答で190だけを許し、負でも普通DEF/filter/counterを使いません。反射は符号維持・再抽選無し、最終targetへ一度だけHP0〜99clip、全action後giftとします。
現行公開説明はHP+10または10damageまでで、50/50・符号の抽選/前公開・negativeの防御不可・死亡/補充順はserver未確認です。旧Flash237のlaunch抽選/COMMAND.hp公開は参考資料であり、現行attack.atk等に動的値が届く可能性も排除しません。公開 `random_hp_rule='experimental:launch-once;uniform-plus10-or-minus10;zero-heals-one-damages;fixed-through-reflections;nonattack-no-defense-or-counters;hp-clips-0-99;gifts-after-action'` に実験規約を明示し、Snapshot15旧1〜14拒否/view-v13/C ABI1とします。未来RNGやreplay anchorは観測へ出しません。

v16は太陽のお守り208（雑貨/復活10/価格10/重み1）を加え211モデル/重み415（97/74/15/25）にします。全body後、入力seat順にHP0かつnot.deadを走査し、その時点でnot.deadが2人以上ならinventory最初の208を1copy消費してHP+10、補充1を予約します。予約は全action後だけで、terminal/cutoffなら取消し。復活後の生存者はcounterを生成できますが、量の基準は復活前の被害段階実HP損失です。手動use/response/discardは不可。武器なしなら満杯の祈りも受理し、gift0/環境RNG0で通常の手番・cutoffへ進みます。
現行clientのreviveは指定instanceを通常消費し補充用空枠を作りますが、実gift数や発火/走査順を証明しません。1gift・not.dead人数gate・最初copy・死亡/反撃順・満杯祈りの進行は実験規約で、旧244のgift0/手動discard可は流用しません。公開 `revival_rule='experimental:post-body-input-order;hp0-and-not-dead;two-not-dead-required;first-copy-once-per-boundary;hp-plus10;one-deferred-gift;counter-basis-pre-revival-loss;passive-and-undiscardable;full-hand-prayer-no-draw'`、Snapshot16旧1〜15拒否/view-v14/C ABI1。累計revivals/token/anchorは非公開です。

v17はあぶないキネ94（光ATK30）とあぶないウス209（99damage）を加え213モデル/重み417（98/74/16/25）にします。94はtarget無し/任意の既知aliveヒントを受理して指定を無視し、launch一度で入力順最初のalive臼所持者/最初copyを優先。臼無しなら自己/味方を含む全aliveから有界一様抽選1回です。臼ありは基底99→通常のboost/Aura/属性/FREE合成、臼一枚を自動消費してgift1予約、0DEFで応答無しの即時bodyへ進みます。臼無しは基底30の通常response（自己もresponse）。反射後に臼を再選択しません。
公開clientは94+Wide232も許しますが、優先関係が未確定なのでruntimeはこの組合せだけ明示unsupported（-5）とします。他の対応boost/MC/FREEは許可し、旧版の単独限定・999・臼非消費を流用しません。209は手動use/response/discard不可、205による強制除去は対象です。
公開 `danger_rule='experimental:launch-once;first-alive-mortar-in-input-order;otherwise-uniform-all-alive;optional-alive-target-ignored;base99-before-ordered-boosts;consume-one-mortar-with-deferred-gift;attracted-body-no-response;redirects-no-reselection;wide-combination-unsupported'`、Snapshot17旧1〜16拒否/view-v15/C ABI1。`last_danger` は実行済launchのorigin/target/models/臼有無/power/elementだけを保持し、次main受理で消去、response中は保持します。token/RNG/未選択holderは出しません。対象分布・99の合成・自動消費/補充・応答省略等はserver未確認の実験規約です。

v19は除去結果の公開を追加し、台帳214モデル/重み437と効果遷移はv18から維持します。Snapshot19（旧1〜18拒否）/view-v17、Match ABI2は据置です。`last_removal` はnullまたは `{origin,source,target,model,removed_models}`。modelは205/206、整数ID配列は実除去の抽選順とコピー重複を保ち、候補0でも解決済みなら空[]を返します。originは原発者、source/targetは現在の解決座標です。
次main受理でclear、失敗時は保持、reset/loadではclear。response処理自体はclear契機ではありません。v19〜24では単一bodyのaction完了eventなのでresponse phaseと同居しませんでしたが、v25以降の後続death workは別に扱います。解決済みcutoffは保持し、未解決cutoffはnullです。token/slot/RNG・元候補数・非除去手札は公開しません。公開 `removal_event_rule='experimental:resolved-body-only;public-removed-models-in-draw-order;duplicates-and-empty-preserved;clear-on-next-accepted-main;no-tokens-or-rng'` に寿命を含む実験規約を明示し、全履歴を投影したとは主張しません。受信clientの他者対象表示は根拠がありますが、全参加者/観戦者へのserver配信や空結果eventの必ずの発行は未確認です。完了snapshotは構造整合だけを検査し、歴史的到達証明ではありません。

v18は両替3（trade/価格5/重み20）を加え214モデル/重み437（trade1/98武器/74防具/16雑貨/25奇跡）にします。`Command(actor, kind="exchange", items=(token,), allocation=(hp,mp,cp))` を主手番で使い、targetは無し、各0〜99・配分前と総和一致、同値も許可します。単品消費/gift1予約・MPfee0（価格5は費用ではない）→3資源を直接代入→既存208救済/死亡→自然終局/cutoff優先→gift/次手番です。HP0も合法で、復活+10後には総和が増え得ます。配分抽選はpolicy RNGだけ、環境RNGは後のgiftだけです。
公開 `exchange_rule='experimental:single-owned-exchange;targetless-self;three-resources-0-99;sum-conserved-before-revival;zero-mp-fee;no-response;one-deferred-gift;death-and-revival-before-gift'`、Snapshot18旧1〜17拒否/view-v16。typed配分追加により `gfo_match_*` はABI2、command168→184byte（末尾hp/mp/cp/has_allocation/reserved_tail[3]）で、旧168byteのまま呼べません。構造体API呼出前にversion/sizeを検査し、ABI1 Match clientは移行してください。config24/status248、catalog/state ABI1・旧37は維持。sacrificeは終末依存で未実装、94+Wideのgapも維持します。

v20は売却4を加え215モデル/重み457（trade2/98武器/74防具/16雑貨/25奇跡）、Snapshot20旧1〜19拒否/view-v18、Match ABI2の構造体サイズは据置です。`Command(actor, kind="sell", items=(sell_token, commodity_token), target=target)` で本人の4と別tokenの未使用商品を使い、自己/味方を含む生存targetを明示します。usedMCは売れませんが武器・208/209・別4・未使用MCは商品にでき、MP0/価格5は操作料ではありません。
4と商品を消費して操作分gift1だけ予約し、商品MCの保持化や商品分の補充はしません。他者は190のみのnonattack response、自己は即時。現在targetへのCP請求（不足MP→HP）→現在sourceへの名目全額CP加算cap99→新tokenで未使用商品を直接配送→208復活/死亡→自然終局/cutoff優先→gift/次手番とします。自己aliasも逐次、反射で品/priceを変えず、配送は環境RNGを使いません。
配送先の実物満杯は明示unsupported（-5）/sampler除外です。自己は二品消費後の空きを数え、旧Flashの配送失敗無視を移植しません。現行clientのoverflowItemから実物eviction規約を確定できないため、完全SELL対応とは主張しません。`last_sale={origin,source,target,model,price}` は解決後だけ公開し、次main受理/reset/loadでclear・失敗保持・解決済みcutoff保持/未解決null。last_danger/last_removalとは排他で、token/他手札/RNGは公開しません。

v20〜25時点の公開 `sell_rule` は `experimental:one-unused-commodity;explicit-alive-target;zero-mp-fee;one-operation-gift;foreign-response-and-reflection;buyer-debit-before-current-source-credit;fresh-token-delivery-before-death;full-recipient-unsupported`。商品の232/Aura231/MP武器58/杵94/復活208等のabilityを売却payloadの構成時に実行・継承せず、単一対象・power0・属性無し・商品priceだけを用いる。配送後208の独立した救済hookは別に扱う。商品fakeも既存unsupportedのまま。構造体は同じABI2でも、clientは新opcode5の認識とruleset20のloader照合が必要である。

直接配送後の予約giftは `min(old_gift, hand_limit−new_items_count)` へ防御的に正規化する。実物を追い出す処理ではない。現scopeの正常経路では原発二品消費/gift1、190消費/gift1が作る空きにより通常は超過しない。実物満杯のunsupportedをこのclampで回避しない。

ベンチの `sell_plays` は操作回数、`sell_resolved` は自己即時/他者空応答で最終bodyが完了した数（190反射途中は除く）、`sell_nominal_cp` は名目price総額で実CP支払/HP損失/加算clip量ではない。`sold_miracles` は配送商品が奇跡だった件数でcast/学習ではない。`sold_miracles<=sell_resolved<=sell_plays` を確認する。v20の `physical_used` はSELL一回につき操作4だけ+1、商品は移送として除外し、通常play/defend/exchangeの選択使用や奇跡first/recastへ混ぜない。

v21は購入5を加え216モデル/重み477（trade3/98武器/74防具/16雑貨/25奇跡）、Snapshot21旧1〜20拒否/view-v19、Match ABI2同サイズです。kind buy6/accept7/decline8、phase purchase3を追加しfinished2は維持します。`Command(actor, kind="buy", items=(buy_token,), target=seller)` は本人以外のalive targetに限り、自己BUYは明示unsupported（-5）。単品5/MP0/gift1、play5・MC/FREE・allocationは不可です。
190反射後の現在sourceがbuyer/targetがseller。最終bodyでused奇跡を除く手札一個を抽選し、元slot/tokenを特権escrowへ一時預かりします。空はdraw無し/見積無し、1候補もdraw1回。CP不足なら元の品を戻して金銭移動無しで終了。購入可ならbuyer actorのpurchase phaseと公開 `{origin,buyer,seller,model,price}` を作り、buyerが空items/target無しのaccept/declineを選びます。
YESはbuyer CP減→seller名目全額CP加算cap99→buyerに新token unused商品を配送→既存復活/死亡/gift等。NOは元slot/tokenへ返し、未完了cutoffでも返却してgift取消、配送/返却自体のRNGは0です。満杯buyerへのYESは-5/NOは可。商品MCはcast/学習でなく、商品giftも無し。`last_purchase={origin,buyer,seller,model,price,bought,reason}` のreasonはbought/declined/empty/insufficient-cp/cutoff、emptyはmodel0/price0、商品未選定cutoffはnull。見積中は完了event無し、次main受理/reset/loadでclear・失敗保持・他完了eventsと排他です。
usedMC除外・YES/NOとbuy/setBought/canNotBuy表示は公開clientの根拠ですが、抽選/不足CP終了・自己対象拒否・escrow同一性・cutoff/返却scheduleはserver未確認の実験規約です。原slot/token・RNG・候補数・非選定手札は公開せず、完全BUY対応とは呼びません。

v21〜25時点のv21当時の公開 `buy_rule` は `experimental:foreign-only;one-operation-gift;uniform-unused-commodity;empty-no-draw;cp-only-affordability;buyer-accept-or-decline;escrow-return-on-decline-or-cutoff;fresh-token-delivery;full-accept-unsupported`。`bought` はreasonがboughtかどうかから派生するbooleanで、独立した履歴flagではない。特権 `PurchaseOffer` はoriginal item（token/model/used=false）と原slotを保持するが、公開purchaseにはこれらを追加しない。承認/拒否も新しい意思決定として既存cutoff規約に従う。

購入決定をjournalへ一件加えるため、BUY actionだけ既存の保守的response/journal boundへ+1する。非BUYの最大3,725を変えず、全体の保守的な上界へさらに+1を見込めば3,726となる。実際のBUYは単一target・190の物理反射だけなので、これより小さい。購入承認でも商品配送後のgift予約を `min(gift, hand_limit−new_items_count)` に正規化し、実物evictionを行わない。

ベンチは `main_commands+responses+purchase_commands=decisions` へ三分離する。`buy_plays` は操作数、`buy_quotes` は公開見積phase開始数、`purchase_commands` はaccept/decline数、`bought`/`declined` は各決定結果、`buy_empty`/`buy_insufficient_cp`/`buy_cutoff` は各完了reason、`buy_nominal_cp` はboughtの名目price総和（sellerのclip後実増分ではない）。`bought+declined=purchase_commands<=buy_quotes<=buy_plays`。`physical_used` はBUY操作5だけ+1で、商品移動・購入決定・奇跡cast/学習を加えない。v20以前のmain+responses=decisionsは過去版の定義として残す。

v22は病気4状態Cold/Fever/Hell/Heavenと付与六モデル30/33/48/62/219/220を加え、222モデル/重み483（trade3/102武器/74防具/16雑貨/27奇跡）、Snapshot22旧1〜21拒否/view-v20とします。v22時点で168selfCurse/198MP+Heavenは未対応だった。Match ABI2・opcode0〜8・phase3を変更しません。
病気無しならincoming、既存ならmax(incoming,current+1)、Heaven再付与はHP0/Heaven維持。非病気Flash/DarkCloudは独立でHeavenfallを起こさず、mildはCold/Fever/Flash、allは対応六curseを解除します。再付与fallをcounter基準へ加えず、regular target HP被害だけを使い208救済後のsurvivorを判定します。
元ownerの全action完了後advance冒頭だけ、生存/病気ありなら固定5%（bounded100<5）で悪化→変化後Cold−1/Fever−2/Hell−5/Heaven+5、HeavenfallはHP0でheal無し→208/死亡→自然終局→gift/次owner。body自然終局/decision cutoff成立時はtickせず、反射/連撃/全体/counter/BUY見積の途中にもtickしません。購入決定で完了しても元ownerだけが対象です。教典の5%/継続量と、このrank/抽選順/境界のserver未確認実験規約を区別し、旧worseChance増加式は採用しません。
native last_tickはbodyとtickのHP変化・蘇生を分ける特権情報で、内部record/token/RNGは公開しません。公開はdisease_ruleとboardの状態だけです。

v22/23時点の公開 `disease_rule` は `experimental:one-disease-plus-flash-and-darkcloud;reapply-max-incoming-or-next;heaven-reapplication-hp0;owner-only-after-whole-action;natural-and-cutoff-before-tick;fixed5-percent-before-hp;post-upgrade-tick;post-tick-revival-before-gifts`。v22/23のraw stateは病気最大一種→Flash→DarkCloudのenum順・重複無し、最大三curseのcanonical集合だった。v24のFog拡張は後述する。tick recordは次main受理でclearし、現在の公開boardから過去のroll/tokenを復元して返す機能は作らない。 現行clientの病気表示は新たな受信curseで旧病気一種を置き換え、非病気Flash/DarkCloudは独立に残す。旧FlashのHeaven中に非病気を付けても転落させる挙動は流用しない。

ベンチの `disease_ticks` / `disease_worsened` / `disease_falls` はtick/自然悪化/Heavenfall件数、`disease_tick_revivals` はtick由来208救済数。`disease_hp_gained/lost` は復活前のtick HP増減を集計し、救済時はrawAfter=0として208の+10を除き、Heavenfallの実HP損失も含める。累計 `revivals` はbody由来とtick由来を含む。HP増減202のbody集計からtick HPdeltaとtick救済を外し、205/206のtoken消失数から同targetのtick208消費を除いて公開除去eventと照合する。これらの内部会計のためにlast_tick/tokenを利用しても、公開JSONには出さない。

v23は168熱狂仮面（Fire普通DEF10/自己Fever/p10w1）と198天国草（NE雑貨/MP20+Heaven/p20w1）を加え、224モデル/重み485（trade3/102武器/75防具/17雑貨/27奇跡）、Snapshot23旧1〜22拒否/view-v21とします。Match ABI2の幅・commands0〜8・phase3は据置、guardian/MP費用はありません。
168はmain不可、属性適合時の普通防御に複数copy可、Flashは一枚、filter前置可/特殊反応併用不可。全response消費後に選択順で自己Feverを各copy一回→incoming body→208/死亡とし、完全防御でも自己付与します。prefixHeavenfallでHP0になっても本体前に蘇生せず、HP0&&!deadを待機snapshotに許しません。counterはbodyのtarget HP実損失だけで、prefix落下を加算しません。自己吸収aliasがHP0→回復→被害を起こす場合の正のbody損失は別に数えます。
198はmain play専用（response品には不可）でMP20clip99→Heaven付与→救済/死亡、自己即時/他人190のみ。能力自体にRNG/holding/FREEは無く、後のowner tick/giftは別。discard/売買商品では両品のabilityは発動しません。v22の病気/5%/owner境界・自然終局/cutoff・救済順を維持します。
公開 `disease_item_rule='experimental:response-self-curse-in-selection-order-after-consumption;prefix-before-body;death-and-revival-after-whole-body;counter-basis-body-target-loss-only;mp-boost-before-disease;no-trigger-on-trade-or-discard'` に付与順を実験規約として明示します。現行selfCurseはevent.indexの演出だけで、198のtooltip/独立MP・curse受信handlerもserverの付与順/HP0/208までを証明しません。 198は全response後のbodyでMPを増やし、nonattackへの反応は190だけなので同batchの有償MP反応予算を増やさない。journal上界は非BUY3,725/BUY3,726の保守的上界を維持する。新ベンチ `fever_mask_uses` は応答で使った168コピー数、`heaven_herb_plays` は198のmain開始数で、捨てる/商品移動を含めない。`heaven_herb_resolved` は自己即時または他者空応答の最終body解決数（190反射途中は除く）、`heaven_herb_mp_gained` は当該targetのMP after-before実増分で、99cap後の値を数える。同body後のtick/giftはMPを変えないため分離でき、名目20倍値に置き換えない。resolved<=plays/実増<=20*resolvedを検査する。未解決cutoffなら開始だけ、解決済みcutoffならbody実増も計上する。

v24はFog付与72/97/182/221を加え228モデル/重み489（trade3/104武器/76防具/17雑貨/28奇跡）、Snapshot24旧1〜23拒否/view-v22にします。Match ABI2の幅/kinds0〜8/phase3は据置。病気一種+Fog+Flash+DarkCloudの最大四curse、mildCold/Fever/Fog/Flash・all全七種、非病気FogはHeavenfallや単独tickを起こしません。
Fogのmain play/buyで敵を明示したlaunchだけalive敵input順一様draw1（単独候補も1）、202なら続いて符号draw。自己/味方は維持、%/Wide/Dangerは除外し、14は同じ選択先へ二回、redirect/counter/YESNOは再選択しません。分布・順序・例外はserver未確認の実験規約で、privileged last_fogやRNGは公開しません。
生存Fog本人/未終局の生存敵statusだけnullにし、自己/味方/dead敵/観戦/dead viewer/終局は可視。名前/team/usedモデルは別paneの公開情報です。forced_hitは命中時initial_targetのpaneが隠れるならtrue/false両方nullとし、可視selfへの反射後も漏らしません。公開payload/quote/除去・売却eventは別経路で残しますがfanoutは未照合です。
v24/25ではFogのforeign SELLは敵/味方とも容量peek前に-5・自己可、samplerのSELLは全状態で自己に固定します。非Fogmanual foreign SELLの容量依存gapは残り、全query/全profile非干渉を主張しません。
ベンチの `fog_main_commands` は受理main前のownerがFogだった全kind、`fog_launches` はそのmainでlast_fogを生成した数、`fog_target_changed` はintended!=selectedの数。response中のrecord残存を重複計上せず、changed<=launches<=fog_main_commands<=main_commandsを検査する。2人戦の候補一人で対象不変でも、launch drawは一回消費する。view-v22のforced_hitはbool/nullの三状態として読み、nullをfalseと同一視しない。

v24/25時点の公開 `fog_rule='experimental:launch-once-for-explicit-enemy-target;uniform-alive-enemies-in-input-order;one-draw-including-singleton;self-and-allies-unchanged;chance-wide-danger-excluded;redirects-no-reselection;fog-foreign-sell-unsupported;sampler-sell-self-only'`、更新した `disease_rule='experimental:one-disease-plus-fog-flash-and-darkcloud;reapply-max-incoming-or-next;heaven-reapplication-hp0;owner-only-after-whole-action;natural-and-cutoff-before-tick;fixed5-percent-before-hp;post-upgrade-tick;post-tick-revival-before-gifts'` に実験規約を明示します。

v25は112昇天弓の通常Light ATK1/25%と死亡時Light30/75%を加え、229モデル/重み490（trade3/105武器/76防具/17雑貨/28奇跡）、Snapshot25旧1〜24拒否/view-v23とします。Match ABI2の幅/kinds0〜8/phase3は据置です。
208先行救済の後に実deadになった人物について、自然終局なら112は未消費、非終局なら在庫全copyを在庫順に直接消しcopy×alive敵input順のdeath FIFOを予約します。自動消費のgiftは0です。body後decision cutoffでは既に消費したcopyは戻さず、残work/未開始roll/response/giftを取消します。duelの最後の敵死亡から道連れを起こさないgateもserver未確認の実験規約です。
death FIFO→普通counter FIFO→初期rosterの順に処理し、新死者は末尾へ追加。dead sourceは自動攻撃可ですが手動行動は不可。DarkCloudは活性化時にdraw無し必中、Fog再選択/redirect再roll無し、deadへ返ればframe終了・bounceはalive候補です。全main kindのprestate journalとtick_processedで病気tick後の死亡にも対応し、owner tick一回/全work後giftを守ります。
新上界death_bound=9×8×18=1,296 frameをjournalへ足し、旧3,725/BUY3,726だけは流用しません。公開pending.deathとdeath_rule以外のqueue/token/stage/未来RNGは非公開です。旧Flash117の通常75%・最初copyだけ・非消費は現行112へ移しません。

ベンチの `death_bows` は自動消費した112コピー数で、通常の `physical_used` へ混ぜない。`death_created` は全予約frame数、`death_started` は75%命中または暗雲forcedでalive actorのresponseを開始したframe数、`death_rolls/hits` は実抽選/成功、`death_forced_hits` は無draw命中、`death_responses` は反射等を含む実defend回数。bows<=created<=8*bows、hits<=rolls<=created、started=hits+forced、forced<=created−rolls、responses>=startedを検査する。新しいtickだけを集計し、死後応答中に残るlast_tickを重複計上しない。自己205/206の後に同tickで消えた208/自動112を除去eventのtoken消失数へ混ぜない。公開JSONへ累計/未来FIFO/来歴stageを追加しない。通常配布で未対応のgiftモデルは残り8種で、Dreamは調査のみとする。
公開 `death_rule='experimental:revival-before-actual-death;nonterminal-boundaries-only;consume-all-bows-in-inventory-order;copy-then-alive-enemies-in-input-order;light30-75-percent;death-fifo-before-counters-and-initial-roster;dead-source-allowed;darkcloud-forces-without-draw;redirects-no-reroll;dead-redirect-target-ends-frame;no-fog-reselection-or-gifts;owner-tick-once;natural-and-cutoff-cancel-pending'` にqueue/終局の実験規約を明示します。

v26は229モデル/重み490を維持し、満杯SELL配送・BUYにも共用する配送helper・Fog foreign SELLを扱います。Snapshot26旧1〜25拒否/view-v24、Match ABI2の幅/kinds0〜8/phase3は据置です。満杯なら配送前の全existing実物（usedMC/208/209/112含む）からinventory順一様bounded draw一回で一copyを取り、新token unused商品を届けます。新商品や補充placeholderは候補外、空きがあればoverflow draw無しです。

v26当時の229モデルprofileではBUYのbuyerは操作5または反射190を消費するため常にcap−1以下で、quote中の流入/giftも無い。したがって満杯BUYはこの閉じたprofileでは到達不能で、共通配送helperの備えに留まる。検証はfullSELLで共用helperの全候補順を確認し、通常BUYでoverflow_model=nullを確認するもので、偽raw quote importでBUY fullを到達させたcoverageを主張しない。
CP支払/加算→eviction→配送→救済/死亡の順で、eviction自身の補充/返金/解除/abilityはありません。BUY拒否/未決cutoffは元slot/token返却なのでoverflow無し。last_sale/last_purchaseのoptional overflow_modelはeviction無しでnull、有りでモデルIDだけを示し、候補数・token/slot/RNGは非公開です。分布/候補/順序/gift/fanoutは現行server未確認の実験規約です。
FogのSELL enemy hintもlaunch一回再選択し、商品232/%/Dangerをmodifierとして使いません。自己/味方は維持。sampler SELLは全alive input順で容量peek無しとなり、Fog foreign拒否と先行capacity oracleを解消します。ベンチ `sell_overflows` / `buy_overflows` は各解決stepで一回だけ数え、`overflow_events=sell_overflows+buy_overflows`、sell_overflows<=sell_resolved、buy_overflows<=boughtを検査する。`overflow_event_checksum` はSELL/BUYのkind5/6を分けた公開event tupleから作りsum集計し、slot/token/元候補数は含めない。これは通常observeへ内部累計を追加するものではない。

v26時点では自己BUY・通常gift full-clamp・94+Wide等のgapを残し、Dreamは未対応でした。
元body完了の除去/売却/購入eventはv25以降、後続death応答中も保持し得ます。quoteとは同居させず次main受理clear/失敗保持を維持します。

公開 `overflow_rule` は `experimental:trade-delivery-only;full-recipient-uniform-existing-item-in-inventory-order;one-bounded-draw;all-existing-items-including-used-miracles-and-passives;cp-debit-and-credit-before-eviction;eviction-before-fresh-delivery-and-death;no-eviction-gift-or-ability;public-model-only;no-overflow-on-decline-or-cutoff-return`。

公開 `sell_rule` は `experimental:one-unused-commodity;explicit-alive-target;zero-mp-fee;one-operation-gift;foreign-response-and-reflection;buyer-debit-before-current-source-credit;fresh-token-delivery-before-death;full-recipient-uniform-existing-item-overflow`。

v21当時の公開 `buy_rule` は `experimental:foreign-only;one-operation-gift;uniform-unused-commodity;empty-no-draw;cp-only-affordability;buyer-accept-or-decline;escrow-return-on-decline-or-cutoff;fresh-token-delivery;full-recipient-uniform-existing-item-overflow`。

公開 `fog_rule` は `experimental:launch-once-for-explicit-enemy-play-buy-sell;uniform-alive-enemies-in-input-order;one-draw-including-singleton;self-and-allies-unchanged;chance-wide-danger-excluded-except-sale-commodity;redirects-no-reselection;sampler-sell-all-alive-without-capacity-peek`。

意思決定の打切りは自然終局と分け、報酬0です。不正・未対応入力で状態/RNG/出力を一部更新しません。
C/Pythonの `load_scene` は同一libraryの `OfficialState` から安定した盤面をdeepcopyして初期化します。非空attacksを持つpending server frameは受け付けず、対局checkpointのimport/restoreやeventからの手順再生は提供しません。

`python3 python/official_match.py` はローカル実行例です。C++ベンチマークは
`./build/godfield_official_bench GAMES PLAYERS SEED WORKERS`、省略時は `10000 2 42 1`。
これは限定プールとローカル方策での実行速度を測り、公式全モデル・対局trace・サーバーとの一致や学習性能を示すものではありません。

以下は過去の **v1・132モデル/重み308** のRelease計測（seed42、1 worker）です。
現行v47の性能やchecksumを示すものではありません。

| 人数 | 試合数 | 意思決定数 | draw / 打切り | 秒 | 意思決定/秒 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 2 | 100,000 | 3,918,160 | 0 / 0 | 2.48432 | 1,577,160 |
| 9 | 10,000 | 2,838,305 | 0 / 29 | 2.48806 | 1,140,770 |

合法手samplerと各試合の終了snapshot検証を含みます。1/4 workersで全決定的な集計・checksumが一致しました。
勝数・checksum・SANとの照合は[限定実行器の検証記録](docs/official-execution-subset.md#検証と計測)に記載しています。

過去のv2（139モデル/重み319、seed42）の決定的な集計:

| 人数 | 試合数 | 意思決定数 | draw / 打切り | 反射 / 消去 | checksum |
| ---: | ---: | ---: | ---: | ---: | --- |
| 2 | 100,000 | 3,949,983 | 0 / 0 | 27,346 / 24,118 | `17327277230250071888` |
| 9 | 10,000 | 2,862,579 | 0 / 22 | 24,214 / 23,344 | `13034266850202749327` |

1/4 workersで時間以外の全集計が一致し、全試合の終了snapshotをrestoreできました。
このベンチマークでの最大反射深度は3であり、実行器の上限3という意味ではありません。
競合を避けたRelease・1 workerで再計測したv2速度は、2人100,000試合が2.51205秒（1,572,410意思決定/秒）、
9人10,000試合が2.52802秒（1,132,340意思決定/秒）。上表と全決定的なfieldが一致しています。
合法手sampler・実行・各試合の終了snapshot検証を含み、合成関数単体や学習速度ではありません。
SAN/Releaseの2人・9人各1,000試合も時間以外の全集計が一致しました。
これらのv1/v2数値は148モデルのv3の計測結果ではありません。

過去のv3では最終freeze後の全Release CTest129件/19.71秒、ASan/UBSan89件/51.45秒が通過しました（各 `-j4` 同時実行）。
Python Match10ケースも通過。公式assetsの再取得hashと296モデルfactsも一致しました。

| v3人数 | 試合数 | 意思決定数 | draw / 打切り | 遮断 / 壁初回 / 壁再使用 | checksum |
| ---: | ---: | ---: | ---: | ---: | --- |
| 2 | 100,000 | 3,905,076 | 0 / 0 | 12,674 / 5,816 / 1,668 | `11131397330614631561` |
| 9 | 10,000 | 2,849,191 | 0 / 32 | 15,884 / 5,349 / 3,554 | `15567992248220262038` |

1/4 workersの時間以外の全集計が一致し、全終了snapshotをrestoreできました。
競合を避けたRelease・1 workerでは、2人100,000試合が2.41763秒（1,615,250意思決定/秒）、
9人10,000試合が2.46832秒（1,154,300意思決定/秒）で、上表の全決定的fieldを維持しました。
合法手sampler・実行・終了snapshot検証を含みます。Release/SAN各1,000試合の時間以外の全集計も一致しました。
勝数・反射/消去の集計は[契約文書のv3記録](docs/official-execution-subset.md#v3の過去の検証と集計)へ記載しています。
これらのv1〜v8結果は現行v47の236モデルの速度・checksumではありません。

v4は全Release CTest129件/19.67秒、ASan/UBSan89件/51.30秒が通過しました（各 `-j4` 同時実行）。Python Matchは11ケースです。
その後、0MPの追加Aura+FREE pairを生成する専用native局面を追加し、Release/SAN各1件も通過しました（0.26/0.57秒）。coreの追加変更はありません。
公式assetsの再取得SHA-256と296モデルfactsも一致しました。

| v4人数 | 試合数 | 意思決定数 | draw / 打切り | FREE品使用 main / response | 免除MP | checksum |
| ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 2 | 100,000 | 3,858,925 | 0 / 0 | 21,576 / 22,484 | 73,916 | `626852514698712269` |
| 9 | 10,000 | 2,776,530 | 0 / 13 | 22,596 / 19,373 | 96,103 | `11822269879695662487` |

1/4 workersで時間以外の全集計が一致し、全終了snapshotをrestoreできました。
FREE品使用数は通常の武器/防具役割での使用も含み、無料化発動回数ではありません。免除MPが実費用の割引合計です。
競合を避けたRelease・1 workerでは、2人100,000試合が1.91505秒（2,015,060意思決定/秒）、
9人10,000試合が2.23955秒（1,239,770意思決定/秒）で、上表の全決定的fieldを維持しました。
合法手sampler・実行・終了snapshot検証込みです。プールとsamplerが異なるためv3との速度比を改善率として扱いません。
Release/SANの2人・9人各1,000試合も時間以外の全集計が一致しました。
勝数・Aura/setter等は[契約文書のv4記録](docs/official-execution-subset.md#v4の過去の検証と集計)へ記載しています。
以上のv4計測も過去の証拠として保持し、229モデルのv26の計測とは分けます。
v5は全Release CTest129件/20.03秒、ASan/UBSan89件/51.49秒、focused Release3件/1.09秒・SAN2件/1.20秒が通過しました。最終全ソースfreeze後の全体suiteは各 `-j4` のRelease/SAN同時実行です。Python MatchはASTで12ケース、公式assets再取得hashと296モデルfactsも一致し、compile warningはありませんでした。

| v5人数 | 試合数 | 意思決定数 | draw / 打切り | bounce / 乱気流 初 / 再 | checksum |
| ---: | ---: | ---: | ---: | --- | --- |
| 2 | 100,000 | 3,900,109 | 0 / 0 | 36,602 / 893 / 288 | `8091597239127085284` |
| 9 | 10,000 | 2,787,125 | 0 / 12 | 31,700 / 1,880 / 639 | `15009826975196651729` |

1/4 workersで時間以外の全fieldが一致し、全終了snapshotをrestoreできました。反射+bounce合計の `max_redirect_depth` は4/5で、連鎖上限ではありません。
`self_bounce_responses` は自己を次actorとして応答が続いた18,203/5,801件で、打切りでactorが消えたbounceを含みません。
競合を避けたRelease・1 workerでは、2人100,000試合が1.9292秒（2,021,620意思決定/秒）、9人10,000試合が2.24261秒（1,242,800意思決定/秒）で、全決定的fieldを維持しました。合法手sampler・実行・終了snapshot検証込みで、学習や推論の速度ではありません。scope・policy・プールが異なるv4との速度改善率は主張しません。
Release/SAN seed42各1,000試合の時間以外の全fieldも一致しました。checksumは2人 `13433316251301511666`、9人 `10911795126884805082`。
勝数・他の集計は[契約文書のv5記録](docs/official-execution-subset.md#v5の過去の検証と集計)へ記載しています。
以上のv5計測も過去の証拠として保持し、v6の検証・計測とは分けます。
v6は全Release CTest129件/18.98秒、ASan/UBSan89件/50.42秒、focused Release3件/0.51秒・SAN2件/0.64秒が通過しました。全ソース最終freeze後のfull検証は各 `-j4` のRelease/SAN同時実行で、compile warning無し、Python Match13ケース、公式assets再取得hash/296factsも一致しました。

| v6人数 | 試合数 | 意思決定数 | draw / 打切り | chance攻撃 / roll / hit / miss | checksum |
| ---: | ---: | ---: | ---: | --- | --- |
| 2 | 100,000 | 3,795,626 | 0 / 0 | 117,318 / 117,318 / 68,036 / 49,282 | `12990827435025744992` |
| 9 | 10,000 | 2,396,687 | 0 / 0 | 47,957 / 195,003 / 113,248 / 81,755 | `12373722458217459627` |

1/4 workersの時間以外の全fieldが一致し、全終了snapshotをrestoreできました。Release/SAN seed42各1,000試合も時間以外の全集計が一致し、checksumは2人 `14650413054504497333`、9人 `15816808055195738927`。
競合を避けたRelease・1 workerでは、2人100,000試合が1.96549秒（1,931,130意思決定/秒）、9人10,000試合が1.98373秒（1,208,170意思決定/秒）で全決定的fieldを維持しました。合法手sampler・実行・終了snapshot検証込みで、学習や推論の速度ではありません。scope・policy・プールが異なるv5との速度改善率は主張しません。
反応等の内訳は[契約文書のv6記録](docs/official-execution-subset.md#v6の過去の検証と集計)へ記載しています。
以上のv6計測も過去の証拠として保持し、v7の検証・計測とは分けます。
v7は全Release CTest129件/18.41秒・ASan/UBSan89件/49.31秒（各 `-j4` 同時実行）、focused Release3件/0.67秒・SAN2件/1.31秒が通過しました。最終全ソースfreeze後のbuild/full、Python MatchはAST確認14ケース、compile warning無し、公式assets再取得hash/296facts一致です。
9人Solo・HP99・seed0で、1行動51counter生成/50開始、元owner死亡後のdead target skipと初期roster取消、全steprestore、生存creatorだけへのgiftも検証しました。固定32上限は置きません。

| v7人数 | 試合数 | 意思決定数 | draw / 打切り | counter 生成 / 開始 / 抽選 / hit / miss | checksum |
| ---: | ---: | ---: | ---: | --- | --- |
| 2 | 100,000 | 3,771,526 | 0 / 0 | 7,881 / 6,885 / 3,998 / 3,007 / 991 | `4348491756840048703` |
| 9 | 10,000 | 2,381,724 | 0 / 0 | 5,593 / 4,829 / 2,898 / 2,139 / 759 | `5379530731485268463` |

1/4 workersの時間以外の全fieldが一致し、全終了snapshotをrestoreできました。Release/SAN seed42各1,000試合も時間以外の全fieldが一致し、checksumは2人 `3714956624162246545`、9人 `13673169011916776123`。
競合を避けたRelease・1 workerでは、2人100,000試合が2.32449秒（1,622,520意思決定/秒）、9人10,000試合が2.49136秒（955,992意思決定/秒）で全決定的fieldを維持しました。合法手sampler・実行・終了snapshot検証込みで、学習や推論の速度ではありません。scope・policy・プールの異なる版同士の速度改善比は主張しません。
勝数・他集計は[契約文書のv7記録](docs/official-execution-subset.md#v7の過去の検証と集計)へ記載しています。
以上のv7計測も過去の証拠として保持し、v8の検証・計測とは分けます。
v8は全Release CTest129件/19.78秒・ASan/UBSan89件/51.38秒、focused Release3件/1.32秒・SAN2件/1.32秒が通過しました。全ソース最終freeze後のbuild/full、Python MatchはAST確認15ケース、compile warning/error0、公式assets再取得hash/296facts一致です。

| v8人数 | 試合数 | 意思決定数 | draw / 打切り | MP反撃解決 / 実MP増分 | CP反撃解決 / 名目CP | checksum |
| ---: | ---: | ---: | ---: | --- | --- | --- |
| 2 | 100,000 | 3,776,364 | 0 / 0 | 3,682 / 53,711 | 3,821 / 26,935 | `2033392899257200488` |
| 9 | 10,000 | 2,380,991 | 0 / 1 | 2,617 / 36,262 | 2,575 / 17,886 | `11415801088495861110` |

MP実増分は99cap後の値、CP名目は不足資源への波及やsourceのcap前の請求額で、実CP移転額とは異なります。
1/4 workersの時間以外の全fieldが一致し、全終了snapshotをrestoreできました。Release/SAN seed42各1,000試合も時間以外の全fieldが一致し、checksumは2人 `13623686729798944892`、9人 `4100116851726543187`。
競合を避けたRelease・1 workerでは、2人100,000試合が2.34208秒（1,612,400意思決定/秒）、9人10,000試合が2.46961秒（964,117意思決定/秒）で全決定的fieldを維持しました。合法手sampler・実行・終了snapshot検証込みで、学習や推論の速度ではありません。版間の速度改善比は主張しません。
勝数・他集計は[契約文書のv8記録](docs/official-execution-subset.md#v8の過去の検証と集計)へ記載しています。
以上のv8計測も過去の証拠として保持し、v9の検証・計測とは分けます。
v9は全Release CTest129件/20.20秒・ASan/UBSan89件/51.81秒、focused Release3件/1.13秒・SAN2件/1.33秒が通過しました。全source/bridge/test最終freeze後のbuild/full、Python MatchはAST確認16ケース、warning/error0、公式assetsのfresh取得hash/296facts一致です。

| v9人数 | 試合数 | 意思決定数 | draw / 打切り | 吸収 正解決 / raw合計 | 同被害 正解決 / raw合計 | checksum |
| ---: | ---: | ---: | ---: | --- | --- | --- |
| 2 | 100,000 | 3,734,360 | 142 / 0 | 22,464 / 180,643 | 6,922 / 81,885 | `8957456254725946378` |
| 9 | 10,000 | 2,361,580 | 15 / 0 | 19,382 / 136,285 | 4,180 / 47,877 | `12174732045177881378` |

名目HPは普通DEF後の正nonDark raw残余で、実回復量やclip後の実HP損失ではありません。1/4 workersはworker数・時間以外の全fieldが一致し、全終了snapshotをrestoreできました。
Release/SAN seed42各1,000試合も時間以外の全fieldが一致し、checksumは2人 `2715841436232270123`、9人 `13089925563403552514`。
競合を避けたRelease・1 workerでは、2人100,000試合が2.34078秒（1,595,350意思決定/秒）、9人10,000試合が2.509秒（941,242意思決定/秒）で全決定的fieldを維持しました。合法手sampler・実行・終了snapshot検証込みで、学習や推論の速度ではありません。版間の速度改善比は主張しません。
勝数・他集計は[契約文書のv9記録](docs/official-execution-subset.md#v9の過去の検証と集計)へ記載しています。
v10は通常129件/20.43秒・ASan/UBSan89件/52.20秒、focused Release3件/0.70秒・SAN2件/1.37秒、Python Match18ケースが通過しました。全source/test最終freeze後のbuild/fullでwarning/error0、公式assets再取得hash/296facts一致。2人10万局・9人1万局は1/4 workersの全決定的fieldが一致し、終了snapshotを全件restoreできました。Release/SAN各1,000局も時間以外が一致しました。閃光の追加/解除/応答件数と独立速度は[契約文書のv10記録](docs/official-execution-subset.md#v10の過去の検証と集計)へ記載しています。

v10検証範囲の訂正：専用native Flash2関数がmainへ未登録だったため、そのcounter FIFO/canonical偽造/late rollback等をv10実行済みとした範囲は撤回する。CTest件数・C API/Python18ケース・大規模benchの記録値は維持し、v11で登録を修正して初実行し、全source/test最終freeze後の検証でもfixture修正無しでRelease/SANとも通過した。 GNU/Clangではofficial_match native test targetに `-Wunused-function -Werror=unused-function` を追加し、同様の未使用テスト関数をcompile errorにする。
以上のv10計測も過去の証拠として保持し、v11の検証・計測とは分けます。
v11は全Release CTest129件/20.55秒・ASan/UBSan89件/52.21秒、focused Release3件/0.65秒・SAN2件/0.88秒が通過しました。全source/test最終freeze後build/full・4buildログwarning/error0、Python MatchはAST確認21ケース、公式assetsのfresh取得hash/296factsも一致。native31関数の呼出しを確認し、v10の登録漏れ2関数も今回初実行してfixture修正無しでRelease/SAN通過、unused-function guardは実flags/negative probeでも確認しました。

| v11人数 | 試合数 | 意思決定数 | draw / 打切り | 暗雲追加 / 解除 | chance / counter forced | checksum |
| ---: | ---: | ---: | ---: | --- | --- | --- |
| 2 | 100,000 | 3,940,299 | 138 / 0 | 15,714 / 1,683 | 5,489 / 176 | `11534437455040832945` |
| 9 | 10,000 | 2,519,780 | 29 / 0 | 11,595 / 1,927 | 13,207 / 156 | `5814487950518364984` |

forcedを実draw hitへ混ぜず、miss=rolls−hitsを維持します。1/4 workersはworker数・時間以外の全fieldが一致し、全終了snapshotをrestoreできました。Release/SAN seed42各1,000試合も時間以外の全fieldが一致し、checksumは2人 `17832090841414215461`、9人 `15509475549777632243`。
競合を避けたRelease・1 workerでは、2人100,000試合が2.46388秒（1,599,230意思決定/秒）、9人10,000試合が2.75792秒（913,652意思決定/秒）で全決定的fieldを維持しました。合法手sampler・実行・終了snapshot検証込みで、学習や推論の速度ではありません。版間の速度改善比は主張しません。
勝数・他集計は[契約文書のv11記録](docs/official-execution-subset.md#v11の過去の検証と集計)へ記載しています。
以上のv11計測も過去の証拠として保持し、v12の検証・計測とは分けます。
v12は全Release CTest129件/18.63秒・ASan/UBSan89件/49.95秒（各 `-j4` 同時実行）、focused Release3件/0.58秒・SAN2件/1.28秒が通過しました。全source/test最終freeze後の最終2full build/全検証でwarning/error0、Python MatchはAST確認23ケース、native零引数テスト34関数main明示登録と公式assetsのfresh取得hash/296facts一致も確認しました。

| v12人数 | 試合数 | 意思決定数 | draw / 打切り | 205解決 / token除去 | 206解決 / token除去 | checksum |
| ---: | ---: | ---: | ---: | --- | --- | --- |
| 2 | 100,000 | 3,942,252 | 133 / 0 | 5,402 / 15,724 | 5,526 / 1,131 | `6915174992826563900` |
| 9 | 10,000 | 2,511,295 | 20 / 2 | 4,327 / 12,845 | 4,324 / 1,745 | `2152079972708342754` |

解決数は空候補を含み、除去数は使用consumeを除いた実token消失で、補充を相殺しません。205は3×解決数/206は2×解決数以下を確認しました。
1/4 workersはworker数・時間以外の全fieldが一致し、全終了snapshotをrestoreできました。Release/SAN seed42各1,000試合も時間以外の全fieldが一致し、checksumは2人 `11958536962789411924`、9人 `6514456865413471030`。
競合を避けたRelease・1 workerでは、2人100,000試合が2.37831秒（1,657,590意思決定/秒）、9人10,000試合が2.67775秒（937,838意思決定/秒）で全決定的fieldを維持しました。合法手sampler・実行・終了snapshot検証込みで、学習や推論の速度ではありません。版間の速度改善比は主張しません。
勝数・他集計は[契約文書のv12記録](docs/official-execution-subset.md#v12の過去の検証と集計)へ記載しています。
以上のv12計測は過去の証拠として保持し、v13とは分けます。
v13は全Release CTest129件/20.39秒・ASan/UBSan89件/51.58秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、focused Release3件/0.94秒（native0.38/C API0.01/Python0.55秒）・SAN2件/1.52秒（native1.17/C API0.35秒）が通過した。Python MatchはAST確認25ケース、native零引数37関数は全てmainへ明示登録を確認した。最終2buildと前段bridge/bench3ログのwarning/errorは0。公式assetsのfresh取得hash/296モデルfactsも一致した。

| v13人数 | 試合数 | 意思決定数 | draw / 打切り | 58使用 / 支払いMP / 基底MP | 14使用 / 完了frame | checksum |
| --- | --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,862,744 | 121 / 0 | 7,394 / 86,103 / 85,307 | 7,322 / 14,489 | `2365312122692827753` |
| 9 | 10,000 | 2,474,625 | 22 / 1 | 4,797 / 61,321 / 60,719 | 4,761 / 9,382 | `9384306151148000397` |

58の支払いMPはMC費用込み、基底MPは費用後残額。14完了frameはblock/完全防御0を含み、実被害hit数ではありません。
1 worker/4 workersでworker数・時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。Release/SAN seed42各1,000試合も時間以外の全fieldが一致し、checksumは2人 `14212402720306530348`、9人 `15651691549885442780`。観測最大redirect深度は機能上の連鎖上限ではない。
競合を避けた独立Release・1 workerは、2人100,000試合が2.36604秒（1,632,580意思決定/秒）、9人10,000試合が2.75418秒（898,499意思決定/秒）。baselineと時間以外の全fieldが一致した。合法手sampler・実行・reset・各試合の終了snapshot検証込みで、合成単体・Python学習・モデル推論/最適化・公式全モデルの速度ではない。プールと方策が異なるため版間の速度改善倍率を主張しない。
勝数・他集計は[契約文書のv13記録](docs/official-execution-subset.md#v13の過去の検証と集計)へ記載しています。
以上のv13計測は過去の証拠として保持し、v14とは分けます。
v14は全Release CTest129件/19.09秒・ASan/UBSan89件/50.23秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、Release/SAN/bench/SAN bench/bridgeの5ログはwarning/error0。focused Release3件/0.81秒（native0.32/C API0.01/Python0.47秒）・SAN2件/1.17秒（native0.83/C API0.34秒）が通過した。Python MatchはAST確認27ケース、native零引数40関数は全てmainへ明示登録を確認した。公式assetsのfresh取得hash/296モデルfactsも一致した。
seed42の大規模集計です。

| v14人数 | 試合数 | 意思決定数 | draw / 打切り | 232使用 / 完了frame | 14使用 / 完了frame | checksum |
| --- | --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,868,837 | 119 / 0 | 9,727 / 9,793 | 7,389 / 14,631 | `7280126839917179986` |
| 9 | 10,000 | 2,392,134 | 23 / 0 | 8,167 / 45,210 | 4,587 / 9,563 | `11171425924432160273` |

232使用は複数copyでも1主動作。完了frameはblock/完全防御0も含み、14+232はwide/repeat両集計へ入ります。実被害hit数や排他的件数ではありません。14は最大2*(人数−1)frameへ広がります。
各人数の1 worker/4 workersでworker数・時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。観測最大redirect深度4は機能上の連鎖上限ではない。
Release/SAN seed42各1,000試合でも時間以外の全fieldが一致し、checksumは2人 `9992836881741631580`、9人 `4056226161425745428`。
競合を避けた独立Release・1 workerは、2人100,000試合が2.45876秒（1,573,490意思決定/秒）、9人10,000試合が2.69532秒（887,513意思決定/秒）。baselineと時間以外の全fieldが一致した。合法手sampler・実行・reset・各試合の終了snapshot検証込みで、合成単体・Python学習・モデル推論/最適化・公式全モデルの速度ではない。プールと方策が異なるため版間の速度改善倍率を主張しない。
勝数・他集計は[契約文書のv14記録](docs/official-execution-subset.md#v14の過去の検証と集計)へ記載しています。
以上のv14計測は過去の証拠として保持し、v15とは分けます。
v15は全Release CTest129件/18.87秒・ASan/UBSan89件/49.59秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、release/sanitize/bench/SAN bench/native/bridge-fixの6ログはwarning/error0。focused Release3件/0.63秒（native0.08/C API0.01/Python0.53秒）・SAN2件/1.00秒（native0.68/C API0.32秒）が通過した。Python MatchはAST確認29ケース、native零引数43関数は全てmainへ明示登録を確認した。公式assetsのfresh取得hash/296モデルfactsも一致した。
seed42の大規模集計です。

| v15人数 | 試合数 | 意思決定数 | draw / 打切り | 202使用 / 解決 | heal / damage解決 | 実HP増 / 減 | checksum |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,889,410 | 125 / 0 | 10,938 / 10,938 | 5,513 / 5,425 | 55,130 / 52,236 | `16376680674247028005` |
| 9 | 10,000 | 2,403,179 | 20 / 0 | 8,180 / 8,180 | 4,073 / 4,107 | 40,728 / 38,842 | `8098081484010360106` |

heal/damageは符号の完了解決分類、実HP増減はclip後の合計です。99のhealで実増0でも解決へ数えます。規約の50/50を公式serverで測定した結果とは扱いません。
各人数の1 worker/4 workersでworker数・時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。Release/SAN seed42各1,000試合でも時間以外の全fieldが一致し、checksumは2人 `3640146110508428199`、9人 `12830718056704997050`。観測最大redirect深度4は機能上の連鎖上限ではない。
競合を避けた独立Release・1 workerは、2人100,000試合が2.49299秒（1,560,140意思決定/秒）、9人10,000試合が2.65853秒（903,949意思決定/秒）。baselineと時間以外の全fieldが一致した。合法手sampler・実行・reset・各試合の終了snapshot検証込みで、合成単体・Python学習・モデル推論/最適化・公式全モデルの速度ではない。プールと方策が異なるため版間の速度改善倍率を主張しない。
勝数・他集計は[契約文書のv15記録](docs/official-execution-subset.md#v15の過去の検証と集計)へ記載しています。
以上のv15計測は過去の証拠として保持し、v16とは分けます。
v16は全Release CTest129件/17.77秒・ASan/UBSan89件/47.15秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、release/sanitize/bench/SAN bench/native/bridgeの6ログはwarning/error0。focused Release3件/0.62秒（native0.08/C API0.01/Python0.53秒）・SAN2件/1.21秒（native0.90/C API0.31秒）が通過した。Python MatchはAST確認31ケース、native零引数46関数は全てmainへ明示登録を確認した。公式assetsのfresh取得hash/296モデルfactsも一致した。

v16検証範囲の訂正：C APIの固定seed照会6本の開始値を誤って275に変更し、条件seed<64へ入らなかったため、閃光解除・暗雲forced・ホウキ・石けん・MP武器・HP増減の各64回、計384照会がv16では未実行だった。この384件をv16で実行済みとした範囲は撤回する。全Release129件/SAN89件の通過・他assert・native/Pythonの相当照会・大規模計測値は事実として保持する。v17では6loopを `seed{}`（0）へ戻し、`fixed_query_probes==384` の完走必須assertを追加した。Wide+FREE探索も開始275から0へ戻すが、こちらはv16でも275以降で探索を実行していた。v17の修正後Release/SANで384照会完走を再検証して通過した。過去のv16検証範囲を遡って増やさない。
seed42の大規模集計です。

| v16人数 | 試合数 | 意思決定数 | draw / 打切り | 復活発動数 | 202 heal / damage | 202実HP増 / 減 | checksum |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,974,476 | 145 / 0 | 5,620 | 5,486 / 5,648 | 54,858 / 54,253 | `6692312305902983439` |
| 9 | 10,000 | 2,456,213 | 44 / 2 | 5,620 | 4,034 / 3,976 | 40,340 / 37,650 | `18168476733015740621` |

復活発動数はnative累計差分による実消費数で、play/defendの選択使用数 `physical_used` と別計上です。202の分類・実HP増減から復活HP+10を除き、negativeから復活してnet正でもdamage分類を保ちます。
各人数の1 worker/4 workersでworker数・時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。Release/SAN seed42各1,000試合でも時間以外の全fieldが一致し、checksumは2人 `13347754452093966556`、9人 `1870282473665283638`。観測最大redirect深度5は機能上の連鎖上限ではない。
競合を避けた独立Release・1 workerは、2人100,000試合が2.4242秒（1,639,500意思決定/秒）、9人10,000試合が2.75017秒（893,114意思決定/秒）。baselineと時間以外の全fieldが一致した。合法手sampler・実行・reset・各試合の終了snapshot検証込みで、合成単体・Python学習・モデル推論/最適化・公式全モデルの速度ではない。プールと方策が異なるため版間の速度改善倍率を主張しない。
勝数・他集計は[契約文書のv16記録](docs/official-execution-subset.md#v16の過去の検証と集計)へ記載しています。
以上のv16計測は過去の証拠として保持し、v17とは分けます。
v17は全Release CTest129件/18.30秒・ASan/UBSan89件/48.50秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、release/sanitize/bench/SAN bench/native/bridgeの6ログはwarning/error0。focused Release3件/0.76秒（native0.32/C API0.01/Python0.43秒）・SAN2件/1.25秒（native0.93/C API0.31秒）が通過した。Python MatchはAST確認33ケース、native零引数49関数は全てmainへ明示登録を確認した。公式assetsのfresh取得hash/296モデルfactsも一致した。修正C APIの6本×64照会はRelease/SANで `fixed_query_probes==384` の完走を確認した。
seed42の大規模集計です。

| v17人数 | 試合数 | 意思決定数 | draw / 打切り | 杵launch / 臼誘引 | 自己 / 味方選択 | checksum |
| --- | --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,851,617 | 110 / 0 | 7,400 / 559 | 3,757 / 0 | `11374515972360380290` |
| 9 | 10,000 | 2,378,375 | 18 / 1 | 4,471 / 1,076 | 795 / 0 | `17909990660097319005` |

杵/臼/自己/味方はlaunch件数で、解決・被害回数ではありません。ベンチは全員Soloのため味方0で、味方を抽選から除く意味ではありません。209の自動消費はphysical_usedへ混ぜません。
各人数の1 worker/4 workersでworker数・時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。Release/SAN seed42各1,000試合でも時間以外の全fieldが一致し、checksumは2人 `18068382678944874926`、9人 `8193855328192109680`。観測最大redirect深度5/6は機能上の連鎖上限ではない。
競合を避けた独立Release・1 workerは、2人100,000試合が2.43255秒（1,583,370意思決定/秒）、9人10,000試合が2.5677秒（926,266意思決定/秒）。baselineと時間以外の全fieldが一致した。合法手sampler・実行・reset・各試合の終了snapshot検証込みで、合成単体・Python学習・モデル推論/最適化・公式全モデルの速度ではない。プールと方策が異なるため版間の速度改善倍率を主張しない。
勝数・他集計は[契約文書のv17記録](docs/official-execution-subset.md#v17の過去の検証と集計)へ記載しています。
以上のv17計測は過去の証拠として保持し、v18とは分けます。
v18は全Release CTest129件/17.79秒・ASan/UBSan89件/46.30秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、release/sanitize/bench/SAN bench/native/bridgeの6ログはwarning/error0。focused Release3件/1.08秒（native0.29/C API0.02/Python0.77秒）・SAN2件/1.36秒（native1.00/C API0.35秒）が通過した。Python MatchはAST確認36ケース、native零引数52関数は全てmainへ明示登録を確認した。既存C API384照会guardも完走し、公式assetsのfresh取得hash/296モデルfactsも一致した。
seed42の大規模集計です。

| v18人数 | 試合数 | 意思決定数 | draw / 打切り | 両替使用 / 希望HP0 | 配分前sum合計 | checksum |
| --- | --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,839,935 | 142 / 0 | 69,512 / 1,229 | 4,247,506 | `4941009284767353250` |
| 9 | 10,000 | 2,428,289 | 23 / 1 | 31,266 / 601 | 1,782,132 | `11062027547654796276` |

希望HP0は要求配分で、復活後のHPとは別です。sum合計は配分前の資源総和。physical_usedはplay/defend/exchange選択の非MC数で、208/209の自動消費は別計上です。
各人数の1 worker/4 workersでworker数・時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。Release/SAN seed42各1,000試合でも時間以外の全fieldが一致し、checksumは2人 `1463939626686470704`、9人 `375287159278559213`。最大redirect深度は各表に示す観測値で、機能上の連鎖上限ではない。
競合を避けた独立Release・1 workerは、2人100,000試合が2.31177秒（1,661,030意思決定/秒）、9人10,000試合が2.58239秒（940,327意思決定/秒）。baselineと時間以外の全fieldが一致した。合法手sampler・実行・reset・各試合の終了snapshot検証込みで、合成単体・Python学習・モデル推論/最適化・公式全モデルの速度ではない。プールと方策が異なるため版間の速度改善倍率を主張しない。
勝数・他集計は[契約文書のv18記録](docs/official-execution-subset.md#v18の過去の検証と集計)へ記載しています。
旧37profileは保持し、学習器と学習済みモデルは追加していません。

以上のv18計測は過去の証拠として保持し、v19の検証結果とは分けます。

## v19の過去の検証と公開除去結果

v19は全source/test最終freeze後のfull Release CTest129件/21.01秒・ASan/UBSan89件/51.62秒が通過した（各 `-j4` 同時、rolloutとの競合あり）。最終Release/SAN full buildはwarning/error0。focused Release3件/1.27秒（native0.46/C API0.02/Python0.79秒）・SAN2件/1.18秒（native1.03/C API0.15秒）が通過した。native零引数55関数のmain明示登録/unused guard、Python AST38ケース、Release/SAN双方の `fixed_query_probes==384` 完走を確認した。公式配信assetは上記の差分確認であり、fresh hash一致とは記録しない。

| v19人数 | 試合数 | 意思決定数 | 遷移checksum | 解決済み除去event | 空結果event | 除去event checksum |
| --- | --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,839,935 | `4941009284767353250` | 10,194 | 4,120 | `16555731515668663194` |
| 9 | 10,000 | 2,428,289 | `11062027547654796276` | 7,817 | 2,655 | `13048935337781861835` |

seed42の2人100,000試合/9人10,000試合を各1 worker/4 workersで実行し、worker数・時間以外の全fieldが一致した。v18 baselineともruleset・時間等の計測fieldを除く既存fieldがchecksumまで全一致し、除去結果の公開追加が遷移/抽選/既存集計を変えないことを確認した。全試合の終了snapshotを復元できた。`removal_events=hand_removal_resolved+used_removal_resolved` と公開removed_modelsの累計数が実除去コピー数に一致するguardも通過した。空結果は0候補でも解決したbodyのeventで、未解決cutoffを含めない。Release/SAN seed42各1,000試合も時間以外の全fieldが一致し、v18の既存非時間fieldとも一致した。遷移checksumは2人 `1463939626686470704` / 9人 `375287159278559213`、新除去event checksumは2人 `17881167250890207477` / 9人 `11841601995346163861`。今回はquiet速度を再計測せず、v19の速度値や改善倍率を主張しない。

詳細は[限定実行器v19記録](docs/official-execution-subset.md#v19の過去の検証と公開除去結果)。



## v20の過去の検証と売却集計

v20は全source/test最終freeze後のfull Release CTest129件/21.25秒・ASan/UBSan89件/51.43秒が通過した（各 `-j4` 同時、rolloutとの競合あり）。最終Release/SAN full buildはwarning/error0。focused SAN2件/1.60秒（native1.20/C API0.40秒）が通過し、Releaseはfixture修正後Python単独1件/0.42秒と最終full suiteの通過を記録する。Release focused合計時間は掲載しない。native零引数59関数のmain各1回登録/unused guard、Python AST41ケースの重複無し、Release/SAN双方の固定query384完走を確認した。

| v20人数 | 試合数 | 意思決定数 | 勝数 | draw / 打切り | checksum |
| --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,988,600 | 49,818 / 50,061 | 121 / 0 | `16830441695517365036` |
| 9 | 10,000 | 2,453,031 | 1,046 / 1,087 / 1,127 / 1,144 / 1,170 / 1,116 / 1,138 / 1,044 / 1,113 | 14 / 1 | `5359086279512792637` |

| 人数 | 売却操作 / 最終解決 | 名目CP総額 | 奇跡商品配送 | 除去event / 空結果 | 除去event checksum |
| --- | --- | --- | --- | --- | --- |
| 2 | 66,834 / 66,834 | 381,497 | 3,795 | 9,825 / 4,163 | `10697524422810475650` |
| 9 | 30,416 / 30,416 | 170,764 | 2,177 | 7,377 / 2,655 | `170769133704707136` |

seed42の2人100,000試合/9人10,000試合を各1 worker/4 workersで実行し、worker数・時間以外の全fieldが一致、全終了snapshot（自然終局/打切り）を復元できた。Release/SAN seed42各1,000試合も非時間fieldが一致し、checksumは2人 `16214924716417635012`（SELL642回）/9人 `1248515908024842571`（SELL2,934回）。新SELLと既存の会計boundsを確認した。プール/方策/効果を増やした結果で、v19との遷移一致は主張しない。今回はquiet速度を再計測せず、v20の速度値や版間改善倍率を掲載しない。

詳細は[限定実行器v20記録](docs/official-execution-subset.md#v20の過去の検証と売却集計)。

## v21の過去の検証と購入集計

v21は全source/test最終freeze後のfull Release CTest129件/20.63秒・ASan/UBSan89件/51.45秒が通過した。最終Release/SAN buildはwarning/error0。focused Release3件/1.46秒（native0.68/C API0.23/Python0.55秒）・SAN2件/1.44秒（native1.09/C API0.34秒）が通過した。native零引数63関数のmain各1回登録/unused guard、Python AST45ケースの重複無し、Release/SAN双方の固定query384完走を確認した。

| v21人数 | 試合数 | 意思決定数 | 勝数 | draw / 打切り | checksum |
| --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 4,189,270 | 49,527 / 50,355 | 118 / 0 | `16901879550753049792` |
| 9 | 10,000 | 2,648,336 | 1,069 / 1,090 / 1,121 / 1,178 / 1,113 / 1,118 / 1,134 / 1,057 / 1,094 | 17 / 9 | `5307337226493388206` |

| 人数 | BUY操作 | 見積 / 購入決定 | bought / declined | empty / CP不足 / 未決cutoff | 成約名目CP |
| --- | --- | --- | --- | --- | --- |
| 2 | 66,229 | 61,060 / 61,060 | 30,447 / 30,613 | 593 / 4,576 / 0 | 160,439 |
| 9 | 66,971 | 62,291 / 62,291 | 31,091 / 31,200 | 250 / 4,430 / 0 | 163,807 |

seed42の2人100,000試合/9人10,000試合で各1 worker/4 workersのworker数・時間以外の全fieldが一致し、全終了snapshot（自然終局/打切り）を復元できた。新旧会計boundsと三phaseの意思決定数を確認した。Release/SAN seed42各1,000試合も非時間fieldが一致し、2人checksum `9959108341010664234`（BUY655/bought284/declined324）、9人checksum `14166364559885031647`（BUY6,766/bought3,156/declined3,185）。今回の大規模方策で未決cutoff件数0だったことを、cutoff返却が到達不能という根拠にしない。quiet速度を再計測せず、v21の速度値や版間の改善倍率は掲載しない。

詳細は[限定実行器v21記録](docs/official-execution-subset.md#v21の過去の検証と購入集計)。

## v22の過去の検証と病気tick集計

v22の最終検証でRelease CTest129件/15.02秒・ASan/UBSan89件/44.60秒が通過した（各 `-j4`、大規模benchは先に完了済み）。最終Release/SAN full buildはwarning/error0。focused SAN2件/1.26秒（native0.90/C API0.37秒）が通過し、ReleaseはC++2件（native0.35/C API0.20秒）と、PythonのWideサンプラassert修正後の単独1件/0.61秒、その後のfull suite通過を記録する。Release focused総3件の合計時間は掲載しない。SANではPythonが対象外で、最終C++テスト確定後に検証した。native零引数68関数のmain各1回登録/unused guard、Python AST48ケースの重複無し、Release/SAN双方の固定query384完走を確認した。本体コードは追加変更していない。suite所要時間を版間の速度比較へ使わない。

| v22人数 | 試合数 | 意思決定数 | 勝数 | draw / 打切り | checksum |
| --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,992,222 | 49,802 / 50,088 | 110 / 0 | `14285408868039593824` |
| 9 | 10,000 | 2,539,475 | 1,051 / 1,146 / 1,110 / 1,100 / 1,096 / 1,116 / 1,152 / 1,102 / 1,104 | 20 / 3 | `12415748028992245618` |

| 人数 | tick | 自然悪化 / Heavenfall | tick208救済 / 全蘇生 | tick HP増 / 減（復活前） |
| --- | --- | --- | --- | --- |
| 2 | 135,265 | 6,673 / 826 | 245 / 4,427 | 83,246 / 230,930 |
| 9 | 95,831 | 4,866 / 1,013 | 233 / 4,355 | 89,891 / 168,855 |

seed42の2人100,000試合/9人10,000試合を各1 worker/4 workersで実行し、worker数・時間以外の全fieldが一致した。全終了snapshot（自然終局/打切り）を復元し、新旧会計boundsを確認した。Release/SAN seed42各1,000試合も非時間fieldが一致し、2人checksum `11520684194578492807`（tick1,245/tick救済1）、9人checksum `17074919555374801115`（tick9,187/tick救済27）。自然悪化の集計はローカル実験乱数の観測結果であり、公式serverの確率測定ではない。今回はquiet速度を再計測せず、v22の速度値や版間改善倍率を掲載しない。

詳細は[限定実行器v22記録](docs/official-execution-subset.md#v22の過去の検証と病気tick集計)。

## v23の過去の検証と仮面/天国草集計

v23は全source/test最終freeze後のfull build・focused・full suiteで、Release CTest129件/19.51秒・ASan/UBSan89件/49.90秒が通過した（各 `-j4` 同時、大規模benchは先に完了）。最終Release/SAN buildはwarning/error0。focused Release3件/1.29秒（native0.08/C API0.26/Python0.94秒）・SAN2件/1.05秒（native0.61/C API0.44秒）が通過した。native零引数72関数のmain各1回登録/unused guard、Python AST51ケースの重複無し、Release/SAN双方の固定query384完走を確認した。

| v23人数 | 試合数 | 意思決定数 | 勝数 | draw / 打切り | checksum |
| --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,965,139 | 49,606 / 50,283 | 111 / 0 | `4864183686981653755` |
| 9 | 10,000 | 2,519,891 | 1,099 / 1,104 / 1,079 / 1,137 / 1,090 / 1,131 / 1,099 / 1,184 / 1,063 | 14 / 0 | `12279515150450788112` |

| 人数 | 168使用コピー | 198開始 / 最終解決 | 198実MP増分（99cap後） |
| --- | --- | --- | --- |
| 2 | 4,471 | 4,547 / 4,547 | 90,931 |
| 9 | 3,552 | 3,367 / 3,367 | 67,336 |

| 人数 | tick | 自然悪化 / Heavenfall | tick208救済 / 全蘇生 | tick HP増 / 減（復活前） |
| --- | --- | --- | --- | --- |
| 2 | 183,727 | 9,283 / 2,472 | 367 / 4,554 | 229,225 / 355,557 |
| 9 | 136,022 | 6,822 / 2,336 | 339 / 4,393 | 212,748 / 265,590 |

seed42の2人100,000試合/9人10,000試合を各1 worker/4 workersで実行し、worker数・時間以外の全fieldが一致した。全終了snapshot（自然終局/打切り）を復元し、新旧会計boundsを確認した。Release/SAN seed42各1,000試合も非時間fieldが一致し、2人checksum `15297651180555030086`（168コピー45/198開始36/実MP720）、9人checksum `16876268204354222509`（168コピー350/198開始352/実MP7,040）。この大規模seedで198開始数と解決数が一致したことを、未解決cutoffが起きない保証にはしない。公式serverの病気順や乱数の測定結果ではない。今回はquiet速度を再計測せず、v23の速度値や版間改善倍率を掲載しない。

詳細は[限定実行器v23記録](docs/official-execution-subset.md#v23の過去の検証と仮面天国草集計)。

## v24の過去の検証とFog集計

v24の最終検証でRelease CTest129/129件（55.56秒）・ASan/UBSan89/89件（85.03秒）が通過した。時間は並行suite実行の経過であり、エンジンの性能値や版間速度比較には使わない。native零引数77関数のmain登録、Python AST55ケースの重複無し、新bridge Fog4ケース、C API固定6×64 query384完走を確認した。動的Wall+FREE fixtureは最初のseed8990へ届くよう探索上限を8192から16384へ広げて実行した。source/native/benchの独立read-only監査で重大な指摘は無かった。

| v24人数 | 試合数 | 意思決定数 | 勝数 | draw / 打切り | checksum |
| --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,947,197 | 49,585 / 50,310 | 105 / 0 | `15312008538106736201` |
| 9 | 10,000 | 2,506,619 | 1,102 / 1,134 / 1,104 / 1,057 / 1,129 / 1,099 / 1,106 / 1,102 / 1,154 | 11 / 2 | `1417387029820349467` |

| 人数 | Fog本人main | Fog launch draw | intendedから対象変更 |
| --- | --- | --- | --- |
| 2 | 112,904 | 30,180 | 0 |
| 9 | 102,818 | 49,146 | 34,241 |

seed42の2人100,000試合/9人10,000試合を各1 worker/4 workersで実行し、worker数・時間以外の全fieldが一致した。全終了snapshot（自然終局/打切り）を復元し、新旧会計boundsを確認した。2人戦の変更0でもlaunchの一候補drawは消費する。Release/SAN seed42各1,000試合の非時間fieldも一致し、checksumは2人 `788222134307769052` / 9人 `57481880523464303`。Fogのroll/対象分布やsampler変更を含むローカル集計で、公式serverの測定や旧版との全profile非干渉の証明ではない。今回quiet速度は再計測せず、v24の速度値・改善倍率は掲載しない。

詳細は[限定実行器v24記録](docs/official-execution-subset.md#v24の過去の検証とfog集計)。

## v25の過去の検証と死亡時攻撃集計

v25の最終検証でRelease CTest129/129件（56.07秒）・ASan/UBSan89/89件（85.40秒）が通過した。並行suite実行の経過時間であり、エンジン性能や版間比較の値には使わない。両buildはwarning0。native零引数83関数はRelease/SANで全てmainへ各1回登録、Python AST59ケースは重複無し、新Death bridge4ケース、C API固定query384完走を確認した。source/benchの独立read-only監査で重大な指摘は無かった。

| v25人数 | 試合数 | 意思決定数 | 勝数 | draw / 打切り | checksum |
| --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,960,357 | 50,255 / 49,644 | 101 / 0 | `14674230429006413330` |
| 9 | 10,000 | 2,394,313 | 1,126 / 1,069 / 1,115 / 1,069 / 1,110 / 1,124 / 1,132 / 1,133 / 1,102 | 17 / 3 | `16501005794355218399` |

| 人数 | 自動112コピー | 予約frame / 開始 | 実roll / hit / miss | 無drawforced | 実defend応答 |
| --- | --- | --- | --- | --- | --- |
| 2 | 0 | 0 / 0 | 0 / 0 / 0 | 0 | 0 |
| 9 | 2,467 | 12,204 / 8,868 | 10,960 / 8,240 / 2,720 | 628 | 8,871 |

seed42の2人100,000試合/9人10,000試合で各1 worker/4 workersのworker数・時間以外の全fieldが一致し、全終了snapshot（自然終局/打切り）を復元できた。2人で死亡時fieldが全て0なのは自然終局優先gateに沿う結果である。Release/SAN seed42各1,000試合も非時間fieldが一致し、checksumは2人 `2410992088900070692` / 9人 `10897494970209546519`。deathと既存会計のboundsを確認した。このqueueや終局gateを公式serverで測定した結果とは扱わず、今回はquiet速度を再計測せず、v25の速度値・改善倍率を掲載しない。

詳細は[限定実行器v25記録](docs/official-execution-subset.md#v25の過去の検証と死亡時攻撃集計)。

## v26の検証と取引overflow集計

native最終88関数のmain登録、Python63 methodの重複無し、新bridge4ケース、C API固定query384完走を確認した。final focused bridgeは2/2件・合計0.94秒（C API0.30/Python0.63秒）が通過した。fullSELLで共通helperの候補順・usedMC/passive・gift0/支払順・Fog/反射・容量query境界を検査し、通常BUYではoverflow_model=nullを確認する。初期配送先が満杯でも190反射後の最終recipientだけを評価し、BUY decline/cutoffはfull sellerの原slot/tokenへ追加draw無しで返す境界もReleaseで確認した。最後の追加assertを含むSANの確認はfull suite欄へ記録する。満杯BUYへの偽raw quote到達coverageは主張しない。

v26の最終検証でRelease CTest129/129件（56.44秒）・ASan/UBSan89/89件（86.64秒）が通過した。両buildはwarning/error0。時間は並行suite実行の経過であり、エンジン性能や版間速度比較には使わない。最後に追加した最終受取先での容量判定・BUY返却時の追加draw無しも、最終Release/SAN双方のsuiteで確認した。

| v26人数 | 試合数 | 意思決定数 | 勝数 | draw / 打切り | checksum |
| --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,988,255 | 49,954 / 49,935 | 111 / 0 | `11173370200104368305` |
| 9 | 10,000 | 2,409,699 | 1,121 / 1,104 / 1,100 / 1,102 / 1,099 / 1,108 / 1,133 / 1,145 / 1,074 | 13 / 1 | `10091894475017822334` |

| 人数 | SELL開始 / 最終解決 | SELL overflow | BUY overflow | overflow events | overflow event checksum |
| --- | --- | --- | --- | --- | --- |
| 2 | 59,744 / 59,744 | 0 | 0 | 0 | `0` |
| 9 | 25,928 / 25,928 | 9 | 0 | 9 | `11148538478604155371` |

seed42の各人数を1 worker/4 workersで実行し、worker数・時間以外の全fieldが一致した。全終了snapshotを復元し、新旧会計boundsを確認した。Release/SAN seed42各1,000試合も非時間fieldが一致し、checksumは2人 `760147037809871428` / 9人 `4393572629372685365`。通常BUYのoverflow0は現profileの空きの不変条件に沿い、fullSELLは専用goldenで候補順を検証する。2人ベンチのSELL overflow0はこのseed/方策での結果で、満杯SELLの到達不能性を示さない。候補分布は公式server未確認の実験規約であり、quiet速度の再計測や版間速度改善の主張はしない。

詳細は[限定実行器v26記録](docs/official-execution-subset.md#v26の検証と取引overflow集計)。

## 旧版を参考にした中立ルールセット

死亡時効果付き武器profileでは117を通常の光ATK1・75%武器として追加します。
[仕様とduel到達不能性の証明](docs/legacy-dying-weapon-profile.md)を参照してください。
死亡時威力30も台帳に保持しますが、dead設定後に生存者2人以上を要求するため2人戦では発火しません。
`g37_*`は88byte行動・41mask・1368特徴で、新しい死亡queueや保持は追加しません。
v36で208の手札属性観測がNEになっていた箇所を闇へ修正しました。効果payload・ゲーム軌跡・checksumは変わりません。

状態・解除奇跡profileでは202/208/211/212/224を追加し、MP付き初取得/保持cast・FREE・状態付与/解除とHeaven転落を扱います。
[状態・解除奇跡profileの仕様とAPI](docs/legacy-status-miracles-profile.md)を参照してください。
旧22保持とindexを固定し、新5配列と忘却27種を分け、効果直後と自然tick/244を検算します。
`g36_*`は88byte行動・40mask・CAST_STATUS kind19、FREE保持base48〜52を使います。
固定commitの全364行を取得し、新5行と旧225種のfactを[新しい出典証拠](data/legacy-status-miracles-v1/immutable-comparison.json)で照合しました。過去台帳の出典記録は当時のまま保持します。

物理追加ATK profileではSUNDRY238を追加し、MP/保持化なしのboost10として通常/順序付き武器攻撃へ加えます。
[物理追加ATK profileの仕様とAPI](docs/legacy-sundry-boost-profile.md)を参照してください。
後の倍化や57の有料MAGICによる再設定を保ち、238消費数×10を最終威力増分とは扱いません。基底/単独USE/防御には使いません。
`g35_*`は88byte行動・37maskで新kindを増やさず、旧価格表を固定して238の価格15だけ補います。

魔法跳ね返しprofileでは物理6種を追加し、MAGICへの応答でMP/保持化なしに現在sourceを応答者へ替えて、生存対象を選び直します。
[魔法跳ね返しprofileの仕様とAPI](docs/legacy-magic-bounce-profile.md)を参照してください。
21はATK5/boost5、残る5防具は普通DEF0〜4も保持します。210の保持奇跡とは区別し、共用の跳ね返し対象tapeを使います。
`g34_*`は88byte行動・36maskで新kindを増やさず、旧価格表を固定して6種の価格だけ補います。

HP増減賭けprofileでは237を追加し、使用時に一度だけ±10を抽選して公開し、現在targetのHPへ0/99上限で適用します。
[HP増減賭けprofileの仕様とAPI](docs/legacy-hp-gamble-profile.md)を参照してください。
先頭196の反射でも符号を引き直さず、実変化と自然tick/244を分けて記録します。
`g33_*`は88byte行動・35maskで既存USE kind3を使い、旧216価格表を固定して237の価格1だけ補います。

購入profileではBUY4を追加し、対象の現在の手札コピー1枚を抽選して、自己/空/現金不足を除くと現在buyerの購入確認phaseへ進みます。
[購入profileの仕様とAPI](docs/legacy-buying-profile.md)を参照してください。
YESで物理品を移し、NOで終了します。buyerは4/196を消費済みで最大15枚なので合法購入の配送は必ず成功し、MP/HPを購入代金に使いません。
`g32_*`は88byte行動・34mask・BUY/NO/YES kind16〜18で、旧216価格表を固定して4の価格5だけ補います。

強制売却profileではSELL3を追加し、手札商品を先に消費してtargetへYEN→MP→HPの順で請求し、物理カードを届けてsourceへ代金を加算します。
[強制売却profileの仕様とAPI](docs/legacy-selling-profile.md)を参照してください。
手札満杯で配送できなくても請求と代金加算は戻さず、奇跡は保持化せず物理品として移します。BUY4は未対応です。
`g31_*`は88byte行動・32mask・SELL kind15で、全216商品の独立価格表を使います。

杵/臼profileでは99/245を追加し、最初の臼所持者へ光raw999を向け、臼が無ければ自己を含むランダム対象へraw30を向けます。
[杵/臼profileの仕様とAPI](docs/legacy-pestle-mortar-profile.md)を参照してください。
臼は攻撃で消費せず、手動捨てで選んでも残ります。普通/特殊応答と240による強制除去を扱います。
`g30_*`は88byte行動・30maskで、旧29台帳と保持番号を変えません。

自己被害profileでは53を追加し、防御後のraw残余を現在source、続いてtargetから減らします。
[自己被害profileの仕様とAPI](docs/legacy-self-damage-profile.md)を参照してください。
自己対象では2回の減算となり、死亡走査・244・自然tickより前の実損失を別々に記録します。
`g29_*`は88byte行動・28maskで、旧28台帳と保持番号を変えません。

HP吸収profileでは武器26/42/103と魔法221を追加し、防御後の残余全量を現在sourceへ先に回復してからtargetへ与えます。
[HP吸収profileの仕様とAPI](docs/legacy-hp-absorption-profile.md)を参照してください。
targetの実HP損失へ回復量を制限せず、自己対象の上限処理・反射・221保持/FREE/忘却も扱います。
`g28_*`は88byte行動・27maskで、221は旧保持番号を動かさずmain slot15へ追加します。

手札除去profileでは240を追加し、対象の現在の手札からコピー1枚を等確率で取り除きます。
[手札除去profileの仕様とAPI](docs/legacy-item-removal-profile.md)を参照してください。
取り除いた対象には補充を与えず、未使用の奇跡も保持化しません。自己使用・空手札・先頭196反射を扱います。
`g27_*`は88byte行動・26maskを使い、操作は既存useのままです。

蘇生profileでは244を追加し、死亡確定前に最初の1枚を自動消費してHP10へ戻します。
[蘇生profileの仕様とAPI](docs/legacy-revival-profile.md)を参照してください。
補充予定は増やさず、病気・harmsを保ちます。同時HP0は固定seat0→1で処理し、既deadの人物は戻しません。
`g26_*`は88byte行動・25maskを維持し、新しい操作は追加しません。

交換profileでは2を追加し、自分のHP/MP/YENを各0〜99・総和保存で再配分します。
[交換profileの仕様とAPI](docs/legacy-exchange-profile.md)を参照してください。
HP0も合法で、自然な病気tickと配分前後の資源差を分けて扱います。売買は含めません。
`gg_*`は88byte行動・25mask、交換は新kind14と配分3値で指定します。

忘却profileでは241を追加し、対象の保持からコピー数に比例して1個を選んで失わせます。
[忘却profileの仕様とAPI](docs/legacy-forgetting-profile.md)を参照してください。
空の対象では抽選せず、属性Noneのため通常防御や195は使えず、先頭196だけが反射できます。
`gj_*`は88byte行動・24maskで、旧ABIと保持indexを保ちます。

MAGIC_FREE profileでは47/153/172/180/239を追加し、直後のFREE品でその直前1品のMPを無料にします。
[MAGIC_FREE profileの仕様とAPI](docs/legacy-magic-free-profile.md)を参照してください。
無料の奇跡でも保持化と配布予定を処理し、防御の特殊効果で途中終了すれば後置FREE品を未消費のまま残します。
`gi_*`は88byte行動・23maskで、旧ABIの幅を保ちます。

全体化奇跡profileでは209を追加し、MP5で武器を全敵対象・命中率100%へ変えます。
[全体化奇跡profileの仕様とAPI](docs/legacy-wide-miracles-profile.md)を参照してください。
1対1では相手を対象にし、必中でも命中抽選を消費します。`gt_*`は88byte行動・17mask、保持参照19です。

倍化奇跡profileでは214を追加し、MP6で順序列のその時点の威力を2倍にします。
[倍化奇跡profileの仕様とAPI](docs/legacy-doubling-miracles-profile.md)を参照してください。
単独使用はできず、保持参照18で反復できます。`gq_*` は88byte行動・15mask、旧`gp_*`は903特徴・13maskを維持します。

追加ATK奇跡profileでは火197・光217を追加し、単独の奇跡使用と武器への順序付き追加、同じ保持の反復を扱います。
[追加ATK奇跡profileの仕様とAPI](docs/legacy-additive-miracles-profile.md)を参照してください。
手札と保持を区別する最大64参照を使い、同じ列での初取得後の保持再使用、MP支払、属性setterと57の威力再計算を検証します。
`gp_*`は88byte行動・13maskを使い、以前のABIの形式を維持します。MP無料化は後段v23で扱います。

魔法遮断profileでは8種を追加し、全属性・命中済み確率・回復・所持金の奇跡を遮断できます。
[魔法遮断profileの仕様とAPI](docs/legacy-magic-block-profile.md)を参照してください。
MAGICに使うと必ず遮断し、武器・反撃には通常のATK/DEFの役割を保ちます。

MP武器profileでは57を追加し、使用時の全MP×2を基礎威力にしてMPを0にします。
[MP武器profileの仕様とAPI](docs/legacy-mana-weapon-profile.md)を参照してください。
MP0でも威力0の武器として使え、通常boost・属性setterを組み合わせられます。
v18では追加魔法とMP無料化は対象外です。v20の有料追加197/217と残る制約は[順序・保持・消費の調査](docs/legacy-additional-magic-rules.md)を参照してください。

万能反射profileでは196を追加し、先頭で使えば属性付き武器・奇跡・反撃を返せます。
[万能反射profileの仕様とAPI](docs/legacy-any-reflection-profile.md)を参照してください。
前置防具ありの制限を検証し、反撃の元所有者と現在sourceを区別します。

魔法反射profileでは40/171/174/178を追加し、攻撃・回復・所持金の奇跡を返せます。
[魔法反射profileの仕様とAPI](docs/legacy-magic-reflection-profile.md)を参照してください。
命中済みの確率奇跡も再抽選せず、元のMP支払・保持・効果を保ちます。
霧188は[観測仕様の未確定点](docs/legacy-fog-rules.md)を整理し、配布対象へ含めていません。

所持金反撃profileでは194を追加し、反撃元へ所持金を加算してから、相手へ同額を請求します。
相手の所持金が足りない場合はMP、HPの順に不足を支払い、HP0なら死亡を確定します。
[所持金反撃profileの仕様とAPI](docs/legacy-money-counter-profile.md)を参照してください。

反撃拡張profileでは190に加え、187火75%・191Glory・192DarkCloud・193自己MP回復を扱います。
[反撃拡張profileの仕様とAPI](docs/legacy-counter-extensions-profile.md)を参照してください。
混合する反撃の順序を行動で指定し、直接状態付与と自己回復をダメージから区別します。
v14では188霧・189幻覚・194所持金反撃は対象外。v15でも霧・幻覚は未対応です。

土のみの反撃profileでは190を追加し、普通防御後のraw残余を2倍にしてキューへ積みます。
[反撃profileの仕様とAPI](docs/legacy-counter-profile.md)を参照してください。
元の攻撃で倒れても予約済み反撃は残り、全項目が終わってから病気tick・補充・勝敗を処理します。
反撃へさらに防御・反撃できる旧server受理を採用し、防御しない旧botとの差をC05に残しています。
v13では土のみ、v14では上記5種の異種混在順序を扱います。

属性消去profileでは防具195を追加し、有色・光・闇の効果を無属性にして防御・反射・遮断できます。
[属性消去profileの仕様とAPI](docs/legacy-attribute-removal-profile.md)を参照してください。
命中済み確率武器を返しても再抽選せず、奇跡や付随効果の由来を保ちます。
所持だけでは足りず、その応答で195を実際に選ぶ必要があります。

属性付け替えprofileでは火65・水85を追加し、使用順で作れる火・水・無属性の結果を6byteの行動で選べます。
[属性付け替えprofileの仕様とAPI](docs/legacy-attribute-change-profile.md)を参照してください。
追加使用時のATKは旧実装に合わせて0で、現行表示の+ATK2/5との差を衝突C04に残しています。
v11では属性消去195は未対応で、v12に分けて追加しています。

防御奇跡profileでは魔法跳ね返し210・武器遮断215を追加し、合計17種類の保持を扱います。
[防御奇跡profileの仕様とAPI](docs/legacy-defensive-miracles-profile.md)を参照してください。
回復や所持金の奇跡も跳ね返せます。保持からの防御では手札を減らさずMPを払い、連鎖終了まで補充を待ちます。
通常手番の15種類の保持maskを維持し、防御2種類は別のmaskと応答用行動で選びます。

攻撃奇跡profileでは13種を追加し、既存2種と合わせて15種類の保持・MP消費・再使用を扱います。
[攻撃奇跡profileの仕様とAPI](docs/legacy-offensive-miracles-profile.md)を参照してください。
通常防具での魔法防御、Glory付き・闇・確率攻撃の奇跡を含め、武器用反射との違いも扱います。
v9では追加魔法や防御奇跡は未対応です。旧版のMP・威力は現行抽出と異なる場合があります。

光・闇profileでは18種を追加し、Gloryの光武器も通常抽選から獲得できます。
[光・闇profileの仕様とAPI](docs/legacy-celestial-profile.md)を参照してください。
光の代用合成、光防具、闇の正残余による即死、光闇の確率武器を扱います。
光への防御規則の資料間衝突C01は残し、v8の対象プールに闇防具がないことを区別しています。v14では闇反撃192と光防具の合成でこの分岐へ到達します。

harms profileではDark Cloudの付与と必中化、Glory時の防御1枚制限、解除を扱います。
[harms profileの仕様とAPI](docs/legacy-harms-profile.md)を参照してください。
Gloryの防御制限は旧版botの挙動を採用しています。v7の通常プールにはGloryの付与元がなく、v7ではfixture専用です。
139種・重み327の限定プールであり、霧・幻覚などは未対応です。

病気profileではCold/Fever/Hell/Heavenの付与・悪化・継続効果・解除を追加しました。
[病気profileの仕様とAPI](docs/legacy-disease-profile.md)を参照してください。
自然悪化は旧版の増加方式で、現在の説明にある固定5%とは区別しています。霧や幻覚等は未対応です。

反応profileでは無属性武器への反射・跳ね返しを追加し、再防御を含む連鎖を扱います。
[反応profileの仕様とAPI](docs/legacy-reflection-profile.md)を参照してください。
通常防具を先に使う順序、自分への跳ね返り、連鎖終了時のまとめた補充も検証します。

確率profileでは単純な確率武器12種を追加し、命中時だけ防御応答を待ちます。
[確率profileの仕様とAPI](docs/legacy-chance-profile.md)を参照してください。
旧版の％ATKは全敵への各1回で、1対1では相手への1回。同一対象への連撃は未対応です。

属性profileでは火・水・木・土の神器54種と、攻撃・防御の属性合成を追加しました。
[属性profileの仕様とAPI](docs/legacy-elements-profile.md)を参照してください。
光・闇、属性の強制付け替え、特殊防御などは対応範囲に含めていません。

初期の奇跡profileでは、初使用での保持化・再使用・MP支払・再使用時の神器獲得まで扱います。
[奇跡profileの仕様とAPI](docs/legacy-miracles-profile.md)を参照してください。
対象の奇跡は旧ID216/220の2種類で、現行公式とは効果・コストが異なるため旧版profileとして区別します。

初期HP40・MP10・所持金20、初期配布10・手札容量16という旧版profileで動かします。
攻撃の先頭と追加ATK、攻防兼用武器、追加ATK防具、0防御神器、HP/MP回復を扱えます。
消費後は手札を詰め、配布予定数を蓄積し、応答を解決してから末尾へ補充します。
武器の捨て禁止、複数捨てで補充なし、武器不所持時の祈りで1枚獲得、周期ごとの手番順変更も実装しています。

[ルール・API・制限](docs/legacy-neutral-profile.md)、[旧版処理の調査](docs/legacy-neutral-rules.md)、
[神器台帳と出典](data/legacy-neutral-v1/README.md)を参照してください。
中立のみのv1 profileでは、属性、奇跡の保持、売買、状態異常、反射、守護神、蘇生、終末、複数人戦は未対応です。
未対応神器を渡すと拒否し、通常攻撃などへ置き換えません。

## 初期の簡略版で実装した範囲

以下の9スロット・521行動・73特徴の説明は `synthetic-neutral-duel-v1` に限ります。

- 2人、手札9枚、初期HP40・MP20、上限99、先手ランダム。
- 武器の複数枚同時攻撃、防具の複数枚同時防御。
- HP回復・MP回復・MPを消費するHP回復、1枚捨て、パス。
- 攻撃後に相手が防御を選ぶ独立した意思決定フェーズ。
- HP0で勝敗確定、固定プレイヤー座標の終局報酬 `+1/-1`。
- 合法手マスク、行動プレイヤーだけの手札を含む観測、seedによる再現。
- C ABIによるバッチ実行、依存追加なしのPython `ctypes` ラッパー。

状態・手札・合法手・観測は固定長配列です。C++の `reset/step/observe/legal_actions` は
動的メモリ確保を行いません。バッチは作成時にメモリを確保します。

### 独自ルールと未実装部分

この節は初期のsynthetic環境についての説明です。カードIDは公式IDと関係ありません。武器3種（威力3/6/10）、防具3種（防御3/6/10）、
回復2種（HP+5/+10）、MP回復1種（MP+5）、回復魔法1種（MP6でHP+10）の全10種。
全種類を等確率・復元抽出します。開始手札にも補充にも同じ分布を使います。

防御終了時に攻撃側→防御側の順で消費枠を補充し、防御側が次の通常手番を得ます。
回復・捨ては即補充して手番終了。パスでは補充しません。防御で使用した防具は全て消費します。
回復と捨てによる手番終了も本環境の独自仕様です。

属性相性、奇跡の攻撃、神器の固有効果、反射・跳ね返し、状態異常、守護神、
売買・所持金、複数人戦、公式の神器出現分布・補充条件などは未実装です。
公式サイトへの接続・アカウント操作はありません。第三者実装・画像は同梱していません。

## synthetic環境の学習インターフェース

C++ API: `include/godfield/env.hpp`。C ABI: `include/godfield/c_api.h`。

行動は固定の521種類です。

| ID | 通常フェーズ | 防御フェーズ |
| --- | --- | --- |
| 0 | パス | 防具なしで受ける |
| 1〜511 | 選択スロットの9bitマスク。武器の組合せ、または回復1枚 | 防具の組合せ |
| 512〜520 | 指定スロット1枚を捨てる | 非合法 |

例: スロット0と2は `(1 << 0) | (1 << 2) = 5`。手札スロットの順番は補充後も維持します。
非合法手は状態と乱数列を変えません。C++では `valid=false`、バッチでは `-3`。
バッチは全行を検証してから更新するため、非合法な行があれば全体を更新しません。

観測は行動プレイヤー視点の73個のfloatです。相手の手札内容と内部乱数状態は含みません。

| 位置 | 内容 |
| --- | --- |
| 0〜3 | 自分HP/MP、相手HP/MP。それぞれ99で正規化 |
| 4〜5 | 通常・防御フェーズのone-hot |
| 6 | 未解決の攻撃力 / 90 |
| 7 | 使用済み意思決定数 / 上限 |
| 8 | 自分が現在の攻撃側なら1 |
| 9 | 相手の手札枚数 / 9 |
| 10〜72 | 自分の手札9スロット × 7特徴。武器/防具/回復/MP回復/回復魔法のone-hot、威力/10、MPコスト/20 |

`actor` は次に行動する固定座席番号です。**手ごとに必ず交代するわけではありません。**
防御を終えたプレイヤーは続けて通常手番を持ちます。報酬配列は常に `[player0, player1]` です。
学習側では座席別の履歴を持ち、行動時点のactorと報酬を対応させてください。
単純に毎step価値の符号を反転する実装では正しく学習できません。

意思決定の上限はデフォルト512。上限は防御待ちでも適用し、勝敗とは別の `truncated` を返します。
時間切れ時の報酬は0、winnerは-1。最終stepでHP0なら `terminated` を優先します。
学習時の打ち切り・ブートストラップの扱いはこの違いを保ってください。
自動resetはしません。終了済みバッチ行は状態固定・報酬0・空の合法手マスクを返します。

Pythonでは `BatchEnv` をコンテキストマネージャーとして使用します。
`observe()` と `step()` の返却バッファーは次回の呼び出しで上書きされるため、
replay bufferに保存するときはコピーしてください。NumPyを使う場合は
`np.ctypeslib.as_array(buffer).reshape(...)` で配列として扱えます。
同じバッチハンドルを複数スレッドから同時操作しないでください。
Gymnasium/PettingZooアダプターやPPOはまだ同梱していません。

## 公式ルールに近づけるための次の作業

現行コード・公式画面の抽出記録・旧Flash版エミュレーターを比較した
[仕様差分表](docs/official-spec-gap.md)に、28項目の差分、資料間の衝突、対応順と受入条件を整理しています。

1. 固定した公式データ・表示関数・部分viewを、出典付きfixtureで継続照合する。
2. 限定実行器の実験的な配布・死亡・手番順を一次の対局結果と照合し、一般effect queueと未対応効果へ広げる。
3. 互換クラス外のDream、取引server挙動の未確定部分、Earth/Moonの公式一般神器/奇跡選出・207/残る現象/悪魔/終末を含む9人/チームの残りの範囲へ広げる。限定playableと公式全体の一致を別に検証する。

学習器は現在の実装範囲に含めません。旧37profileのEnvとAPIは比較用に保存します。

調査の入口: [旧Flash版の非公式エミュレーター](https://github.com/Igoorx/godfield-flash)、
[部分的なシミュレーターと学習系の先行実装](https://github.com/BrandenXia/godfield-bot)。
いずれも現行公式コアとしては扱わず、仕様確認用の資料として参照します。

## GitHub（private）

GitHubへ接続でき、`.git`への書き込みが可能な端末で実行:

```sh
sh scripts/publish-private.sh
```

認証済みアカウントの `godfield-rl` をprivateで作成するか、既存privateリポジトリへ更新をpushします。
既存originはfetch・pushの宛先がこのリポジトリと一致する場合だけ使い、上書きしません。
commit前とpush直前にprivateであることを確認し、現在のブランチへ通常pushします。
認証情報やビルド成果物はcommitしません。

## v27：公式台帳v2とclient出典の同期

v27時点では `official-execution-subset-v27`、Snapshot27（旧1〜26は移行せず拒否）、`official-execution-view-v25`。229モデル/重み490（trade3/weapons105/armor76/sundries17/miracles28）と、配布・効果・抽選・取引・死亡・病気の実験規約はv26から変更しない。公開viewに静的な `catalog_version` / `catalog_sha256` / `client_sha256` を加える。これらは配信資料の識別情報で、隠れた手札・token・乱数状態ではない。`catalog_version="official-v2"`、`catalog_sha256` は正規化した `cards.json` のSHA256であり、元JA全体のhashとは区別する。

| official-v2取得物 | bytes | SHA256 |
| --- | ---: | --- |
| 元JA JSON | 49,240 | `91b82c2600eac771c1a94dec80f249758496e2bbcb7befe6db52dacec1b55b02` |
| 元client JS | 440,420 | `5ece06c224b6a7a5d877eb45d22ea18b82a03aaf1b7161cbbdf6a1e23ceab2cd` |
| 正規化cards.json | manifest参照 | `8e465e2d296a2dd039d6e3b6e22956447c1fc4a5572a31e2597cdf20c551dc2a` |

v1とv2の296件を全field・欠損有無・ID順まで比較し、非表示fieldの変更は0。表示差分は19の画像 `chain-sickle→kusarigama`、26の `glaive-classic→naginata-classic`、117の `slate→stone-tablet` と名称 `石版→石板` の3行4fieldで、現行C++/C/Pythonの台帳へ同期した。旧v1のデータ、元JA/JS pins、crosswalk、過去のgolden vectors、legacy37のfingerprintは履歴として保存する。v1のarchive検査は現在のv2 C++生成物との同一性を要求せず、当時の資料自身の整合を検査する。

pure catalog/rulesの識別子は `official-catalog-client-rules-v2`。C ABIの構造体幅・関数は変更せずcatalog/state ABI1、Match ABI2（command184byte、opcode0〜8、purchase phase3/finished2）を維持する。ABI番号だけで出典まで一致したと判断せず、rulesetとcatalog/client hashも確認する。新Snapshot27はv1台帳を使った旧Snapshot26以前を受理しない。

最新版JSを直接オフライン実行した計算・選択50,270件、状態表示22,140件、資源算術7,898件、合計80,308件が一致した。v1の保存vector、新v2の生成vector、旧clientを再実行したvectorはbyte単位でも一致した。3つの抽出scriptは既知hashのv1/v2組合せを識別し、unknown hashと異なる世代の組合せを拒否する。startupやネットワークを実行せず対象関数/分岐を取り出す。状態照合は抽出分岐と描画stubを含み、全UI操作やserver対局の検証ではない。元ファイルの変更に追従して抽出点を識別する必要があり、旧JSの関数名・行番号を新JSへそのまま当てはめない。

表示差分の未反映gapを解消する更新であり、サーバー規約を補う更新ではない。Dreamの見かけと実体の不適合、守護神の生成/行動閉包、終末、自己BUY、94+Wide、通常giftの満杯時挙動は未対応または未検証のまま。旧37profileは保存し、学習器・学習済みモデルは追加しない。v10のnative Flash未登録とv16のCAPI384照会未実行の訂正履歴、および各版の過去数値を維持する。

[official-v2の取得・再生成](data/official-v2/README.md) / [v1→v2比較](data/official-v2/upgrade-review.json) / [v1 archive](data/official-v1/README.md)。

### v27の検証とv26回帰比較

最新版clientの80,308件の照合は上記の範囲で完了した。focused検証はRelease16/16件（3.68秒）・ASan/UBSan13/13件（5.07秒）が通過した。Matchのnative88群/Python63件は同件数、pure OfficialDataには最新表示3行を検査する1methodを追加した。旧v1 archiveとlegacy37のfingerprint保全も検査する。

seed42のv27・worker4で、2人100,000試合と9人10,000試合をv26の同条件worker4へ比較し、ruleset・追加の静的provenance・時間を除く全gameplay/既存counter fieldが一致した。意思決定数は3,988,255 / 2,409,699、checksumは `11173370200104368305` / `10091894475017822334`。他の勝数・draw・打切り・overflow集計もv26表と同じである。これは**v26との同条件worker4回帰比較**であり、v27単独のworker1/4再計測ではない。Release/SAN各1,000試合も一致し、2人/9人のchecksumは `760147037809871428` / `4393572629372685365`。quiet速度の再計測や速度改善比は主張しない。

v27の最終検証でRelease CTest130/130件（54.34秒）・ASan/UBSan90/90件（87.17秒）が通過し、両buildはwarning/error0だった。時間は並行suite実行の経過であり性能比較には使わない。最新clientから3oracleを独立に再実行し、全80,308件のTSVとv2の3manifestがbyte一致することを確認した。出力fileのsymlink6件・hardlink6件を有効client入力で拒否し、companion sentinelとv1入力のhashが不変、通常regular出力への再生成は成功した。未知hash・旧新不一致9件も再確認した。

## v28：互換クラスに限定したDream

`official-execution-subset-v28` はDreamの36/80/183/222を加え、233モデル/重み494（trade3/weapons107/armor77/sundries17/miracles29）を扱う。Snapshot28は旧1〜27を拒否し、公開JSONは `official-execution-view-v26`。Match ABI2の184byte command、opcode0〜8、purchase phase3/finished2、catalog/state ABI1、official-v2の出典3fieldは維持する。残る通常配布4モデルは178のredraw、守護神201/239、超常現象207（終末GFのscheduleも別途未対応）である。旧37profileと過去の記録・訂正を保存し、学習器を追加しない。

### 一次根拠と独自制限

公式JA/ENは、Dream中に新たに与えられる神器が50%の確率で別の神器に見えると説明する。公開clientは個体token・実モデル・fakeモデル・usedを分離し、手札faceと選択/MP表示ではfakeを優先し、送信には個体tokenを使う。使用時は受信した実モデルをrevealし、Dream解除時にもfakeを解除する。これは睡眠・行動skip・confusionによる自動行動ではない。

一方、公式の候補集合・重み、見かけ上合法で実体不適合な入力をserverがどう受理するか、売買・redraw・保持済み奇跡にいつ偽装するかは未確定である。本実装はこれらを推測で埋めず、**同じ選択・MP・target・commandの形を保つ互換クラスだけに限定した実験mask**を採用する。公式Dreamの全再現とは呼ばない。178は自身へのDreamだけでなく全手札redrawを含むため、今回の4種へ混ぜない。

| 追加モデル | 通常role / Dream role |
| --- | --- |
| 36 | 無属性ATK10武器・被害時Dream |
| 80 | 木ATK4武器・被害時Dream |
| 183 | 木防具・純Dream反撃（非attack/power0） |
| 222 | 木・MP6の直接Dream奇跡、used保持/FREE対象 |

36/80の付与は既存の通常targetdamage段階の実HP損失、183は既存survivor counter/FIFO、222は非attackの直接付与として扱い、trigger・死亡・反撃・補充順は既存の実験規約を引き継ぐ。状態は病気1種＋Fog/Dream/Flash/DarkCloudをcanonical順で持つ。Dreamは重複を持たず、Heavenへの非病気付与で転落させない。自然tickは追加しない。mild解除はDreamを残し、all解除はDreamと現在の全fakeを解除する。

### 互換クラスと抽選

signatureは `category, element, ability, curse, cost, hit_rate, is_plus_atk, atk>0, def>0` の9keyを完全一致させる。v28当時の候補は233 supportedモデルに限り、actualと同じIDは除く。146モデルに別face候補があり、87モデルはsingleton、全体は117クラス・異なるactual/faceの順序付きpairは992組である。数値ATK/DEF/abilityValue/priceの同値は要求しないので、表示威力・表示値と実際に解決される値が異なることはある。選択互換は効果値の同値を意味しない。

通常giftで実モデルを従来の重み付き抽選後、受取人がDream中かつ別の互換face候補がある場合だけbounded2を1回引く。0ならmaskし、候補からuniform boundedをもう1回引く。face候補が1個でもそのdrawは消費する。1ならmaskしない。Dream無し/候補無しは追加drawを行わない。したがって「eligible giftの50%」であり、全モデルが一律に50%変装する実装ではない。候補分布・singleton除外・抽選順は公式serverで未確認の独自規約である。

既存手札を遡及して変装しない。tradeで新たに受け取る商品には新maskを付けない。closed stateが受理するfakeはDream所持者の未使用個体で、actualと異なるsupported互換faceだけ。使用済み奇跡のfakeや互換クラス外のfakeをraw importで持ち込むことはできない。dead/終局の本人tooltipに実体を表示する契約と、手札faceの表示は区別する。

### 選択・reveal・公開境界

`legal` とsamplerはfaceに基づき選択し、実モデルで消費・MP・効果値・価格・payloadを解決する。受理された使用はactualをrevealし、保持される奇跡もunmaskする。失敗したcommandはreveal・抽選・token・状態を変更しない。既存fakeの売却/購入/除去/overflowもactualをrevealしてunmaskする。BUYは商品選出時にunmaskし、NO・CP不足・cutoffで返却しても元tokenはunmaskedのまま返す。このlifetime・売買適用範囲もローカル規約である。

公開 `last_reveals` は `[{owner,face,model}, ...]` として、使用・応答・quote・除去・overflowによる公開itemイベント相当のrevealを同じaction内の順で保持する。all解除で残手札のfakeを解除するrevealは、内部で `owner_only=true` とし、viewerが本人のときだけこの配列へ含める。他viewer・観戦者には該当entryを完全にfilterし、件数・face・実モデルも漏らさない。`removeCurses` が受信するcurse一覧は、他者の全手札を公開する根拠ではない。個体token・slot・元候補数・RNGは出さない。次の受理mainでclearし、失敗入力では保持する。抽選累計 `dream_rolls` / `dream_masks` やreplay anchorは特権snapshotだけに置き、プレイヤー向けviewへ渡さない。

v28時点の公開 `dream_rule` は当時のheaderと同じ次の文字列とする。これを公式server仕様の識別子とは扱わない。

```text
experimental:compatible-face-only;key-category-element-ability-curse-cost-hit-rate-plus-atk-positive-def-positive;supported-other-models-in-id-order;gift-real-before-eligible-only-half-coin-zero-masks-before-uniform-face;singleton-no-extra-draw;face-choice-one-draw-including-one-candidate;no-retroactive-mask;trade-receipts-unmasked;legal-and-policy-visible-models;resolve-actual-values-and-price;use-quote-removal-overflow-reveal;quote-return-stays-unmasked;all-cure-clears-dream-and-reveals-hand-only-to-owner;mild-preserves;no-tick;redraw178-unsupported
```

### v28の互換性照合と検証

[再現script](scripts/official_dream_oracle.cjs) と [互換クラス報告](data/official-v2/dream-compatibility-v28.json) を追加した。v28当時の233モデルに対し、別faceを持つのは146モデル（gift重み321）、singletonは87モデル（重み173）。全miracle・tradeと今回のDream付与4種はsingletonで、現在のfake生成対象は非miracle物理品だけである。保持MCをunmaskする共通実装があっても、現クラスで偽装MCの通常到達を検証したという意味ではない。

992順序付きpairについて、開始/既定target1,984件・MP計算14,880件・main追加693,408件・response追加1,111,040件、計1,821,312件のクライアントpredicate比較を報告に記録した。これらは検査したprefix/response contextに対する互換性であり、威力・回復値・DEF・価格の同値や、公式の偽装候補分布・server全入力受理を証明しない。公式の50%説明を満たす対象も本実装では146/233に制限される。台帳・allowlist・predicateが変われば再生成と閉包の再検査が必要である。

CAPI/Pythonのfocused検証2/2件が通過し、Pythonは67件が通過した。v27の80,308件・native88群/Python63件と、その全suite130/90件の記録は前版の証拠であり、今回のDream追加検証として読み替えない。今回の全検証結果は次のとおり。追加drawがあるため、v27の対局checksum不変は要求しない。速度改善比は主張しない。

v28の最終検証はRelease CTest130/130件（20.90秒）・ASan/UBSan90/90件（44.44秒）が通過した。native94群を全てmainへ登録し、Python67件・CAPI固定query384件も通過した。Dream oracleの `--check` で保存reportがbyte一致し、1,821,312比較を確認した。時間はsuiteの経過であり、版間の速度比較には使わない。

| v28・seed42 | 試合数 | 意思決定数 | checksum | eligible coin / masks | 公開reveal / 本人限定reveal |
| --- | ---: | ---: | --- | ---: | ---: |
| 2人 | 100,000 | 3,978,368 | `18145630014188039408` | 80,134 / 39,946 | 28,241 / 1,149 |
| 9人 | 10,000 | 2,408,047 | `11938917261778849633` | 69,094 / 34,635 | 24,346 / 1,633 |

同条件の1worker/4workersはworker数・時間・速度を除く全fieldが一致した。Release/SAN各1,000試合も全fieldが一致し、2人/9人のchecksumは `16592869786979077265` / `14472242390983499012`。coinはeligible giftだけの抽選回数、masksは生成したfake数であり、全giftが50%対象だったという集計ではない。revealの二分類はbench内部集計で、本人限定件数を他viewerの観測へ公開しない。版間の速度改善比は主張しない。

クライアントpredicateとの一致を検証しても、未確定の公式server候補分布や実体不適合commandの受理規約が確定するわけではない。178のredraw・守護神201/239・超常現象207（終末GFのscheduleも別途未対応）の残件と、自己BUY・94+Wide・通常gift満杯の別gapを維持する。

## v29：夢見る帽子178の神器一新

`official-execution-subset-v29` は178を加え、234モデル/重み495（trade3/weapons107/armor78/sundries17/miracles29）を扱う。Snapshot29は旧1〜28を拒否し、公開viewは `official-execution-view-v27`。Match ABI2のcommand184byte・opcode0〜8・purchase phase3/finished2、catalog/state ABI1、official-v2の出典を維持する。残る通常配布3モデルは守護神201/239と超常現象207（終末GFのscheduleも別途未対応）。Dreamの互換クラス外の偽装分布・実体不適合入力等は未確定のままで、公式Dream完全再現とは呼ばない。legacy37・過去の検証値・v10/v16訂正履歴は保存し、学習器を追加しない。

### 公開clientと旧Flashの限界

公式178は木DEF14・`selfCurseAndRedraw` / Dream・価格15・giftRate1で、説明は「使用者に夢と神器一新」。最新版JSの `redraw` は演出、`replaceItems` は受信したplayerId/itemsによる置換である。後者は対象の公開usedMiracles paneをclearし、本人のhandでは空placeholderを保ちつつ全nonempty表示slotをitemsで置き換える。used奇跡を除外する分岐は無い。usedMiraclesNoteの対象外記述はSELL/BUY/Broomについてであり、redrawのused保持を保証しない。これらは置換表示の根拠で、どの品を何枚抽選し、いつDreamを付け、複数帽子をどの順で解決するかを生成するserverロジックではない。

旧Flash相当品は旧ID184（木DEF14・ILLUSION）で、固定sourceのdefenderCommandは消費→補充1予約→自身ILLUSION→DEF減算だけを行う。全手札一新/REFRESHの処理は無く、別配列の保持magicsや帽子以外のpiecesを維持する。旧版の全queue後owner病気・敵助手機会・各alive playerへの補充や、Heaven中の任意harm付与HP0を、現行一新の根拠へ流用しない。

### 実験用の置換・会計規約

応答で選択された品を全て消費した後、選択順に178各copyを処理する。各copyで自己Dreamを付け、その時点の全nonempty実物手札（used奇跡を含む）を同数の新token・unused品へ即時置換する。空の補充予約placeholderは抽選枚数に含めず、既存の `s.gifts` 予約を維持する。抽選は現在の234/495限定プールのweighted drawと、従来のcompatible Dream maskを使う。帽子本体を消費した空きへ先に補充を実行する規約ではない。複数copyは各時点の新たな手札を次のcopyが再置換する。

used奇跡も新札へ置き換わるため元の能力は失うが、既に付与したcurseや実行済み効果を巻き戻さない。選択時のDEF14等は保存し、旧tokenが置換で無くなってもその応答の防御量は変えない。一新はincoming body・復活/死亡確定より前に行い、HP0の内部状態を途中snapshotとして公開しない。同actionの後続responseでは新札を使用できる。discardや売買の商品として動かした178は一新を発動しない。これらの順・枚数・token再発行・新札の即時使用はserver未確認のローカル規約である。

新札が同action中に追加されるため、物理品消費を初期人数×手札上限だけで制限した旧journal会計は流用できない。受理command全体を `max_decisions` で制限し、全action journalから消費・即時draw・MP・予約gift・復活・反撃と後続新札使用をreplayする。通常giftは従来どおり全action後で、即時redrawと別である。

公開 `last_redraws:[ownerID,...]` は全viewerへ一新対象のowner IDだけを知らせる。同actionで複数回なら順に記録し、旧手札の枚数・used数・fakeの有無/face/actualモデルは出さない。次の受理mainでclearし、失敗commandでは保持する。置換だけを理由に既存fakeの `last_reveals` を公開しない。本人は通常の自分のhand viewから新札を見られる。累計 `redraws` / `redrawn_items` とRNG/token/journalは特権状態で、public viewへ出さない。all解除の残fake revealは引き続きowner-onlyであり、一新通知と混同しない。

### v29の公開規約と検証

`redraw_rule` は次の文字列である。

```text
experimental:response-only178;consume-entire-selection-first;selection-order-self-dream-before-each-redraw;replace-all-current-nonempty-including-used-miracles;same-count-fresh-unused-tokens;immediate-weighted-draw-with-compatible-dream-mask;reserved-gifts-excluded-and-preserved;before-incoming-body-and-revival;no-old-hand-models-or-counts-public;owner-id-only-redraw-notice;same-action-new-items-usable;no-trigger-on-discard-or-trade;max-decisions-bounds-journal
```

`dream_rule` は末尾を `redraw178-same-mask-policy` へ変更する。

```text
experimental:compatible-face-only;key-category-element-ability-curse-cost-hit-rate-plus-atk-positive-def-positive;supported-other-models-in-id-order;gift-real-before-eligible-only-half-coin-zero-masks-before-uniform-face;singleton-no-extra-draw;face-choice-one-draw-including-one-candidate;no-retroactive-mask;trade-receipts-unmasked;legal-and-policy-visible-models;resolve-actual-values-and-price;use-quote-removal-overflow-reveal;quote-return-stays-unmasked;all-cure-clears-dream-and-reveals-hand-only-to-owner;mild-preserves;no-tick;redraw178-same-mask-policy
```

9keyの互換制限は維持し、178はsingletonとして追加する。146 eligibleモデル（重み321）・88 singleton（重み174）・118クラス・992順序付きpairで、全234モデルが50%変装する規約ではない。v28の1,821,312比較・native94/Python67とその対局値は前版の証拠として保持し、今回の一新を検証した値へ読み替えない。新版の[redraw再現script](scripts/official_redraw_oracle.cjs) / [client報告](data/official-v2/redraw-client-v29.json) は、実 `fH.lI`・実item decoder・実 `j9.jF` をオフライン抽出し、DOM/widgetだけstubに置き換えた。通常置換223例・不足配列1例・used pane19例を記録する。非empty slotにはused奇跡も含み、空placeholderを維持すること、used paneを全clearすることを照合した。不足配列でthrowし、余剰配列は未使用になるというrendererの挙動も含む。受信itemsのtoken/model/fake/usedを使う表示処理であり、serverの置換枚数・new token発行・used=false再配布・帽子/curse/body/復活/giftの順を証明しない。各helperを個別に呼ぶため、実際の全viewerへのevent fanoutや全UI操作の検証とも区別する。

新版の[Dream再現script](scripts/official_dream_v29_oracle.cjs) / [互換報告](data/official-v2/dream-compatibility-v29.json) は234 IDs/118クラス/146 eligible/88 singleton/992 pairを記録する。開始/既定target1,984件・MP14,880件・main追加696,384件・response追加1,111,040件、計1,824,288比較で、検査したclient選択・MP・targetの互換性を照合する。威力/価格/回復値の同値や公式偽装分布は保証しない。旧v28 script/reportは当時の233モデルの証拠として保存する。

seed42の大規模対局では、1worker/4workersで時間・worker数・速度を除く全fieldが一致した。

| v29 | 試合数 | 意思決定数 | checksum | redraw / redrawn items / notice | Dream coin / masks |
| --- | ---: | ---: | --- | ---: | ---: |
| 2人 | 100,000 | 3,994,686 | `12366315977385056288` | 4,304 / 21,660 / 4,304 | 119,719 / 59,481 |
| 9人 | 10,000 | 2,411,205 | `14932456998465571650` | 3,433 / 21,517 / 3,433 | 109,228 / 54,853 |

9人のtrade overflowは6件。redraw/itemsは特権bench集計で、各viewerへ旧手札枚数を公開するものではない。noticeの公開内容はowner IDだけである。これらの対局結果と、最終native/bridge/full suite検証は別に記録する。速度改善比は主張しない。

今回のnative検証は100群を全てmainへ登録した。使用済みTurbulence→自己bounce→帽子の順で、保持されていた反応carrierが一新で消えてもreplayできる接合を含む。nativeのRelease/SAN単体、Python70件・CAPI/Python focused2件・固定query384件が通過した。Release/SAN各1,000局も全fieldが一致し、2人/9人のchecksumは `10532187165089695254` / `6271775957059974581`。これらは独自規約の内部整合・来歴再生の証拠であり、公式server順を確認した証拠とは分ける。

v29の最終全suiteはRelease **130/130件**（17.20秒）、Sanitizer **90/90件**（47.06秒）が通過した。native100群はすべてmainへ1回ずつ登録され、Python70件、CAPI/Python focused2件、固定CAPI照会384回を実行した。新redraw oracleとDream v29 oracleの `--check` は保存reportとbyte一致し、旧v28 oracle/report・旧v1 archiveは変更していない。大規模1/4worker比較とRelease/Sanitizer各1,000局の非時間field一致は上記のとおり。これらはv29のローカル規約の検証であり、公式serverの配布枚数・抽選順・手番scheduleの同値性を証明しない。

## v30：自己BUYの入力と商品照会

`official-execution-subset-v30` は同じ234モデル/重み495（trade3/weapons107/armor78/sundries17/miracles29）のまま、自己BUYを実験規約で受理する。Snapshot30は旧1〜29を拒否し、公開viewは `official-execution-view-v28`。Match ABI2のcommand184byte・opcode0〜8・purchase phase3/finished2、catalog/state ABI1、official-v2の出典は維持する。新しいopcodeは追加しない。旧版の数値・v10/v16訂正履歴・legacy37は保存し、学習器を追加しない。

### 入力の一次根拠と実験規約

現行clientのtarget選択は自己paneを選べ、通常BUYのsubmitは個体tokenとtargetPlayerIdを送る。BUYの自己targetをUIが常に拒否するという根拠は無い。ただし、自己BUYが選ぶ商品、資金条件、応答・送金・移動・補充のserver処理を確認したことにはならない。既知のalive本人IDを明示targetにすることと、target省略は区別する。

固定clientの通常クリック経路も確認した。main開始の `fF` は `h8.aG()` の全alive playerへ `si1(true)` を呼び、本人を除外しない。各playerのtarget overlayに結ばれた `A.lz.$0` は選択先を `jO.aT` へ渡す。BUY選択時のenemy指定は初期値で、その後のalive本人クリックを禁止する分岐は無い。このcaller確認は固定sourceの静的追跡による根拠で、108ケースの実method試験と合わせて入力経路を確認した。

補助資料の旧Flashは、操作BUYを消費し補充1を予約した後、自己なら自動空防御から物理手札を1個選び、COMMANDへ選択品を載せて即終了する。商品を動かさず、後段の資金判定やYES/NO待ちへ到達しない。旧版は保持magicsが別配列であり、現行のused個体形式と同一ではない。この旧経路は今回の実験規約を選ぶ参考で、現行server一致の証明ではない。

主手番の `Command(actor, kind="buy", items=(buy_token,), target=actor)` は本人の未使用物理5一個を使う。allocation・MC/FREE混入・play5は許さず、MP費用0。物理5を消費後、その時点のunused自手札をinventory順に列挙し、一様bounded抽選を1回行う。1候補でも1draw、空なら0draw。使用済み奇跡は候補外で、別のBUYやpassive品・未使用奇跡は候補になり得る。選択品のactualを公開してunmaskし、操作5を消費した後の在庫で同じslot/token/used=falseを保ったまま置く。操作消費による通常のindex詰めを禁止する意味ではない。移動・escrow・新token配送・再抽選はしない。この抽選集合/分布/順はローカル規約である。

資金条件やCP送金は無く、priceは公開商品の値であり料金ではない。自己response・quote・YES/NOは作らず即時完了し、操作分gift1だけを全action後へ予約する。自己targetはFogによる敵対象変更から除外し、samplerも全alive対象へ本人を含める。対象無しや未知/死亡targetを許す契約ではない。候補空時の0drawは照会本体だけの話で、その後の通常giftによる抽選まで0という意味ではない。

### 結果・privacy・cutoff

非空なら既存の `last_purchase` に `reason="self_inspection"` を記録し、`bought=false` とする。native `PurchaseReason::self_inspection` は既存enumの末尾value5へ追加し、旧reason値は変えない。buyerとsellerは同じ本人IDで、商品model/priceは実体から取る。空なら既存の `empty` 結果（model0/price0）を使い、buyer=sellerを許す。private slot/token/候補数/RNGは公開しない。fake商品の照会は既存の公開item revealと同じローカル規約で `last_reveals` に載せ、本人限定のall解除revealとは分ける。

自己照会のbodyが完了した後にdecision cutoffへ達しても照会結果とunmaskは保持し、予約giftだけを取消す。次の受理mainで結果をclear、失敗入力では保持し、reset/loadではclearする。照会だけでcurseを解除したり、商品をusedへしたり、HPを変えたりしない。その後のowner病気tick・死亡/復活・gift/次ownerは既存の全action完了規約に従う。他者BUYの応答・資金条件・quote/YESNO・escrow返却は従来の別経路を維持する。

今回解消するのは自己targetを一律unsupportedとしていた入力gapである。自己照会の公式server規約が確定したという意味ではない。守護神201/239と超常現象207（終末GFのscheduleも別途未対応）は未着手の残件である。旧助手HP20/30%行動/肩代わり盾を現行へ流用しない。現教典の敵ターン後25%・被害時10%退去、現42行動の表示台帳と、種類の抽選・対象・Earth/Moonから神器/奇跡/召喚/現象へ広がる閉包のserver scheduleは区別し、後者を未確認のまま導入しない。

### v30の検証

[自己BUY再現script](scripts/official_self_buy_oracle.cjs) / [client報告](data/official-v2/self-buy-client-v30.json) は108ケースで実target選択 `jO.aT`・実alive click roster `h8.aG`・実team判定 `c5.ce`・実MP計算 `A.ld` と、submit `jS.$0` の最初のawaitまでを抽出実行した。自分/敵/味方のtarget、同じtargetの再選択、ID0/最大safe integer、MP0/CP0/Fog/Dreamを含む。108のRPC payloadをローカルstubで捕捉し、自己targetPlayerIdと物理itemIdsが送信され、このpreflightでCPを参照しないことを記録した。DOM/音/Future/backendはstubで、アプリ起動・ネット接続・server応答経路を実行しない。直接methodのdead/unknown-object検査不足は、通常alive rosterからのUIクリックやserverがそれらを受理する証拠ではない。

このoracleは照会商品の抽選、資金条件、送金、消費・gift、reveal、死亡・cutoffのserver順を証明しない。これらは上記のローカル規約である。v29以前の検証数・対局checksumは、その時点では自己BUYを拒否していた履歴として保持する。新native/C/Python・対局回帰の最終結果は以下に記録した。

公開 `buy_rule` は次のexact文字列とする。MatchのABI幅は据置だが、結果reasonを読むclientは新 `self_inspection` を認識する必要がある。

```text
experimental:one-operation-gift;uniform-unused-commodity-in-inventory-order;singleton-one-draw;empty-no-draw;foreign-cp-only-affordability;foreign-buyer-accept-or-decline;foreign-escrow-return-on-decline-or-cutoff;foreign-fresh-token-delivery;foreign-full-recipient-uniform-existing-item-overflow;self-immediate-inspection-after-operation-consumption;self-public-actual-model-and-unmask;self-same-slot-token-and-unused-state;self-no-cp-check-or-transfer-or-quote;self-result-before-cutoff-and-gift-after-action
```

今回のnative106群を全てmainへ登録し、Release/SAN単体が通過した。Python74件・CAPI/Python focused2/2件・固定query384件も通過し、自己BUYのprimary oracle108ケースはrootの `--check` で保存reportとbyte一致した。Release全CTest130/130件（17.18秒）は通過済み。SAN全suiteの結果は末尾の最終検証欄に記録した。

| v30・seed42 | 試合数 | 意思決定数 | checksum | 自己BUY = 非空照会 + 空 | foreign quote |
| --- | ---: | ---: | --- | ---: | ---: |
| 2人 | 100,000 | 3,991,570 | `9150208344638119859` | 45,014 = 44,283 + 731 | 42,051 |
| 9人 | 10,000 | 2,400,812 | `5747068493126455233` | 10,861 = 10,717 + 144 | 49,744 |

同条件1worker/4workersはworker数・時間・速度を除く全fieldが一致した。Release/SAN各1,000試合も全非時間fieldが一致し、2人/9人のchecksumは `127032000236014493` / `11541973377553558042`。BUYは既存trade categoryであり、対応プール234モデル/重み495を増やす更新ではない。official-v2のpinsとv29のDream/redraw oracleは変更しない。これらの対局集計は実験規約の内部整合の検証で、未対応scopeを実装済みとしたり、公式serverと同じ購入動作を証明したりするものではない。速度改善比は主張しない。

v30の最終全suiteはRelease **130/130件**（17.18秒）、Sanitizer **90/90件**（46.99秒）が通過した。native106群は全てmainへ1回ずつ登録し、Python74件・CAPI/Python focused2件・固定CAPI照会384回を実行した。自己BUY oracle108ケースは保存reportとbyte一致した。これらは実験規約の自己照会と既存scopeの回帰検証であり、自己BUYの現行server処理・守護神・超常現象・終末の公式完全対応を証明するものではない。

## v31：外部指定の守護神42行動

`official-execution-subset-v31` は、通常配布234モデル/重み495を維持したまま、別入口でcanonical guardian model245〜286の42行動を実行する。42種をgiftプールや通常手札へ入れる更新ではない。Snapshot31は旧1〜30を拒否し、公開viewは `official-execution-view-v29`。既存Match ABI2のcommand幅/opcode0〜8/phaseは変えず、24byte `GuardianAction` と追加のguardian入口で公開payloadを受ける。catalog/state ABI1、official-v2の出典、legacy37、過去の数値と訂正を維持し、学習器を追加しない。

これは**外部から選ばれた行動を進める実験基盤**であり、自動守護神scheduleではない。召喚201/239、守護神の種類抽選、敵ターン後25%の行動機会、被害時10%の退去、Earth/Moonによる一般神器/奇跡選出、超常現象207と終末GFは未実装のまま。scene importでguardiansを持つ状態を用意することと、公式の召喚・行動選択を再現することは区別する。特権sceneやsnapshotには他者の実手札を含み得るので、agentへそのまま観測として渡さない。

### 入力・進行の実験規約

native `GuardianAction{source, model, target}` はmain境界でのみ受理する。sourceはaliveで、stateのpersistent guardianが選んだcardのguardian metadataと一致する必要がある。sourceが現在のordinary ownerと同じである必要はない。280だけtarget無し、その他41種は既知のalive targetを明示する。自分・味方を含む指定を受け付けるのはこの外部入力の規約で、公式自動selectorが全ての指定を作るという証明ではない。

1回の外部注入を1decisionとして数え、必要な各responseも従来どおり別decisionとなる。sourceの手札・MPを消費せず、操作分giftも予約しない。相手の応答品・復活・285の補充は既存の予約giftを使う。ATK payloadは自己指定でもresponseを作り、非attackの自己対象と280は即時解決、その他の非attack foreign対象も通常responseを待つ。確率行動はこの明示target一人へ従来のhit抽選/DarkCloud保証を適用し、redirect後に再抽選しない。Fogを理由に外部指定targetを選び直さない。

264/286の `categoryWeapons` だけは実効categoryをweaponsへ変える。他40種はguardiansであり、全てを武器や奇跡へ置き換えない。通常ATKには属性に沿う普通DEF/filterと190、nonattackには190が適合し、264/286では武器専用反応もその属性条件に従う。metadataのATK/属性/curse/量を使い、効果の反射/反撃/死後FIFO・178の新札・復活は既存action journalで再生する。元guardianを表示するmetadataと、redirect後のcurrent source/targetは分離する。

全外部actionと反応が完了した後にalive分のgiftを処理し、ordinary ownerの病気tickやGF進行は起こさない。元ownerがaliveならそのmainへ戻り、死んでいれば次のaliveへ戻す。自然終局/decision cutoffは従来の優先規約を使う。これらのstage・応答・gift/owner復帰はserver未確認のlocal scheduleである。

### 効果の一次根拠と未確認selector

| model | 外部payloadの解釈 | 根拠と限界 |
| --- | --- | --- |
| 280 boostCPOfEverybody | 全aliveのCP+1、支払元無し、target無し | 受信handlerはalive pane列へbc(+cp)だけを適用。誰にいつ発行するかは別 |
| 281 boostCPToEnemy | 明示current targetのCP+5、source支払無し | 教典「敵にあげる」とboostCP受信のtarget加算。敵selectorの分布・許容対象は未確定 |
| 282 takeCP | current targetから名目3を減算→current sourceへ名目3を加算 | 受信takeCPと同じ順。clip99/不足CP→MP→HPを含み、実残高だけに請求を制限しない。自己aliasでも逐次 |
| 275 / 276〜279 | 全curse解除 / HP5・MP5・HP10・MP10のcurrent target回復 | 現台帳と受信効果。旧助手表のHP/MP順を使わない |
| 285 addItem | current targetへ通常gift1を予約、満杯ならgift draw0 | 一個付与の説明・addItem受信は根拠。候補選出/即時配送ではなくdeferredにするのは独自規約 |
| 286 categoryWeapons | 無属性ATK10の武器として実行 | clientがcategoryWeaponsを反応判定へ適用。Moonによる任意奇跡の選出/合成を実装した意味ではない |

他の固定行動も現42recordを使う。通常ATK、HP吸収、被害時Fog/Dream、純Dream/Flash/DarkCloud付与などの意味は既存bodyへ接続する。guardianから生じたという理由で物理品/保持奇跡を取得・再使用した扱いにしない。恒常guardian state、attack payloadのguardian、187/188/235/236等のcard metadataは別の情報である。201/239には固定guardian fieldがなく、10種uniform召喚の根拠を与えない。

現教典の25%行動・10%退去と、旧Flashの30%行動・HP20肩代わり盾は異なる。現MARSのATK3〜7、SATURN2/4/6/9/15やcategoryWeapons、NEPTUNEのHP/MP順、PLUTOの純curseなども旧表と異なる。旧Moon/Earthの一般神器再抽選、旧Heavenに任意harmで即死、旧Mysteryの木星ILLUSION・金星全員99等を現行へ流用しない。外部固定42を扱えても、召喚からEarth/Moon→201/239/207→現象へ広がる全選出閉包を実装したことにはならない。

### 公開境界・規約・検証

公開 `last_guardian` は受理された外部指定のsource/model/guardian/targetだけを示し、待機payloadにもguardian identityを表示する。未来の種類/行動/対象抽選やtoken・RNG・私的journalは公開しない。persistent guardianの状態欄は既存のFog可視性に従い、非表示になる場合がある。実行済み行動のguardian表示と、persistent状態欄・将来の抽選状態は分離する。`guardian_legal` もmatching guardian等の特権状態を参照する外部制御APIであり、player観測だけに基づく照会ではない。player向けvisible-only selection APIと同じ非干渉性を主張せず、特権scene/制御をagent観測へ混ぜない。入力失敗は従来どおりatomicで、応答待ち中に別の外部行動を差し込むことはできない。

v31時点の `guardian_rule` は次の文字列とする。

```text
experimental:externally-supplied-canonical-model245-286;main-boundary-only;source-owns-matching-guardian;explicit-alive-target-except280-all-alive;no-summon-move-target-departure-or-opportunity-sampling;metadata-values;categoryWeapons264-286-only;no-source-items-mp-fee-or-operation-gift;single-target-percent-roll-and-darkcloud-no-reroll;self-nonattack-and280-immediate;ordinary-response-counter-death-redraw-and-gifts;no-owner-tick-or-gf-advance;resume-owner-or-next-alive;one-decision-per-injection;earth285-one-deferred-gift;earth-moon-ordinary-model-selection-unsupported
```

[守護神再現script](scripts/official_guardian_oracle.cjs) / [client報告](data/official-v2/guardian-client-v31.json) は、実 `A.p6` attack decoder8ケース、実 `fs.fu` 受信frame・`gf2/iI` attack表示・`tz/lc/ld` 算術・`eu` 応答追加などの664投影を記録する。内訳は42原record×4条件の168、通常239モデルの単品239、仮Earth/Moon compound239、動的表示18。10種のguardian表示のreplaceとclearも個別に照合し、DOM/widget/CSS/text表示先はstubに置く。application startup・通信・serverの行動選出/phaseは実行しない。

通常239モデル単品や仮compoundの入力は、公式selectorがそれらを生成するという証拠ではない。decoderが任意DTOのatk/hitRate/mp/cpを読み、表示関数がoverrideを使うことと、実行器がそれを受理することも区別する。今回のAPIは245〜286のcanonical一モデルとmetadata値だけで、DTOの任意overrideやEarth/Moonの一般神器/奇跡合成は未対応。clientのtarget欠損が0へdefaultする挙動を、実行APIのtarget省略可否へ自動的に移さない。after-filter応答maskも、filter prefix自体の合法性を満たす場合だけ完成した応答として使える。

native `guardian_legal` / `guardian_step` とC `gfo_match_guardian_legal` / `gfo_match_guardian_step`、Python `GuardianAction` / `Match.guardian_legal` / `Match.guardian_step` を追加する。scene入口はC `gfo_match_load_scene`、Python `Match.load_scene(OfficialState, owner, seed)`。同じlibraryのlive stateをscope検査後に独立deep copyし、既存Configを維持してRNG/counter/pending workを初期化する。これはsnapshot checkpointの復元やserverイベント再生ではない。Pythonはrawのpending attacksを拒否し、callerはquiescent sceneを渡す。通常の `sample_action` は守護神を自動選択しない。

READMEの他のPython例と同様に `PYTHONPATH=python` で実行する最小例です。source/model/targetを手動指定し、自動召喚・行動選出は実行しません。

```python
from official_state import OfficialState
from official_match import Match, GuardianAction, Command

raw = {"gf": 0, "players": [
    {"id": 0, "hp": 40, "mp": 10, "cp": 20},
    {"id": 7, "hp": 40, "mp": 10, "cp": 20, "guardian": "saturn"},
]}
with OfficialState(raw) as scene, Match() as game:
    game.load_scene(scene, owner=0, seed=42)
    game.guardian_step(GuardianAction(7, 264, 0))
    status = game.step(Command(0, "defend"))
    board = game.observe()["board"]
    print(status.phase, status.owner, board["gf"], board["players"][0]["status"]["hp"])
    # main 0 0 25
```


新native8群を含む114群が通過した。守護神専用bench `./build/godfield_guardian_bench 420 9 42 2` のsmokeでは42モデルを全てcoverし、955decisions（420外部指定＋535responses）、70rolls/41hits/60forced、99redraws、checksum `7872010414762406687` を記録した。これは外部入力の局所試行で、守護神を自動抽選する通常対局の統計ではない。大規模回帰と最終suite結果は以下に記録した。速度比較は主張しない。

clientの受信表示を実行しても、召喚・selector・各stageを生成する権威的serverと同じと証明するわけではない。

primary oracleはrootの `--check` で保存reportとbyte一致した。native114群のRelease/SAN単体、Python78件・CAPI/Python focused2件・固定query384件が通過した。最終全suiteは末尾の最終検証欄に記録した。

通常対局のseed42・2人100,000試合/9人10,000試合は1worker/4workersで全非時間fieldが一致し、rulesetと新guardian_ruleを除くgameplay fieldはv30とも完全一致した。通常checksumは `9150208344638119859` / `5747068493126455233`。Release/SAN各1,000試合も一致し、checksumは `127032000236014493` / `11541973377553558042`。通常samplerが守護神を自動起動した結果ではなく、既存対局へ影響を与えていない回帰確認である。

| 外部守護神・seed42 | 試行数 | decisions / responses | covered models | terminal / cutoff | rolls / hits / forced | redraws | checksum |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 2人 | 100,000 | 227,967 / 127,967 | 42 | 2,967 / 0 | 16,667 / 10,052 / 14,286 | 24,125 | `2684282905704164173` |
| 9人 | 10,000 | 22,826 / 12,826 | 42 | 0 / 0 | 1,668 / 1,021 / 1,430 | 2,412 | `4158330987123714582` |

専用試行も1worker/4workersで全非時間fieldが一致した。Release/SANの420試行smokeもchecksum `7872010414762406687` が一致した。専用試行の42coverageや終局件数は外部指定sceneの検証で、公式の25%行動機会・種類/対象抽選や通常自動対局を評価した結果ではない。速度改善比は主張しない。

v31の最終全suiteはRelease **131/131件**（42.29秒）、Sanitizer **91/91件**（100.09秒）が通過した。native114群は全てmainへ1回ずつ登録し、Python78件・CAPI/Python focused2件・固定CAPI照会384回を実行した。新しい `official_guardian_benchmark` CTestも両buildで通過した。READMEのPython最小例をそのまま実行し `main 0 0 25` を確認した。これらはcanonical42行動の外部実行と既存対局の回帰検証であり、公式の召喚・自動guardian選出・超常現象・終末scheduleを再現した証明ではない。

## v32：召喚とcanonical守護神行動の通常対局接続

`official-execution-subset-v32` は201守護封印のつぼと239＜解放＞を加え、236モデル/重み499（trade3/weapons107/armor78/sundries18/miracles30）を通常配布する。Snapshot32は旧1〜31を拒否し、公開viewは `official-execution-view-v30`。Match ABI2の幅/opcode/phase、catalog/state ABI1、official-v2のpinsは維持する。外部 `GuardianAction` の入口も残す。残る通常giftモデルは超常現象207であり、終末GFのscheduleと同一視しない。legacy37、過去の数値・v10/v16訂正は保存し、学習器を追加しない。

### 一次事実と今回の独自selector

現教典は敵のターン後の行動率25%と、ダメージ時の退去率10%を説明し、公開台帳は10種の守護神と42行動を持つ。最初の8種の5行動はguardianAttackRate6/5/4/3/2（各合計20）で、clientの教典表示も20を分母にする。Earth/Moonの285/286には同じ重み表が無く、公開clientは種類/行動/targetを選ぶserverを実装していない。旧HP20の盾・30%行動・旧Earth/Moonの一般神器filterを持ち込まない。

今回の召喚はenum順に並ぶ10種からuniformに1種を選び、既存と同じtypeへの置換も許す実験規約である。201はsundryの非attack、239はMP15のmiracleで、通常の消費・used保持・FREE・反応規約を使う。守護神identityをcardのguardian metadataから固定して召喚する処理ではない。種類分布、同type許容、抽選/置換のstageは現行serverで未確認である。

通常ownerのmain actionが全て終わり、既存のowner病気tick/死亡・自然終局/decision cutoffの優先規約を処理した後、自動守護神のstageへ進む。aliveな敵guardian sourceを入力seat順に扱い、それぞれ25%のbounded抽選をする。成功した場合、最初の8種は公開の6/5/4/3/2でcanonical行動を選び、Earthは285、Moonは286を固定候補にする。候補一つでもmoveの抽選を1回消費する。全10typeを残すが、これは**canonical42に限る独自selector**であり、Earth/Moonの公式一般神器/奇跡選出を実装したという意味ではない。

ATK>0、純curse、わいろ281、没収282はalive敵からtargetをuniformに選び、他の単体効果はselfを含むalive味方から選ぶ。280はtarget抽選無しで全aliveへCP+1。これらの対象集合・分布・抽選順はローカル規約である。単体対象の抽選は候補一人でも1回消費する。選ばれたpayloadのhit/response・反射・反撃/死後FIFOを解決してから次sourceへ進み、main ownerが死亡したら次の自動sourceへは進まない。全てが終わってからalive分のgift・次owner/GFへ進む。未完了途中にgiftを引かない。

25%・move・target・命中など内部抽選は新しいdecisionを増やさず、応答commandは従来どおりdecisionとして数える。別入口の外部 `guardian_step` は従来の1decisionの注入で、自動stageを連続起動しない。自動種類・行動選択を方策から自由に指定できるplayer commandへ変更する更新でもない。

### 退去とHP算術

直接bodyで、sourceへの自傷stageまたはtargetへのdamage stageの実HP減少が正だったとき、その時点でguardianを持つ本人へ10%を1回抽選する。非attackの負の202によるHP−10も、この直接減少に含める。致死被害も復活前の実HP減少で判定する。退去後も既に起動したpayloadのmodel/guardian/powerを変更しない。病気tick、自己病気防具のprefix/Heaven転落、CP/MP不足のHP債務、両替の直接配分だけのHP減少は今回の退去triggerに含めない。raw残余・通常HP損失・死亡/復活のどこを基準にするか、同bodyの各stageで抽選することや除外条件はserver未確認の実験規約である。

自動payloadも275の解除、回復、HP吸収、curse、285の補充、264/286の武器category等をv31と同じbodyへ接続する。sourceの手札・MPを使わず、操作giftを作らない。応答品と復活/285の補充は通常のdeferred ledgerに従う。239の通常cast自体のMPと保持は別であり、guardian行動を使って239を無料取得する処理ではない。

### 公開履歴とreplay

公開 `last_guardian_changes` は `{player,before:null|guardianName,after:null|guardianName,reason:"summon"|"departure"}` の配列である。private `GuardianChange.hidden_viewers` のseat maskにevent発生時のFog可視性を保存し、各viewer向けにentryをfilterする。後からFog解除/付与・死亡/終局が起きたという理由で、当時見えなかった変化の件数やidentityを過去へ遡って公開しない。persistent状態欄は現在の既存Fog可視性に従う。`last_auto_guardians` は既に選ばれた行動だけを公開し、失敗した25%判定・未起動source一覧・未来move/target・cursor・RNG・内部累計を出さない。行動表示として分かったguardianと、状態欄/未来抽選情報を区別する。

病気tick後にguardianがHPやcurseを変えるため、`last_auto_guardians` が非空のときだけ、private last_tickと現在HP/病気の一致条件を緩めてhistorical recordとして扱う。tick自身の静的な数式と、全main/action journal/auto stageを含むfull replayは維持し、当時のtick結果・退去・召喚・抽選・反応・giftを検査する。この再生は明記したローカル規約の整合を確認するもので、任意の完成raw sceneが公式serverで到達可能だと証明するものではない。

### v32の規約と検証

新native/C/Python・通常対局/外部行動回帰の結果は確定後に追記する。v31以前の外部42専用試行や「通常対局不変」の記録は当時の結果として保存し、今回の自動stageを検証した値へ読み替えない。Earth/Moon一般神器/奇跡selector、超常現象207、終末GF、公式Dreamの全候補分布・実体不適合入力は残件である。


公開exact規約はheaderと同じ以下の3本とする。

`guardian_rule`:

```text
experimental:canonical42-only;auto-after-ordinary-owner-tick-before-gifts;alive-enemy-sources-in-input-order;one-quarter-opportunity-draw-per-present-source;weighted6-5-4-3-2-for-first8;earth285-moon286-fixed-singleton-move-draw;uniform-alive-enemy-for-atk-curse-bribe-takecp-otherwise-alive-allies-including-self;singleton-target-draw;280-all-alive-no-target-draw;single-target-percent-and-darkcloud;no-fog-reselection;counter-death-fifo-before-next-source;stop-on-main-owner-death;no-internal-decisions-or-source-fee-gift;external-main-boundary-one-decision-no-auto-followup;earth-moon-general-selection-unsupported
```

`guardian_summon_rule`:

```text
experimental:201-and239-current-target;uniform-all10-in-enum-order;one-draw-including-resummon-same-type;normal-consumption-mp-retention-and-gift
```

`guardian_departure_rule`:

```text
experimental:present-guardian-on-positive-direct-body-hp-loss;source-self-damage-before-target-loss;one10-percent-draw-per-stage;lethal-included-before-revival;include-negative-random202;exclude-disease-prefix-debt-tick-and-exchange;launched-payload-unchanged;event-time-fog-viewers-filter-changes
```

[新版Dream再現script](scripts/official_dream_v32_oracle.cjs) / [互換報告](data/official-v2/dream-compatibility-v32.json) は236 IDs・120クラス・146 eligible・90 singleton・992順序付きpairを記録する。開始/既定target1,984件・MP14,880件・main追加702,336件・response追加1,111,040件、計1,830,240比較である。201/239はsingletonとして追加され、旧v29のscript/reportも保存する。これは検査したclient predicateの互換性で、召喚/行動selectorや公式Dream全分布・威力/価格の同値を証明する結果ではない。

native120群を全てmainへ各1回登録し、Release/SAN単体が通過した。Dream v32 oracleの `--check` は保存reportとbyte一致した。通常対局と外部守護神専用試行の大規模各2人数、計4組で1worker/4workersの全非時間fieldが一致した。

| 通常対局・seed42 | 試合数 | decisions / responses | checksum |
| --- | ---: | ---: | --- |
| 2人 | 100,000 | 3,989,071 / 1,356,844 | `13564747736715185154` |
| 9人 | 10,000 | 2,361,978 / 1,062,264 | `17158162465123773162` |

| 通常対局 | summons | opportunities / auto actions | departure rolls / departures | auto records / change records |
| --- | ---: | ---: | ---: | ---: |
| 2人100,000試合 | 14,773 | 98,777 / 24,807 | 32,214 / 3,208 | 24,807 / 17,981 |
| 9人10,000試合 | 10,781 | 314,754 / 78,393 | 34,410 / 3,409 | 78,393 / 14,190 |

通常poolと自動stageが変わったため、v31のchecksum不変は要求せず、v31以前の数値を歴史として保存する。Release/SANの通常各1,000試合も全非時間fieldが一致し、2人/9人のchecksumは `1704475476168731728` / `13129530548541068452`。

| 外部守護神・seed42 | 試行数 | decisions / responses | coverage | departure rolls / departures | checksum |
| --- | ---: | ---: | ---: | ---: | --- |
| 2人 | 100,000 | 227,967 / 127,967 | 42 | 13,188 / 1,345 | `15242932842311417291` |
| 9人 | 10,000 | 22,826 / 12,826 | 42 | 1,338 / 140 | `678603818888237652` |

外部専用の2人/9人はterminal2,967/0、cutoff0/0、rolls16,667/1,668・hits10,052/1,021・forced14,286/1,430・redraw24,125/2,412。専用420試行smokeもRelease/SANで全非時間fieldが一致し、checksumは `12692595741725906978`。専用試行を公式の種類/行動selector評価と混同しない。

benchの202 body HP・198 body MP・187 counter MPは、捕捉したbody量と開始時resourceからclip/minで求め、同じstep内の自動Neptune回復を混ぜないよう集計を修正した。これは測定の区分修正であり、ゲーム状態遷移の変更ではない。集計値は特権bench用で、失敗opportunity・未来行動やprivate cursorをplayer向けJSONへ公開しない。速度改善比は主張しない。

v32の最終全suiteはRelease **131/131件**（135.99秒）、Sanitizer **91/91件**（199.86秒）が通過した。native120群は全てmainへ各1回登録し、Python82件（Release）・CAPI/Python focused2件・固定CAPI照会384回も通過した。C APIの全42外部行動と新しいauto/departure境界を維持し、READMEのPython最小例はそのまま実行して `main 0 0 25` を確認した。これらは明記したローカル規約と既存機能の回帰検証であり、公式serverの種類/行動/対象選択・処理順・全分布を再現した証明ではない。

## v33：typed外部Earth/Moon payload

`official-execution-subset-v33` は配布236モデル/重み499（trade3/weapons107/armor78/sundries18/miracles30）とv32のcanonical42自動行動を維持し、一般モデル列を**外部で選択して渡す**typed adapterを追加する。Snapshot33は旧1〜32を拒否し、公開viewは `official-execution-view-v31`。Match ABI2の既存command/phase/opcodeは変えず、64byteの追加構造体と関数・Python dataclass/methodsで `GuardianPayloadAction{source,guardian,models,target}` を扱う。catalog/state ABI1・official-v2 pins・legacy37・過去数値/訂正を維持し、学習器を追加しない。

これはEarth/Moonの公式selectorを実装する更新ではない。通常の25%機会で自動選ぶ行動は引き続きEarth285/Moon286のcanonical42であり、一般神器/奇跡の候補分布や公式filterは未確定のままである。207超常現象、悪魔、終末GF、公式Dream全分布/実体不適合入力を実装済みとも扱わない。

### 厳格な入力domain

sourceはaliveで、指定したEarthまたはMoonがpersistent guardianと一致する必要がある。main境界にだけ注入でき、sourceはordinary ownerと別でよい。全モデルが `match_supported` であることに加え、Earthはtrade/weapons/armor/sundries、Moonはmiraclesのdomainに**全列**が属し、実モデルを使う `can_append_main` に順に適合することを要求する。既知DTOを描画できることだけでは実行可能にしない。

Earthの先頭として使えるのは120モデル。trade3/4/5・Danger94・passive・response専用・addon-only先頭を拒否するため、domain名にtradeを含めても今回の入力で売買/両替を実行する契約ではない。Earth203等の合法nonMC追加は許し、18モデルを上限とする。chance武器は既存predicateにより追加不可。Moonは26種の対応単品だけで、231/232のaddon-only先頭も、複合Moon列も許さない。

Moon231/232やFREE204、EarthへのMC混在はunsupportedで、通常のplayer選択では合法な組合せでも外部adapterのdomain外なら拒否する。285/286を隠れたwrapperに追加したり、categoryを都合よく武器へ変えたりする特例は作らない。Wide232もこのadapterでは未対応。ここに記載した厳格domainを受理契約とする。

### 起動・MP・対象・戻り先

全てのpayloadに既知alive targetを一人明示する。自己非attackは即時、attackとforeign効果は従来のresponseへ進む。percentも明示した一人に対するchanceとして扱い、通常main chanceのtargetless全敵とは別である。14は同じ明示targetへfresh payloadを二回、counter/death FIFOを次hitより先に処理し、dead target skipやorigin死亡取消・自然終局/cutoffは従来の規約を使う。外部で決めたmodel/targetを自動selectorやFog/Dreamで選び直さない。202を基底にした場合の符号は起動時に一度だけ抽選し、反射後も固定する。

1注入は1decisionで、必要なresponse commandは別decision。sourceの手札を使わず、モデル列をsourceへ保持化/取得せず、普通のMC MP費用や操作giftを作らない。58だけは起動時のsource MP×2を基底にして合法nonMC追加を順に合成し、元MPを一度だけ0へする。redirectや二回攻撃で再計算/再課金しない。旧Flash助手のATK200固定・MP不消費ではなく、現clientのMP武器表示を参考にした実験規約である。

payload自身でsourceの実guardianが召喚/退去により変わっても、起動済みidentity/model/powerは保持する。通常の応答・反射・反撃・死後queue・178の新札・復活・deferred giftに接続し、ordinary ownerの病気tick・GF進行・自動guardian followupは起こさない。全体が完了したら元ownerがaliveならそのmainへ、死亡していれば次aliveへ戻る。これらの費用・対象・stageは公式serverで未確認の外部実行用規約である。

### 公開情報と一次根拠の限界

新 `last_guardian_payload` は受理されたsource/guardian/models/targetだけを公開する。canonical `last_guardian` と別のfieldで、payload起動時のcanonical fieldはnull。次の受理mainまたは外部actionで他イベントとともにclearし、応答中は保持、失敗入力では保持する。private累計 `guardian_payload_actions` と `batch.guardian_payload_action` / 起動時MP `guardian_payload_mp` は観測へ出さず、個体token・RNG・未来selector/queue・private cursorも追加しない。adapterのlegal/scene/実行入口は特権外部制御であり、player観測だけの照会とは扱わない。pendingの起動済みモデル列と、将来どの一般神器を選ぶかという内部抽選を区別する。

現clientのattack decoder・frame表示は複数 `itemModelIds` やguardian identity、動的ATK/MP/CPを受信できる。ただし、その配列をどのguardianが生成するか、source品の所有やMPをどう扱うかはserver側である。過去のv31 oracleの仮Earth/Moon compoundは表示式の試験であって、v33が全ての仮配列を受理する証拠ではない。

旧FlashのEarthはweighted一般神器から種類ごとに単体ATK/BUY・SELLと生成商品・ADD_ITEM贈与へ分岐し、Moonはdefensive MCを除外するrejectionとWIDE/DOUBLE用ATK10 wrapperを持つ。forced経路は所有品消費/MC保持/通常MP課金をskipするが、Earth MAGICALの200固定、TO_ENEMYの再抽選、旧Wide100%化など現行と違う処理もある。これらをそのまま現行の選出/合成規約に移さず、今回受理する実列と対象を外部から明示する。

### v33の規約と検証

native `guardian_payload_legal` / `guardian_payload_step`、C `gfo_match_guardian_payload_legal` / `gfo_match_guardian_payload_step`、Python `GuardianPayloadAction` / `Match.guardian_payload_legal` / `Match.guardian_payload_step` を追加する。Cのaction size queryは64byteで、既存command構造体を流用/拡張せずに追加する。単品候補のdescriptorだけで複合列全体の合法性を判断せず、完成したtyped列をlegalで検査する。現在の通常agent向けsamplerがこの特権外部payloadを自動選択する契約ではない。

`PYTHONPATH=python` を設定して、空の手札から明示した列を実行できる。

```python
from official_state import OfficialState
from official_match import Match, GuardianPayloadAction, Command

raw = {"gf": 0, "players": [
    {"id": 0, "hp": 99, "mp": 17, "cp": 20, "guardian": "earth"},
    {"id": 7, "hp": 99, "mp": 10, "cp": 20},
]}
with OfficialState(raw) as scene, Match() as game:
    game.load_scene(scene, owner=7, seed=42)
    game.guardian_payload_step(GuardianPayloadAction(0, "earth", (58, 203), 7))
    print(game.observe()["pending"]["power"])  # 44
    status = game.step(Command(7, "defend"))
    board = game.observe()["board"]
    print(status.phase, status.owner, board["gf"], board["players"][1]["status"]["hp"])
    # main 7 0 55
```

公開exact規約は以下とする。

```text
experimental:externally-selected-earth-moon-ordinary-model-columns;main-boundary-one-decision;alive-source-owns-explicit-guardian;all-models-in-guardian-category-domain-and-supported;actual-main-append;max18-models;earth120-moon26-standalone-bases;trade-danger-passive-response-only-bases-unsupported;mixed-domain-and-wide-unsupported;explicit-single-alive-target;14-two-fresh-hits-same-target;single-target-percent-and-darkcloud-no-reroll;no-fog-or-dream-reselection;no-source-inventory-retention-ordinary-mp-fee-or-operation-gift;58-launch-source-mp-times2-then-mp0-once;202-one-launch-sign;fixed-origin-guardian-and-launch-values-through-redirects;ordinary-response-counter-death-redraw-and-deferred-gifts;no-owner-tick-gf-or-auto-followup;resume-owner-or-next-alive;general-selector-unsupported
```

[payload一次oracle](scripts/official_guardian_payload_oracle.cjs) / [保存report](data/official-v2/guardian-payload-client-v33.json) を追加した。report SHA256は `b35a47b235d4d1645d030cf5ddb967292971bdb1dfaac36a7f17f1f1ccc74861` で、rootの `--check` はbyte一致した。146単品base＋Earth複合10＋拒否形14＋58境界3、計173ケースで、実 `A.p6` decoderと `ks.k` のattackByGuardian受信branchを同期Future shellで最後まで動かし、実main/response append・ATK/属性/MP算式・`gf2/iI` attack表示を照合する。58のsource MP0/1/17/99も含む。

受信branchはgeneric手札消費・MC費用・保持処理を呼ばず、58だけsource表示MPを読み2倍preview→MP0へ変える。これは描画receiverの事実で、権威的serverの所有・費用・retentionを証明しない。DOM/widget/animation/text/Futureはstub、アプリ起動・通信・selector/効果/乱数scheduleは実行しない。実MC baseのcategoryはmiracles、実weapon baseはweaponsのままであり、guardian画像を理由にguardians categoryへ変えない。反応適合もその実category/属性に従う。

拒否例はMoon[210,231]/[210,232]が実appendで不適合、Moon[218,204]とEarth[6,210]はclient append可でもadapter全列domain外、19モデル列はappend可でもlocal18上限外である。source displayが受信できる広い配列を全てadapterへ許す設計ではない。

新native6群を含む126群のRelease/SAN単体が通過した。新[payload bench source](src/official_guardian_payload_bench.cpp) を `godfield_guardian_payload_bench` として追加し、CTest `official_guardian_payload_benchmark` に登録した。`./build/godfield_guardian_payload_bench 456 9 42 2` のsmokeは146 bases＋6 Earth複合の152列をcoverし、456 scenes/1,157 decisions/701 responses、146 models/152 columns、departure92 rolls/12、chance50 rolls/32 hits/25 forced、174 redraws、checksum `14485308570291186513` を記録した。これは外部指定payloadのscene試行であり、自動一般selectorの分布評価ではない。大規模回帰・bridge・SAN/full suite結果は次の確定欄へ記録する。v32以前の実測を保持し、通常canonical42自動selectorと配布プールの回帰も別に確認する。広い一般selector・207/現象・悪魔・終末が完成したという主張はしない。

native126群のRelease/SAN単体が通過し、Python87件が通過した。Pythonはprimary report全173ケース（159受理・14拒否）も実行器/公開投影へ対照した。CAPIの固定query384件も維持した。Release全CTest132/132件（46.21秒）は通過済みで、SAN全suiteの最終結果は次の確定欄へ記録する。

| 一般payload・seed42 | 外部試行数 | decisions / responses | models / columns | terminal / cutoff | checksum |
| --- | ---: | ---: | ---: | ---: | --- |
| 2人 | 100,000 | 249,171 / 149,171 | 146 / 152 | 3,325 / 0 | `7340058501784454053` |
| 9人 | 10,000 | 24,898 / 14,898 | 146 / 152 | 0 / 0 | `6161355456919445122` |

一般payloadの2人/9人はrolls10,963/1,096・hits6,273/616・forced5,482/547・redraw38,902/3,906、departure19,419/1,945 rollsと1,986/223 departures。これらは外部選択した固定scene/モデル列の試行であり、一般Earth/Moon自動selectorの分布ではない。

通常対局・canonical guardian・一般payloadの各2人数、計6組の大規模1worker/4workersで、worker数・時間・速度を除く全fieldが一致した。通常とcanonicalのgameplay fieldはruleset/新guardian_payload_ruleを除きv32と完全一致し、次のchecksumを維持した。

| v33のv32回帰 | 2人100,000 | 9人10,000 |
| --- | --- | --- |
| 通常対局checksum | `13564747736715185154` | `17158162465123773162` |
| canonical外部guardian checksum | `15242932842311417291` | `678603818888237652` |

Release/SANの通常各1,000局、canonical420試行、payload456試行の計4組も全非時間fieldが一致した。通常2人/9人のchecksumは `1704475476168731728` / `13129530548541068452`、canonical smokeは `12692595741725906978`、payload smokeは `14485308570291186513`。配布pool236/499・canonical42自動modeを変えず、一般モデル列を外部入口だけで追加した更新である。速度改善比や公式server同等性は主張しない。

v33の最終全suiteはRelease **132/132件**（46.21秒）、Sanitizer **92/92件**（102.28秒）が通過した。native126群は全てmainへ各1回登録し、Python87件（Release）・CAPI/Python focused2件・固定CAPI照会384回も通過した。新しい `official_guardian_payload_benchmark` CTestは両buildで通過した。primary report全173ケースの受理/拒否と公開投影対照、大規模6組worker比較、Release/SAN4組比較は上記のとおり。これらは外部指定payloadと既存機能のローカル規約を検証した結果であり、公式の一般Earth/Moon選出分布・混合列・207/現象・悪魔・終末の完全対応を証明するものではない。

## v34：外部指定のdeterministic global現象5種

`official-execution-subset-v34` は通常配布236モデル/重み499とcanonical42自動guardianを変えず、現象287/288/290/294/296の外部primitiveを追加する。Snapshot34は旧1〜33を拒否し、公開viewは `official-execution-view-v32`。既存Match ABI2へ24byteの追加構造体/関数とPythonの `PhenomenonAction` を加え、通常command/opcode/phase、catalog/state ABI1、official-v2 pinsは維持する。207をgiftプールへ加えたり、自動現象の抽選を実装したりする更新ではない。旧37profile、各版の実測、v10/v16訂正履歴を保存し、学習器を追加しない。

### 一次根拠とatomic効果

公開clientは受信済みglobalイベントをalive playerのpane列へ適用する。ここで287は通常の病気再付与/自然悪化とは異なり、受信したFeverへ病気を直接replaceする。現clientの `dz` が旧病気を取り除き指定病気へ置き換えることは、既存のローカルonDamage/病気再付与で使うmax/次段階規約をこのglobal primitiveへ流用する根拠にはならない。

| 外部model | 今回の効果 |
| --- | --- |
| 287 夕焼け | 全aliveの既存病気だけをFeverへ直接置換。Cold/Hell/HeavenからもFeverになり、他curseとHPを維持。Heavenfall・208・112を発動しない |
| 288 濃霧 | 全aliveへFogをidempotentに付与。病気/Dream/Flash/DarkCloud/HPを維持 |
| 290 竜巻 | 全aliveのHPを直接1へ。damageではなく、counter・guardian退去を生成しない |
| 294 金山 | 全alive CP合計を捕捉→全alive CPを0へ→明示recipientへ合計を正加算して99cap。recipient自身の元CPも一度だけ合計し、MP/HPは変えない |
| 296 日食 | 全aliveのguardianを明示配列で置換。同じtypeも可。通常のguardian_summonsは増やさず、phenomenon理由の変化を記録 |

いずれもstableなalive入力へ一括適用し、この5種のbody自体からHP0/死亡を起こさない。per-target resolve/死亡確定を繰り返す処理にせず、全体をatomicに完了する。通常の病気付与・被害・召喚との違いはこのprimitiveの契約である。serverが207からどの現象をどの順で発生させるかを検証したという意味ではない。

### 型付き外部入力と進行

native `PhenomenonAction{model,recipient?,guardians[]}` はsourceを持たず、main境界で1decisionとして実行する。cardのguardian family metadataをsource playerや所有条件へ変換しない。294だけ既知alive recipientを必須とし、296だけ開始時aliveの入力seat順にちょうど同数のguardian1〜10を要求する。296は同type再指定可だがnone不可。その他の不要fieldは空にし、未知model・余剰/不足配列・不正enum・不要recipient等はatomic拒否する。型付き入力は特権外部制御用で、通常playerの選択commandではない。

乱数・MP/CP費用・source手札消費・操作gift・ordinary owner病気tick・GF進行・自動guardian followupを起こさない。全global bodyの適用後にdecision cutoffを判定し、未打切りなら同ownerのmainを維持する。cutoff時も適用済み結果は保持する。5種は直後に完了するため、新たなresponse/購入phaseやpending phenomenon batchを作らず、後続deathFIFOも生成しない。外部選択の配列を、uniformな種類抽選や自動scheduleだと扱わない。

### guardian履歴・privacy・restore

公開 `last_phenomenon` は `{model,recipient}` だけで、296の割当guardian配列や294のCP totalを追加しない。296の結果は `GuardianChangeReason::phenomenon`（value2）/JSON reason `phenomenon` として既存 `last_guardian_changes` に最大9件記録する。変更event時のFog非公開seat maskを保存し、viewerには既存のfilterを通して投影する。現在のboard statusも既存Fog可視性に従う。後からFog解除・死亡・終局が起きても、当時hiddenだった履歴entryの件数/typeを遡って公開しない。

private累計 `phenomenon_actions` は観測へ出さず、以降のnormal actionの `ActionBatch` anchorへ引き継いでreplayを比較する。guardian変化logは `9*decisions` 上限とし、reason validatorは新value2を受理する。通常の召喚累計に296を加算して辻褄を合わせない。source不在の即時効果なので、guardian/payloadのpending variantを無理に流用せず、通常完成sceneの検査と次actionのreplayで整合を保つ。

### 未対応範囲と検証

207そのものと自動現象抽選、289の混乱、291/292の攻撃、293の個人回復、295のshuffleは今回の入口に含めない。簡単な5種だけへ公式の10種分布を黙って再正規化するものでもない。悪魔240〜244、終末の開始/ささげる/25%悪魔/gift連鎖、Earth/Moonの一般selector、公式Dreamの全分布も残件である。旧Flashの木星ILLUSION/金星全員99/HP60等を現行へ持ち込まない。

native `phenomenon_legal` / `phenomenon_step`、C `gfo_match_phenomenon_legal` / `gfo_match_phenomenon_step`、Python `PhenomenonAction` / `Match.phenomenon_legal` / `Match.phenomenon_step` を追加する。C actionは24byteで、既存command構造体の幅は変えない。static supported queryは5モデルだけを返し、completed action全体のfield/現在phase/recipient/配列検査はlegalへ渡す。一般player samplerがこの特権global入口を選ぶ契約ではない。

`PYTHONPATH=python` で動かす小例です。これは明示した外部global primitiveであり、207の使用や自動現象抽選ではありません。

```python
from official_state import OfficialState
from official_match import Match, PhenomenonAction

raw = {"gf": 0, "players": [
    {"id": 0, "hp": 40, "mp": 10, "cp": 30, "items": []},
    {"id": 7, "hp": 40, "mp": 10, "cp": 40, "items": []},
    {"id": 22, "hp": 40, "mp": 10, "cp": 50, "items": []},
]}
with OfficialState(raw) as scene, Match() as game:
    game.load_scene(scene, owner=0, seed=42)
    game.phenomenon_step(PhenomenonAction(294, recipient=7))
    board = game.observe()["board"]
    print([p["status"]["cp"] for p in board["players"]], game.status.owner, board["gf"])
    # [0, 99, 0] 0 0
```


restoreは保存したquiescent eventの型・phase・guardianのevent-time Fog maskと、各modelの完成postconditionを検査する。元CP合計や旧病気など、既に上書きした全履歴が実際の公式対局で成立したことまで証明しない。後続normal pending actionでは `phenomenon_actions` のprefix累計をanchorから再生/比較する。即時5primitiveに新pending phenomenon variantやdeathFIFOを追加する設計ではない。

公開exact規約は次の文字列とする。

```text
experimental:external-source-free-global-primitives287-288-290-294-296-only;main-boundary-one-atomic-decision;launch-alive-in-input-order-including-all-teams;287-replace-existing-disease-with-fever-preserve-other-curses-no-upgrade-or-heavenfall;288-idempotent-fog;290-direct-hp1-not-damage;294-sum-alive-cp-then-zero-all-before-explicit-alive-recipient-cap99;296-exact-alive-count-explicit-guardian1-to10-same-type-allowed;no-selector-or-rng;no-inventory-mp-fee-gift-owner-tick-gf-or-auto-followup;body-before-decision-cutoff;unchanged-owner;guardian-changes-event-time-fog-filtered;public-model-recipient-only-no-assignment-array-or-cp-total;207-and-other-phenomena-unsupported
```

[現象一次oracle](scripts/official_phenomenon_oracle.cjs) / [保存report](data/official-v2/phenomenon-client-v34.json) を追加し、rootの `--check` はbyte一致した。report SHA256は `67532814e73876a1bddcc282e3336610b6f0ff2078113f6783717bdb8b9820ed`。実event decoder・global受信branch・alive roster・curse/resource/guardian操作・Fog fanoutを実行し、298event＋8decoder＋単独 `sd0(null)` 1helperケースを記録する。DOM/widget/animation/sound/Futureのscaffoldingはstubで、backend・アプリ起動・自動selectorは動かさない。

296のraw guardian配列にnull/unknown/nonstringがある場合、実 `A.to` はそれらをfilterして残った既知typeを詰める。nullは解除slotではなく、単独 `sd0(null)` のclearとは別である。不足配列は先行player変更後にthrowし、余剰配列は残る。今回のtyped adapterはそれより強い制約として、exact alive count・既知1〜10・none無しを事前に検査してatomic拒否する。294のalive recipient制約もreceiver単体より厳格である。rendererが内部値を書き換えることだけでは、他viewerへraw CP合計や割当配列を公開する根拠にならない。

native新6群を含む132群を全てmainへ各1回登録し、Release/SAN単体が通過した。Python92件はprimary298eventを265適用・11入力拒否・22 scene範囲外（20自然終局・2 alive HP0）へ分類して対照した。clientのalive判定はdead flagなのでHP0だけでは除外しないが、実行器はstable/quiescent sceneでHP0&&!deadを受理しない。これは即時5primitiveに死亡queueを加える理由にはしない。

新[接合bench source](src/official_phenomenon_bench.cpp) を `godfield_phenomenon_bench` として追加し、CTest `official_phenomenon_benchmark` に `500 9 42 2` を登録した。各sceneは外部primitive一回の無RNG/同Config/token・GF0/dec1/同ownerと効果postconditionを検算し、その後にordinary prayerを一回、response完了まで進めて毎step snapshotをrestore監査する。初回primitiveと最終stateは別hashにし、`decisions=2*scenes+responses`、phenomenon prefix累計、後続病気tick/auto guardianとの接合を検査する。最初のprimitiveがGFを変えないことと、後続ordinary action終了時にGFが進み得ることを分ける。CPtotal等の統計とfullstate hashは特権bench用で、player観測には追加しない。

新benchは自動現象selectorの評価ではない。smoke/determinism・大規模回帰・最終全suiteの結果は次のとおり。v33以前の検証と対局checksumは履歴として維持する。

v34の最終全suiteはRelease **133/133件**（41.66秒）、ASan/UBSan **93/93件**（98.60秒）が通過した。native132群を全てmainへ各1回登録し、Release/SAN単体が通過した。Python92件・primary298eventの265適用/11拒否/22scene範囲外対照、CAPI/Python focused2件・固定query384件も確認した。suite経過時間をエンジンの性能比較には使わない。

通常/canonical/payload/phenomenonの各2人数、計8組の大規模1worker/4workersで、worker数・時間・速度を除く全fieldが一致した。通常/canonical/payloadの6組のgameplay fieldは、rulesetと新phenomenon_ruleを除きv33と同値で、過去のchecksumを維持した。公開schemaの追加と、対局動作の不変は区別する。

| 接合phenomenon・seed42 | scenes | decisions / responses | terminal / cutoff | primitive checksum | final checksum |
| --- | ---: | ---: | ---: | --- | --- |
| 2人 | 100,000 | 221,884 / 21,884 | 18,231 / 0 | `2238725749637457136` | `8443246086499383498` |
| 9人 | 10,000 | 37,431 / 17,431 | 0 / 0 | `3655171676226107657` | `2228729406112071100` |

これらの終局は外部primitive後のordinary prayer/病気tick/auto guardian・応答により起きたもので、5primitive自身が死亡・tick・GF進行を起こした結果ではない。初回の無RNG/GF0/同ownerと、後続ordinary actionの進行を別境界として検査した。fullfield hashはprivate hand/token/counters/historyも含む特権bench用で、player viewのhashではない。

Release/SANの通常1,000局×2人数・canonical420試行・payload456試行・phenomenon500試行の計5組も全非時間fieldが一致した。phenomenon500・9人は5モデル各100回、dead入力scene30、1,895decisions/895responses、disease ticks500、guardian opportunities3,245/auto834、departure379rolls/33。primitive checksumは `1567088900019069442`、finalは `5095387203829390652`。外部指定sceneの接合検証を自動現象selectorや公式server全対局の検証へ読み替えず、速度改善比は主張しない。


## v35：sourceなしの外部指定現象3種と応答

`official-execution-subset-v35` は、外部 `PhenomenonAction` に291/292/293を追加する。通常配布236モデル/重み499、canonical42自動guardian、typed Earth/Moon payloadの対応範囲は変更しない。Snapshot35は旧1〜34を拒否し、公開viewは `official-execution-view-v33`。既存24byteの現象action、Match ABI2の幅・通常opcode/phase、catalog/state ABI1、official-v2 pinsを維持する。207からの自動現象抽選を実装する更新ではない。旧37profile、過去の実測とv10/v16訂正履歴を保存し、学習器を追加しない。

### sourceなし反射取消の最小例

`PYTHONPATH=python python3` で次を実行できる。外部からモデルと対象を指定する例であり、自動現象抽選ではない。ID0は実在するownerだが、sourceなしの291を190で返す先にはならない。

```python
from official_state import OfficialState
from official_match import Match, PhenomenonAction, Command

raw = {"gf": 0, "players": [
    {"id": 0, "hp": 40, "mp": 10, "cp": 20, "items": []},
    {"id": 7, "hp": 99, "mp": 10, "cp": 20,
     "items": [{"id": 10, "modelId": 190}]},
]}
with OfficialState(raw) as scene, Match() as game:
    game.load_scene(scene, owner=0, seed=42)
    game.phenomenon_step(PhenomenonAction(291, recipient=7))
    pending = game.observe()["pending"]
    print(pending["origin"], pending["source"], pending["power"])
    # None None 50
    game.step(Command(7, "defend", items=(10,)))
    board = game.observe()["board"]
    print(game.status.phase, game.status.owner, board["gf"],
          board["players"][1]["status"]["hp"])
    # main 0 0 99
```

### 一次根拠とローカル規約の境界

公式台帳では291がphenomena/Light ATK50/attackSomebody、292がphenomena/Dark ATK30/75%、293がphenomena/boostHP50である。293のNeptune metadataを発生源プレイヤーや守護神召喚として扱わない。公開clientの反応predicateは190のreflectAnythingをcategoryにかかわらず認める一方、受信reflect handlerは旧source paneへ返す処理であり、sourceなしの帰着を決めない。counterAttack handlerも指定した防具indexの演出で、sourceなし攻撃に対する反撃の生成・対象を証明しない。

以下はこれらを閉じた実行範囲で動かすための**実験規約**である。3種とも明示したalive recipientを1人だけ対象にし、Fogによる再選択・全敵化・自動selectorを使わない。293も即時回復へ省略せず、対象をactorとするresponseを作り、passまたは190を待つ。291のLightには普通DEFを使えず、189のfilterで無属性へ消去した後は適合する普通防御を使える。292は75%を対象活性化時に抽選し、DarkCloudがあれば無drawで必中にする。hitだけresponseを開始し、redirectによる再抽選は無い。293は最終bodyで対象1人のHPを50増やして99にclipするもので、全員回復ではない。

### 発生源の欠落と反応・死亡

待機payloadには `environmental=true` を持たせ、origin/sourceが存在しないことをこのflagで定義する。内部の未使用数値0をプレイヤーとして参照しない。通常payloadの `environmental=false` ではPlayerID0は引き続き有効である。受信DTOのplayerId欠損を0へdefaultするclient decoderは、PlayerID0を「発生源なし」のsentinelと定める根拠ではない。公開pendingは `environmental=true` と `origin=null` / `source=null` を返し、ownerを代理の発生源へ置き換えない。

sourceなしpayloadへの190は通常どおり品を消費して応答者の補充を予約した後、返却先なしとしてpayloadを取り消す。これは公式serverで確認した反射規約ではない。適法に選択・消費した181/182/183/184/185/186/188は普通DEFへの寄与を通常どおり扱うが、sourceを宛先とする派生反撃だけを予約しない。187のself MP反撃は実在する応答者をsource/targetに持つ通常payloadとして予約できる。112の死亡時攻撃にも実在する死者sourceがあり、environmentalにはしない。

被害・反撃の基準は既存bodyのtarget実HP損失を使い、208救済・112予約・自然終局優先・decision cutoff・全action後のdeferred giftsを共用する。外部起動は1decision、各responseは別decision。発生源の品消費・MP費用・操作giftは無く、ordinary ownerの病気tick・GF進行・自動guardian followupも起こさない。全work完了後は元ownerが生存なら同ownerへ戻り、死亡していれば次aliveへ移るがGFは増やさない。world由来の初期対象をowner死亡だけで取り消さない。

### pending replayと公開範囲

ActionBatchに現象actionの起動前anchorと全応答journalを保存し、同じfactory/executorから抽選・消費・補充・反撃・死後workを完全replayする。新3種はpending・死亡・自然終局・178redraw・guardian departureを伴い得る。完了履歴のrecipientは、その後deadでもよい。v34のglobal5種287/288/290/294/296には、即時・死亡なしの強いquiescent postconditionを引き続き適用し、同じ現象入口でも検査を混同しない。未来FIFO・journal・乱数・tokenや内部累計は公開viewへ追加しない。任意の完了raw stateの全過去到達履歴を証明するものではない。

残件は207からの抽選、289の混乱、295のshuffle、Earth/Moonの一般selector、悪魔240〜244と終末、公式Dreamの全分布・実体不適合入力等である。3種の反射先・反撃抑制・対象選択とscheduleを公式serverの仕様として断定しない。

公開 `phenomenon_rule` はheaderと同じ次の文字列である。これはローカル実験規約を明示するmetadataであり、server同値性の宣言ではない。

```text
experimental:external-source-free-phenomena287-288-290-291-292-293-294-296;main-boundary-one-injection-decision;global5-immediate-atomic-no-rng-unchanged-from-v34;targeted291-light50-292-dark30-at75-percent-293-hpplus50-cap99;explicit-single-alive-recipient291-to294;no-target-selector-wide-or-fog-reselection;targeted3-always-response-after-single-target-chance-and-darkcloud;environmental-frame-origin-source-absent-not-player0;190-consume-and-reserve-response-gift-then-cancel-without-return-recipient;source-directed-counters-not-created-without-source;187-self-mp-counter-and112-death-use-real-player-sources;ordinary-defense-filter-revival-death-redraw-and-deferred-gifts;targeted3-original-action-anchor-and-full-response-replay;no-owner-tick-gf-auto-source-items-mp-fee-or-operation-gift;resume-owner-or-next-alive-with-natural-before-cutoff;287-direct-fever-replacement-no-upgrade-or-heavenfall;288-idempotent-fog;290-direct-hp1-not-damage;294-sum-alive-cp-then-zero-all-before-recipient-cap99;296-exact-alive-count-explicit-guardian1-to10;guardian-changes-event-time-fog-filtered;public-model-recipient-only-no-assignment-array-or-cp-total;207-selector289-confusion295-shuffle-unsupported
```

### v35の検証

[targeted現象一次oracle](scripts/official_targeted_phenomenon_oracle.cjs) / [保存report](data/official-v2/targeted-phenomenon-client-v35.json) は101ケース（18frame・69numeric・6reflect・8decoder）を記録し、rootの `--check` はbyte一致した。report SHA256は `9c7a8f00b18d35f8121f48a231719151dea9e16ecdae4f52a1b27447c616bca1`。実decoder、frame/summary、HP算術、反応append、受信HP/通常被害/闇被害/反射branchをvisual/widget・同期Futureのshellで動かす照合であり、serverのselector・sourceなしの返却先・counter生成policyは証明しない。sourceNullableの注記とDTOのplayerId default0を区別し、actual UI appendの適合と、ローカル190取消/反撃抑制を混同しない。

新[専用bench](src/official_targeted_phenomenon_bench.cpp) を `godfield_targeted_phenomenon_bench` として追加し、CTest `official_targeted_phenomenon_benchmark` に登録した。`./build/godfield_targeted_phenomenon_bench 600 9 42 2` は3モデル全てをcoverし、600scenes / 1,246decisions / 646responses、checksum `3353421385653118848` を記録した。外部起動からそのresponse/FIFO完了までを進め、ordinary mainへは進まない。全stepでsnapshotを復元し、最初のworld frameのenvironmental/source不在、派生frameの実source、特にPlayerID0を検査する。world bodyで増えたcounter数が選択187数を超えないことも必須assertにしている。

smokeではworld応答580、190によるworld取消251、source宛counter品選択184copy、実source frame観測66（source0は6）を記録した。MP187を含むcounter43生成/43開始、死亡時112は1copy/8frame予約/7開始、85revivals、94redrawsを含む。frame観測は各待機境界の観測回数であり、ユニークなpayload数とは異なる。GF17・tickなし・自動guardian機会なしを検査し、`decisions=scenes+responses` とする。checksumは特権snapshotの全fieldを使い、公開viewのhashではない。

v35の最終全suiteはRelease **134/134件**（35.99秒）、ASan/UBSan **94/94件**（61.67秒）が通過した。native138群は全てmainへ各1回登録し、Release/SAN単体が通過した。Python97件・CAPI/Python focused2件・固定query384件と、旧173/298ケースの対照も継続して通過した。新しい実client対照では6frame×236対応モデルの初期投影1,416回に加え、empty-prefix1,416回/filter-prefix944回、合計2,360回のlegality照会が一致した。この一致はUIのframe/選択predicateの範囲であり、sourceなし190取消やcounter生成policyのserver同値性を証明しない。suite経過時間をエンジン性能比較に使わない。

通常/canonical/payload/global phenomenon/targeted phenomenonの各2人数、計10組の大規模1worker/4workersで、worker数・時間・速度を除く全fieldが一致した。通常/canonical/payloadの6組のgameplay fieldはrulesetとphenomenon_ruleを除きv34と同値。global phenomenonの2組もその他のgameplay fieldは同値だが、primitive/final checksumはSnapshot.version35を含むため異なる。v34のhashをv35のhashへ読み替えず、版番号を含むhash差と状態遷移差を区別する。

| targeted phenomenon・seed42 | scenes | decisions / responses | terminal / cutoff | checksum |
| --- | ---: | ---: | ---: | --- |
| 2人 | 100,000 | 206,605 / 106,605 | 11,062 / 0 | `1328364666406328546` |
| 9人 | 10,000 | 20,777 / 10,777 | 0 / 0 | `3485014558350256781` |

Release/SANの通常1,000局×2人数・canonical420試行・payload456試行・global phenomenon500試行・targeted phenomenon600試行の計6組も、全非時間fieldが一致した。targeted smokeは上記の1,246decisions/646responses、world応答580、190取消251、実source frame観測66/source0観測6、counter43、85revivals、94redraws、guardian departure230rolls/22を含み、外部処理中の自動guardian機会は0だった。モデルとrecipientは外部指定で、公式selectorの実装・分布評価ではない。速度改善比は主張しない。


## v36：磁気嵐の外部指定item permutation

`official-execution-subset-v36` は、295磁気嵐のために `ItemShuffleAction{order: vector<ItemSource{owner, token}>}` の専用外部入口を追加する。通常配布236モデル/重み499、canonical42自動guardian、typed Earth/Moon payload、既存のgeneric現象8種は変更しない。Snapshot36は旧1〜35を拒否し、公開viewは `official-execution-view-v34`。既存Match ABI2と24byteの `PhenomenonAction`、その対応8モデルは維持し、295をgeneric actionへ混ぜない。新C入口は16byteのsource構造体と16byteのpointer/count actionを追加する。旧37profile、official-v2 pins、各版の実測とv10/v16訂正履歴を保持し、学習器を追加しない。

### 公開clientの事実と独自の分配契約

JA説明は「みんなの神器が無作為に入れ替わる」。公開 `replaceItems` handlerは指定playerのused表示paneをclearし、本人の手札だけを供給されたwire rowsで置換する。実 `fH.lI` はnonemptyの枠（used奇跡の枠も含む）を順に置き換え、補充用empty placeholderは保持し、token/model/fake/usedはrow値をそのまま使える。この表示能力は、磁気嵐のserverがusedを転送すること、各人枚数を保存すること、tokenやfakeをどう扱うか、候補・順序・均一分布・乱数消費を証明しない。

今回の外部入口は分配を抽選しない。main境界だけで1decisionを使い、aliveの入力順、各人のraw inventory順でrecipient slotを固定し、各人の実物枚数を保存する。orderは全live copiesをsource pair `(owner, token)` で過不足なく1回ずつ指定する全単射である。token0も有効で、別ownerの同じtokenは別copyとして指定できる。fresh viewのused-last/face sort順を物理slot順に使わない。deadはsourceにもrecipientにも含めず、そのinventoryを完全に保存する。この明示permutationは特権外部入力であり、公式selectorや通常playerのcommandではない。

### instance・Dream・tokenの扱い

modelとusedを保存しつつ、全live copyへ `next_token` から連続するfreshなglobal tokenを与える。identity permutationでも再採番する。これにより別owner間の同tokenを同じrecipientへ移す場合も衝突しない。live inventory総数0ならtokenを必要とせず、next_tokenが枯渇済みでも受理できる。非emptyで必要なfresh ID予算が無ければ、状態・RNG・意思決定数・公開eventを一切変えず失敗する。

既存compatible fakeはrecipientがDreamなら保存し、非Dreamならprivateに解除する。解除時にpublic revealを発行せず、移送modelや旧faceを他viewerへ知らせない。fake無しの品をDream本人へ渡しても新しいmaskを抽選しない。used奇跡はusedのままで、再取得・費用支払・MP消費は起きない。災いそのものは移送しない。これは既存のclosed-state制約と整合させたローカル方針で、公式Dream/磁気嵐の完全再現ではない。

旧FlashのEARTH現象はpiecesを集めて新しいCommandPieceへ再分配し、保持magicsは別配列で対象外、illusionも保存しない。現行のused instance転送やcompatible fake保持の根拠としてその仕様を流用しない。

### 専用shuffle APIの最小例

`PYTHONPATH=python python3` で実行できる。通常のplayer commandや自動抽選ではなく、特権入力で全copyの移送順を指定する。入力の物理品には `modelId` を使う。別ownerのtoken0は別sourceで、used奇跡を移してもunused化や費用支払はしない。

```python
from official_state import OfficialState
from official_match import Match, ItemSource, ItemShuffleAction

raw = {"gf": 0, "players": [
    {"id": 0, "hp": 40, "mp": 10, "cp": 20,
     "items": [{"id": 0, "modelId": 6}]},
    {"id": 7, "hp": 40, "mp": 10, "cp": 20,
     "items": [{"id": 0, "modelId": 235, "used": True}]},
]}
with OfficialState(raw) as scene, Match() as game:
    game.load_scene(scene, owner=0, seed=42)
    game.item_shuffle_step(ItemShuffleAction((ItemSource(7, 0), ItemSource(0, 0))))
    for viewer in (0, 7):
        hand = game.observe(viewer)["board"]["hand"]
        print([(item["token"], item["face"], item["used"]) for item in hand])
    # [(1, 235, True)]
    # [(2, 6, False)]
    print(game.status.phase, game.status.owner, game.observe()["board"]["gf"])
    # main 0 0
```

### 原子的完了・公開と復元

consume/gift/redraw/deliveryのhelperを使わず、移送品の208/112/178等の能力も発動しない。HP/MP/CP、災い、guardian、RNG、補充予定を変更せず、sourceの品消費・MC費用・普通ownerのtick・GF・auto guardianを起こさない。bodyを一括適用した後にdecision cutoffを判定し、ownerを維持する。受取枚数は保存するのでoverflowも起こさない。

公開は `last_phenomenon={model:295, recipient:null}` だけで、order、人数別枚数、移送したmodel/token、起動anchorを追加公開しない。新しい `item_shuffle_legal` / step相当のmanifest照会は特権APIであり、player viewだけから答える合法手照会とは異なる。通常samplerへshuffle操作を追加しない。移送後も通常観測・samplerは本人のvisible handと既存の公開used表示に従い、私的permutationをpolicyへ渡さない。

privateの `last_item_shuffle` は `ItemShuffleRecord{ActionBatch anchor, ItemShuffleAction action}` として非再帰の起動証拠を保持し、元world・全累計・RNG・next_tokenを通常pendingと共通の `snapshot_from_anchor` で復元し、同executorを再生して全snapshotを照合する。既存pending batchとは排他で、次のaccepted main/externalでclearし、失敗時は保持する。後続normal actionは移送後の状態をanchorに使う。過去の全公式対局履歴まで証明するものではない。旧generic global5種の強いpostcondition、targeted3種の応答journalと、この即時ownership移転の証拠を混同しない。

外部generic8種とtyped295を合わせた9現象効果を扱うが、207からの現象抽選、289混乱、Earth/Moon一般selector、悪魔・終末、公式Dream全分布/実体不適合入力は残件である。対応9種だけへ公式の現象分布を再正規化せず、明示permutationの順序・枚数・used転送・retoken・fake方針をserver確定仕様と呼ばない。

v36の公開exact規約はheaderと同じ次の2文字列である。`phenomenon_rule` の末尾も295専用APIを要求し、generic `PhenomenonAction` が295を受理するという意味にはしない。v35節の旧文字列は当時の履歴として保持する。

`item_shuffle_rule`:

```text
experimental:external295-fixed-slot-item-permutation-only;main-boundary-one-phenomenon-decision;alive-input-order-then-raw-inventory-order;source-owner-token-pairs-exact-cover;retain-each-alive-hand-count-and-copy-model-used;dead-inventories-unchanged;all-alive-copies-get-contiguous-fresh-global-tokens-including-identity;existing-compatible-fake-preserved-only-for-dream-recipient-otherwise-private-clear;unmasked-copy-no-new-dream-mask;no-selector-or-rng-consumption-redraw-reveal-gift-cost-ability-trigger;no-resource-curse-guardian-owner-tick-gf-or-auto-changes;body-before-cutoff;zero-copies-valid-even-at-token-exhaustion;private-nonrecursive-pre-action-anchor-and-whole-snapshot-replay;public-phenomenon295-only-no-order-origin-model-count-or-token-manifest;generic-phenomenon-action295-unsupported-use-dedicated-api;server-distribution-slot-used-token-fake-policy-unverified
```

`phenomenon_rule`:

```text
experimental:external-source-free-phenomena287-288-290-291-292-293-294-296;main-boundary-one-injection-decision;global5-immediate-atomic-no-rng-unchanged-from-v34;targeted291-light50-292-dark30-at75-percent-293-hpplus50-cap99;explicit-single-alive-recipient291-to294;no-target-selector-wide-or-fog-reselection;targeted3-always-response-after-single-target-chance-and-darkcloud;environmental-frame-origin-source-absent-not-player0;190-consume-and-reserve-response-gift-then-cancel-without-return-recipient;source-directed-counters-not-created-without-source;187-self-mp-counter-and112-death-use-real-player-sources;ordinary-defense-filter-revival-death-redraw-and-deferred-gifts;targeted3-original-action-anchor-and-full-response-replay;no-owner-tick-gf-auto-source-items-mp-fee-or-operation-gift;resume-owner-or-next-alive-with-natural-before-cutoff;287-direct-fever-replacement-no-upgrade-or-heavenfall;288-idempotent-fog;290-direct-hp1-not-damage;294-sum-alive-cp-then-zero-all-before-recipient-cap99;296-exact-alive-count-explicit-guardian1-to10;guardian-changes-event-time-fog-filtered;public-model-recipient-only-no-assignment-array-or-cp-total;207-selector289-confusion-unsupported;295-requires-dedicated-item-shuffle-api
```

### v36の検証

[item shuffle一次oracle](scripts/official_item_shuffle_oracle.cjs) / [保存report](data/official-v2/item-shuffle-client-v36.json) は249ケース、792,139bytes、SHA256 `192af0ec5dfbcc81979683c0ba5eb05e888963ab9de8644920a087a073b0aed6` を記録する。内訳はdecoder9・通常inventory152・短いinventory1・不正model3・受信branch36・fresh projection48。実event/item decoder、replaceItems受信branch、nonempty置換loop、used-pane clear、供給model/fake/token/usedのwidget投影とfresh-stateの手札/used表示/face sortを動かす。DOM/visual/widget・同期Future等のshellを使い、アプリ起動・ネットワーク・server selectorは実行しない。

実行結果では、incremental受信は本人の手札だけを置換し、全viewerの該当player used-paneをclearする。そこにはdead/Fog除外や全aliveへ分配するloopが無い。一方fresh snapshotは本人の手札を供給個数で再構築し、供給used flagから各playerの公開used-model表示を作る。両経路の違いを295のserver fanoutや保存則の証明に読み替えない。供給rowの重複/0 token・fake・usedをrendererが扱えることも、Matchで不正なfakeや曖昧なmanifestを受理する根拠ではない。短い置換listは途中の表示変更後にthrowし、余剰rowは未使用となるため、typed APIのexact cover先行検査とtransactional rollbackは独立したローカル契約である。

新[shuffle専用bench](src/official_item_shuffle_bench.cpp) を `godfield_item_shuffle_bench` として追加し、CTest `official_item_shuffle_benchmark` は `600 9 42 2` を指定する。私的なbefore world・manifest・after worldと全snapshot restoreを照合し、明示shuffleの後に通常samplerの1主動作と全response/purchase決定を進める設計である。world/slot・manifestのhashを「全snapshot field hash」とは呼ばず、非公開のItemShuffleRecord/journalを含む一致はrestore比較で別に検査する。分配の確率分布や公式selectorの証拠ではない。最終検証値は次の確定欄へ記録する。

v36の最終全suiteはRelease **135/135件**（23.82秒）、ASan/UBSan **95/95件**（native1件2.28秒、残94件56.28秒の分割実行）が通過した。native144群は全てmainへ各1回登録し、Release/SAN単体も通過した。Python101件、C API/Python、固定query384件、旧173/298ケースと実clientの2,360回のlegality照会も継続して通過した。新6群はowner別token0、used保持と再使用、Dream境界、最大162copy、dead/empty、fresh IDとupdate枯渇時rollback、18種のproof改竄、反復shuffle後の通常対局再生を検査する。READMEの最小Python例も実行し、移送後のfresh token1/2・used保持・owner/GF不変を確認した。suite時間を性能比較に使わない。

通常/canonical/payload/global phenomenon/targeted phenomenon/shuffleの各2人数、計12組で2人100,000局またはscene・9人10,000局またはsceneを1worker/4workersで実行し、worker数・時間・速度以外の全fieldが一致した。既存の通常/canonical/payloadはv35のgameplay field・checksumを維持した。global/targeted現象はSnapshot.version36をhashへ含むためchecksumは変わるが、その他のgameplay fieldはv35と同値だった。Release/SANの通常1,000局×2人数・canonical420・payload456・global500・targeted600・shuffle600の計7組も非時間fieldが一致した。

| 明示shuffle＋後続1主動作・seed42 | scenes | decisions / responses / purchase | 移送copy / 最大copy | primitive checksum | manifest checksum | 後続world checksum |
| --- | ---: | ---: | ---: | --- | --- | --- |
| 2人 | 100,000 | 273,390 / 71,968 / 1,422 | 2,077,577 / 36 | `15674481000839696594` | `15731540149134808078` | `11018779129227397467` |
| 9人 | 10,000 | 48,722 / 28,117 / 605 | 928,747 / 162 | `8353509864660691491` | `421095288420525988` | `17742503789800449523` |

shuffle smoke600sceneは2,963decisions/1,719responses/44purchase、55,724copy（empty7、used5,029）、mask保存1,588/解除930を記録した。後続通常行動ではcounter138、死亡時frame75、revival54、redraw167、自動guardian1,000も通過し、world checksum `2901487151624391478` がRelease/SANで一致した。shuffle自体はRNG/GF/tick/autoを変えず、これらはその後の普通の主動作に由来する。manifest/world hashと全snapshot restoreを区別し、公式のshuffle分布・server schedulingの証拠とはしない。


## v37：混乱289と外部指定の混乱主動作

`official-execution-subset-v37` はgeneric現象へ289のconfuseEverybodyを加え、9 genericモデルと専用295を合わせて10現象効果を外部入力で扱う。通常配布236モデル/重み499、canonical42自動guardianとtyped Earth/Moon payloadの範囲は変更しない。Snapshot37は旧1〜36を拒否し、公開viewは `official-execution-view-v35`。Match ABI2と既存Commandの幅/opcode/phaseを維持し、同じCommandを受ける外部 `confusion_legal` / `confusion_step` を追加する。207の自動現象抽選や混乱の公式自動選択分布を実装する更新ではない。旧37profile、official-v2 pins、各版の実測・v10/v16訂正履歴を保存し、学習器を追加しない。

### 現行clientで確定するcountと表示

公式JAは「みんなが三回ずつ勝手に行動する」、ENはlose control for 3 turnsと説明する。実clientのconfuseEverybody受信branchはalive全員のcountへ3を加算し、再付与を3へclampしない。useAttackItemsの末尾は正countだけを1減らし、itemsが空でも同じである。`advanceGF` 受信時は正countなら御乱心の演出を行う。ここにstartTurnというwire actionを仮定しない。死亡表示は本人countを0にし、endGameはalive全員を0にする。

混乱は病気/Fog/Dream/閃光/暗雲のcurse集合とは別の状態で、普通のmild/all cureはcountを解除しない。HP0から208で救済され、実死亡を経なければcountを保持する。公開viewは既存のconfused boolだけを返し、数値countや残り自動選択を公開しない。Fogの既存status pane非表示にも従う。

clientは受信した行動を表示するが、勝手に選ぶcommand・対象・候補の分布、responseや購入確認を誰が選ぶかは示さない。旧FlashのJUPITER/ILLUSIONは見かけの神器を変える別効果で、今回の混乱countや自動行動policyの根拠にしない。

### 選択分布を作らない外部control入口

mainでownerのcountが正の場合だけ、外部 `confusion_step(Command)` を受理する。同じ通常commandのshape・所有token・visible selection・MP等を検査し、受理した起動のbodyより前にcountを1減らす。祈り/捨てる/取引/両替にもuseAttackItems（emptyを含む）と補助eventを対応させて1減らすのは、明示的なローカルscheduleである。responseとpurchase決定は通常のstep/samplerで進み、countを減らさない。

混乱ownerのmainでは通常のlegal/step/samplerを拒否し、その拒否でpolicy RNGも変更せず、playerが自由に選んだ行動を自動選択と呼び替えない。外部driverがcommandを指定する特権入口であり、一様な乱数選択・完全な公式混乱AI・自動連続実行を実装したとは呼ばない。1回の外部指定は1decisionで、既存のbody/反撃/死後work/購入/補充・tick・guardian stageを通常主動作として進める。未来の混乱行動やpolicy RNGを公開しない。

### 外部指定の混乱主動作の最小例

`PYTHONPATH=python python3` で実行できる。289と、その後の祈りを外部で明示する例で、自動選択分布を再現するものではない。genericの対応9モデルと295専用APIは別である。

```python
from official_state import OfficialState
from official_match import Match, PhenomenonAction, Command

raw = {"gf": 0, "players": [
    {"id": 0, "hp": 40, "mp": 10, "cp": 20, "items": []},
    {"id": 7, "hp": 40, "mp": 10, "cp": 20,
     "items": [{"id": 10, "modelId": 6}]},
]}
with OfficialState(raw) as scene, Match() as game:
    game.load_scene(scene, owner=0, seed=42)
    game.phenomenon_step(PhenomenonAction(289))
    command = Command(0, "pray")
    print(len(game.supported_phenomenon_models), game.legal(command),
          game.confusion_legal(command))
    # 9 False True
    game.confusion_step(command)
    board = game.observe()["board"]
    print(game.status.phase, game.status.owner, board["gf"])
    # main 7 1
```

### bounded count・回復・再生

Matchのcountは0〜JS safe integer上限の非負整数で、dead sceneのcountは0に限定する。289はaliveへ3ずつ加算するため、1人でも加算が上限を超える場合はworld・RNG・意思決定数・eventを一切変更せず失敗する。289自体はglobalな即時外部効果で、乱数・通常ownerのtick/GF/auto・品消費・操作giftを起こさない。

混乱主動作のjournalは減算前のanchorと専用起動flagを保持し、減算・通常main・全response/purchaseを同executorからreplayして全snapshotを比較する。死亡で0、自然終局ではaliveを0へ更新するが、decision cutoffではendGameを仮定せず残countを保持する。通常cure、revival、外部guardian/payload/shuffle/globalの処理はcountを勝手に回復せず、それらに伴う実死亡/自然終局だけを既存の共通境界で扱う。

現在の範囲は混乱の付与と外部指定commandを進める基盤までである。自動選択分布、207のselector、Earth/Moon一般selector、公式Dream全分布・実体不適合入力、悪魔・終末は残件として維持する。既存の公式client表示とこのローカルcontrol契約を、権威的serverの全面再現へ読み替えない。

v37の公開exact規約はheaderと同じ次の2文字列である。過去版の文字列はその版の履歴として保持する。

`confusion_rule`:

```text
experimental:external-resolved-command-only-no-auto-selector;289-add3-to-every-alive-count-no-clamp;nonnegative-safe-wire-counts-dead-zero;main-positive-owner-only-dedicated-confusion-api;ordinary-main-and-sampler-reject-without-policy-rng-change;usual-visible-composition-mp-items-target-and-allocation-validation;one-count-debit-at-launch-before-body-and-cutoff;all-main-kinds-mapped-to-useAttackItems-including-empty-pray-and-auxiliary-discard-trade-exchange;pre-debit-anchor-and-private-confusion-marker-whole-pending-replay;responses-and-purchase-no-debit-and-use-normal-api;no-extra-selector-rng-or-operation-fee;ordinary-body-gifts-owner-tick-gf-auto-and-next-owner;revival-preserves-count-death-clears-natural-end-clears-alive-cutoff-preserves;curse-cures-and-unrelated-external-effects-preserve-count;public-confused-bool-only-with-existing-fog-dead-and-end-filter;server-command-generation-and-selection-distribution-unverified
```

`phenomenon_rule`:

```text
experimental:external-source-free-phenomena287-288-289-290-291-292-293-294-296;main-boundary-one-injection-decision;global5-unchanged-from-v34-plus289-alive-count-plus3-immediate-atomic-no-rng;targeted291-light50-292-dark30-at75-percent-293-hpplus50-cap99;explicit-single-alive-recipient291-to294;no-target-selector-wide-or-fog-reselection;targeted3-always-response-after-single-target-chance-and-darkcloud;environmental-frame-origin-source-absent-not-player0;190-consume-and-reserve-response-gift-then-cancel-without-return-recipient;source-directed-counters-not-created-without-source;187-self-mp-counter-and112-death-use-real-player-sources;ordinary-defense-filter-revival-death-redraw-and-deferred-gifts;targeted3-original-action-anchor-and-full-response-replay;no-owner-tick-gf-auto-source-items-mp-fee-or-operation-gift;resume-owner-or-next-alive-with-natural-before-cutoff;287-direct-fever-replacement-no-upgrade-or-heavenfall;288-idempotent-fog;290-direct-hp1-not-damage;294-sum-alive-cp-then-zero-all-before-recipient-cap99;296-exact-alive-count-explicit-guardian1-to10;guardian-changes-event-time-fog-filtered;public-model-recipient-only-no-assignment-array-or-cp-total;207-selector-unsupported;289-counts-require-dedicated-confusion-command-api;295-requires-dedicated-item-shuffle-api
```

### v37の検証

[confusion一次oracle](scripts/official_confusion_oracle.cjs) / [保存report](data/official-v2/confusion-client-v37.json) は158ケース、137,800bytes、SHA256 `335a67ee5978deeda63fdde663274e5b22078c261e46c335581fa322056c83b5` を記録し、rootの `--check` はbyte一致した。内訳はdecoder/fresh投影22・setter6・global48・safe-number境界5・attack use36・補助event12・cure6・advanceGF9・死亡4・終局4・command活性化6。実decoder、global/useAttackItems/補助/removeCurses/advanceGF/endGameの受信branch、player投影・setter・死亡処理・command活性化を、DOM/widget・同期Future等のshellを使って照合する。アプリ起動・ネットワーク・server自動選択は実行しない。

primaryではpray/discard/sacrifice/useDefenseItemsのevent単独には減算が無く、減算はuseAttackItems末尾にある。したがって実行器で祈り/捨てる/取引/両替を一律1減算へ対応づける部分は、完全なserver event列から導いた事実ではない。正countでも実clientの `fv.fF` は本人のhand/target/command controlsを活性化するが、これもserverが手動入力を受理する証拠ではない。専用外部APIと通常main拒否は明示的な実験制限である。raw JSのunsafeな加算表示を示す境界ケースと、Matchのsafe integerを超えた加算のatomic拒否も区別する。

新[confusion専用bench](src/official_confusion_bench.cpp) は289を1回または2回適用し、初期alive数×3回までのmain起動と、その全response/purchase決定を進める。countを0にした特権cloneで既存samplerからcommandを提案するのはテストcontrollerだけの方策で、live matchを変更せず、公式の混乱selectorとは呼ばない。通常main API拒否・policy RNG不変、専用APIの適法性、減算前anchor/flag、他者count保存と死亡/自然終局のclearを各stepの全snapshot restoreと合わせて検査する。world/countと選択した累計のhashを全field hashと呼ばず、私的journalまで含む一致はrestore比較で確認する。最終実行結果は次の確定欄へ記録する。

v37の最終全suiteはRelease **136/136件**（20.73秒）、ASan/UBSan **96/96件**（50.70秒）が通過した。native151群は全てmainへ各1回登録し、Release/SAN単体も通過した。Python105件、C API/Python focused2件、固定query384件、旧173/298ケースと実clientの2,360回のlegality照会も維持した。新7群はglobal加算・safe境界、専用API/通常sampler拒否の原子性、全主動作の1減算とcure、反射応答とproof改竄、売買/購入確認、死亡/復活/自然終局/cutoff、2/9人×20seedの外部controllerによる再生を検査する。READMEのPython例も実行して `9 False True` と `main 7 1` を確認した。suite時間をエンジン性能比較に使わない。

通常/canonical/payload/global phenomenon/targeted phenomenon/shuffle/confusionの各2人数、計14組で2人100,000局またはscene・9人10,000局またはsceneを1worker/4workersで実行し、worker数・時間・速度以外の全fieldが一致した。既存の通常/canonical/payloadはv36のgameplay field・checksumを維持した。global/targeted現象とshuffleはSnapshot.version37をhashへ含むためchecksumが変わるが、その他のgameplay fieldはv36と同値だった。Release/SANの通常1,000局×2人数・canonical420・payload456・global500・targeted600・shuffle600・confusion600の計8組も非時間fieldが一致した。

| 明示混乱controller・seed42 | scenes | 混乱main / 通常main | decisions / responses / purchase | natural / cutoff | primitive checksum | 後続world checksum |
| --- | ---: | ---: | ---: | ---: | --- | --- |
| 2人 | 100,000 | 594,773 / 0 | 1,185,985 / 432,355 / 8,857 | 2,784 / 0 | `18137689041152655999` | `13524298264185358902` |
| 9人 | 10,000 | 189,284 / 34 | 639,946 / 427,284 / 8,344 | 0 / 9,979 | `10157322869025728524` | `17185602672145400864` |

confusion smoke600sceneは289を900回、main11,347回（混乱11,346/通常1）、全38,399decisions、25,671responses/481purchaseを実行した。付与24,138point・最大count8、減算前anchor10,353回、counter1,786予約/1,735開始、death588予約/400開始、revival494、redraw2,138、自動guardian15,940を含み、world checksum `16730214666434115189` がRelease/SANで一致した。count8には初期の既存countと再付与を含む。ベンチは各scene最大64decisionsまたは初期alive数×3mainまでの有限実行で、9人smokeの599cutoffや大規模9人の9,979cutoffはその設定による。全sceneの自然決着や公式の混乱分布を検証した数字ではない。selector用RNGをengineへ追加せず、cloneによるcommand提案はテスト方策として区別する。


## v38：所有する207と外部指定結果を一括解決するFate API

`official-execution-subset-v38` は、実際に所有する207運命のひもを消費し、外部から指定した現象結果を同じ通常手番で解決する `FateAction(actor, token, outcome, shuffle?)` を追加する。Snapshot38は旧1〜37を拒否し、公開viewは `official-execution-view-v36`。Match ABI2、既存構造体の幅・phase・通常opcodeは維持し、64byteの追加C API型とnative/Pythonの専用入口を使う。公式serverの結果選出分布やscheduleが確定した更新ではない。旧37profile、official-v2 pins、過去の実測・v10/v16訂正履歴を保存し、学習器を追加しない。

公開scopeは `experimental:236-models;external207;not-official-server-equivalence`。通常配布catalogは236モデル/重み499のままで、自動giftに207を入れない。207は別のfate capabilityでのみ照会できる、外部入力scene用の特別な神器である。closed sceneでは実207のunused/unmaskedコピーを認めるが、通常play207を拒否し、通常samplerも207のplayを提案しない。捨てる・売買商品・除去・overflow・shuffleでは通常の実物として安全に扱い、能力を発動しない。Earth payloadの120 basesやcanonical42自動選出の閉包も変更しない。これを237モデル全配布・全自動超常現象対応とは呼ばない。

### 一次根拠と旧Flashとの差

現行台帳の207はsundries/callPhenomenon、price3/giftRate1。公開clientのphenomenon受信は供給された結果modelを元の列へ足して表示し、表示計算は先頭callPhenomenonを外して結果modelの数値を使う。これらは選ばれた結果を表示する根拠であり、十種類の候補分布・宛先・guardians・shuffle分配を決めるserver処理ではない。price3を操作手数料として徴収する根拠にもならない。

旧FlashのMYSTERYは助手10typeから選び、JupiterはILLUSION、UranusはLight60をrandom aliveへ、PlutoはDark30/75%を全敵へ、Neptuneは使用者HP+60、Venusは全員YEN99、Earthはpiecesだけの再配分だった。現行の混乱・Light50・個人HP+50・CP集合・used instance転送とは異なる。旧版の一様選択や攻撃者/対象・補充/病気/助手scheduleを、現行serverの根拠として移植しない。

### 1回の通常起動と結果payload

Fate APIはmainのownerだけが、自分の実207 tokenを単品指定して使う。独立した207targetや追加品は無い。generic9種の結果を明示するか、295と専用exact manifestを明示する。295以外へshuffleを付けず、295には必ずmanifestを付ける。入力は未公開の選出結果を外部から与える特権APIで、playerが自分の都合で公式の結果を選べるという主張ではない。

混乱ownerなら通常の混乱起動と同じくcountを1減らし、207を物理消費してgift1を予約し、その直後に結果を適用する。結果が289ならその後alive全員へ+3する。207消費と結果の間に別のdecision・次owner・補充を挟まず、accepted Fate全体で1decisionとする。sourceなしtargeted3種291/292/293は既存のresponse queueを使い、190は返却先なしとして取消し、source宛counterを作らず187 self反撃/112死後攻撃を実player sourceとして扱う。207の使用者をworld payloadのsourceや反射先へ昇格させない。global6種と295は即時bodyを一括適用する。

通常手番として、全body/response/FIFOの後にownerの病気tick・自動guardian・gift・GF・次ownerを既存の優先規約で進める。自然終局とdecision cutoffは通常どおり優先する。global/295はbody適用後にcutoffを判定し、targeted3が未解決でcutoffなら残work/giftを取り消す。独立した外部PhenomenonActionのno-tick/no-GF/no-auto規約とは別である。新しい選出用RNGやMP費用は無いが、命中・既存の病気/guardian・gift等のRNGは通常どおり進む。

### global結果と295専用manifestの最小例

`PYTHONPATH=python python3` で実行できる。207をsceneへ外部入力し、結果を選ぶ特権例であり、通常giftや公式selectorではない。295のmanifestは207消費後のコピーだけを指定する。

```python
from official_state import OfficialState
from official_match import Match, FateAction, PhenomenonAction, ItemSource, ItemShuffleAction

raw = {"gf": 0, "players": [
    {"id": 0, "hp": 40, "mp": 10, "cp": 20,
     "items": [{"id": 0, "modelId": 207}, {"id": 1, "modelId": 6}]},
    {"id": 7, "hp": 40, "mp": 10, "cp": 20,
     "items": [{"id": 0, "modelId": 235, "used": True}]},
]}
shuffle = ItemShuffleAction((ItemSource(7, 0), ItemSource(0, 1)))
for outcome, order in ((288, None), (295, shuffle)):
    with OfficialState(raw) as scene, Match() as game:
        game.load_scene(scene, owner=0, seed=42)
        game.fate_step(FateAction(0, 0, PhenomenonAction(outcome), order))
        print(outcome, game.status.phase, game.status.owner, game.observe()["board"]["gf"])
        print(game.observe()["last_fate"])
        if outcome == 295:
            hand = game.observe(7)["board"]["hand"]
            print([(item["token"], item["face"], item["used"]) for item in hand])
# 288 main 7 1
# {'actor': 0, 'model': 207, 'phenomenon': 288}
# 295 main 7 1
# {'actor': 0, 'model': 207, 'phenomenon': 295}
# [(3, 6, False)]
```

### 295 manifest・原子的失敗・復元

295のsource pairsは207を消費した後に残るaliveコピー全てを覆う。207自身やgift予約placeholderを含めない。既存typed shuffleと同じ各人実物枚数保存・fresh global token・used保持・recipient Dreamに従うprivate fake clearを適用する。単独shuffle APIを内側で呼んで別decisionを数えたり、消費後を起動anchorにしたりしない。

不正入力や遅いtoken/gift枯渇を含め、失敗は207消費前のworld・count・RNG・eventまで一括で戻す。private証拠は消費前anchor、指定結果/manifest、全response/purchaseを保持し、完了後も同executorによる全snapshot比較で検証する。shuffle後のgift/tick/auto等でreceiptが変わるため、旧globalのquiescent postconditionを最終状態へ機械的に当てはめない。

公開 `last_fate` はactor、model207、phenomenon（結果model）だけで、既存 `last_phenomenon` のmodel/recipient要約も使う。token、manifest、raw countやprivate anchor/journalは公開しない。Fateの私的累計は独立external phenomenon_actionsと分離する。過去の全公式対局履歴を証明するものではない。

未対応として残るのは207の自動結果選出、Earth/Moon一般selector、公式Dream全分布・実体不適合入力、悪魔・終末等である。既存通常samplerを破る未解決の結果選択phaseを黙って加えず、外部指定結果の実行と公式の自動抽選を区別する。

公開 `fate_rule` はheaderと同じ次の文字列である。

```text
experimental:external-outcome-owned207-only;standard-pool236-weight499-unchanged;207-scene-only-special-capability-no-gift-or-earth-payload;main-owner-single-actual-unused-unmasked207-token;normal207-play-and-sampler-generation-unsupported-discard-and-trade-inert;explicit-one-of-ten-phenomena-no-selector-or-selector-rng;generic9-outcome-shape-and295-only-exact-post-consumption-alive-copy-manifest;reserved-operation-gift-excluded-from-shuffle;one-normal-main-decision-single207-consumption-and-one-reserved-gift-no-mp-fee;positive-confusion-one-pre-outcome-debit;source-free-targeted291-292-293-with-ordinary-response-counter-death-redraw;global6-and295-immediate-body-before-cutoff;ordinary-owner-tick-auto-guardian-deferred-gifts-gf-next-owner-after-whole-action;natural-before-cutoff-pending-response-cancellation-as-normal;private-pre207-nonrecursive-anchor-full-action-and-response-replay-through-completion;public-actor-model207-outcome-model-only-plus-existing-phenomenon-recipient-no-token-manifest-or-count;failure-rolls-back-before207-use;server-outcome-distribution-targets-and-schedule-unverified
```

現行 `phenomenon_rule` も207の専用入口を明示する。

```text
experimental:external-source-free-phenomena287-288-289-290-291-292-293-294-296;main-boundary-one-injection-decision;global5-unchanged-from-v34-plus289-alive-count-plus3-immediate-atomic-no-rng;targeted291-light50-292-dark30-at75-percent-293-hpplus50-cap99;explicit-single-alive-recipient291-to294;no-target-selector-wide-or-fog-reselection;targeted3-always-response-after-single-target-chance-and-darkcloud;environmental-frame-origin-source-absent-not-player0;190-consume-and-reserve-response-gift-then-cancel-without-return-recipient;source-directed-counters-not-created-without-source;187-self-mp-counter-and112-death-use-real-player-sources;ordinary-defense-filter-revival-death-redraw-and-deferred-gifts;targeted3-original-action-anchor-and-full-response-replay;no-owner-tick-gf-auto-source-items-mp-fee-or-operation-gift;resume-owner-or-next-alive-with-natural-before-cutoff;287-direct-fever-replacement-no-upgrade-or-heavenfall;288-idempotent-fog;290-direct-hp1-not-damage;294-sum-alive-cp-then-zero-all-before-recipient-cap99;296-exact-alive-count-explicit-guardian1-to10;guardian-changes-event-time-fog-filtered;public-model-recipient-only-no-assignment-array-or-cp-total;207-selector-unsupported-owned207-requires-fate-api;289-counts-require-dedicated-confusion-command-api;295-requires-dedicated-item-shuffle-api
```

### v38の検証

[Fate一次oracle](scripts/official_fate_oracle.cjs) / [保存report](data/official-v2/fate-client-v38.json) は421ケース、117,149bytes、SHA256 `0f60a34134d1f42eaaf66e478f49aea78b5018d46e74ad7bf1a635f1cd0429b9` を記録する。append296・role4・target/submit48・decoder20・announcement53で、実append/start/default-target、選択・target helper、submit preflight、event decoder、useAttackItems/phenomenon受信、summary/数値helperを同期shellやcaptured RPCで照合する。アプリ起動・ネットワーク・server selectorは実行しない。

実clientでは207にFREE等を追加できず、選択時の自動enemy defaultも無いが、後からgeneric helperで選んだtargetが送信へ残る場合はある。submitが送るのはphysical itemIdsと残ったtargetで、選出した結果・重み・selector seedは送らない。phenomenon受信は整数modelを表示するだけで、無理に供給した非現象modelまで描ける。したがってrenderer受理を結果候補の合法性へ読み替えず、Fateのtarget無し入力・十種の外部結果・単一decision・消費/反射/補充scheduleは明示したローカル規約とする。naked207のresponse UIが190を認めることも、207自身に別の応答窓がある証明にはしない。

v38の最終全suiteはRelease **137/137件**（21.64秒）、ASan/UBSan **97/97件**（50.11秒）が通過した。native160群は全てmainへ各1回登録し、Python109件・C API/Python focused2件・固定query384件も通過した。旧173/298ケースと実clientの2,360回のlegality照会は維持した。新9群はscene限定207/capability/sampler、全10結果、207前anchor、sourceなし応答、postconsume295とgift順、confusion debit、MAX-2の安全加算、死亡/復活/end/cutoff、late GF/gift枯渇rollback、17種のproof改竄、完了後・guardian応答・反復起動のreplay、207のdiscard/Fog売却/購入/除去を検査する。橋ではvariant不一致をmanifest pointer参照前に拒否し、207をoverflow victimにする通常SELLも確認した。READMEのglobal288/295例は実行して記載どおりの結果になった。suite時間を速度改善の指標に使わない。

通常/canonical/payload/global phenomenon/targeted phenomenon/shuffle/confusion/fateの各2人数、計16組で2人100,000局またはscene・9人10,000局またはsceneを1worker/4workersで実行し、worker数・時間・速度以外の全fieldが一致した。既存の通常/canonical/payloadはruleset・scope・rule metadataを除くv37のgameplay field・checksumを維持した。global/targeted現象・shuffle・confusionはSnapshot.version38をhashへ含むためchecksumが変わるが、その他のgameplay fieldはv37と同値だった。Release/SANの通常1,000局×2人数・canonical420・payload456・global500・targeted600・shuffle600・confusion600・fate600の計9組も非時間fieldが一致した。

新しい[Fate専用bench](src/official_fate_bench.cpp) は `godfield_fate_bench`、CTest登録は `official_fate_benchmark 600 9 42 2` である。全10結果を固定の周期で指定し、各sceneで207を一度使って全responseを完了する。これは公式分布の抽選でも、全sceneの自然決着までの対局でもない。world・選択した累計・manifestのhashと、private journalを含む全snapshot restoreの検査を区別する。

| 明示Fate・seed42 | scenes | decisions / responses | natural / cutoff | confusion debit | 295 copy | primitive checksum | manifest checksum | 後続world checksum |
| --- | ---: | ---: | ---: | ---: | ---: | --- | --- | --- |
| 2人 | 100,000 | 155,774 / 55,774 | 2,859 / 0 | 66,666 | 180,000 | `8787916626795642147` | `7394346407599480527` | `6152307982011462577` |
| 9人 | 10,000 | 31,683 / 21,683 | 0 / 0 | 6,666 | 95,312 | `2522107609375653635` | `14722099739704624541` | `2778877789192396666` |

fate smoke600sceneは1,951decisions/1,351responses、10結果各60回、混乱減算400回、295を60回/5,707copy（used536）、初期used5,400copyのlaunch保存を確認した。world応答169/190取消50、source宛counter品選択33、実source frame観測1,182（PlayerID0は59）、counter81予約/81開始、death28予約/26開始、revival121、redraw157、通常tick180、自動guardian1,027も含む。frame観測は待機境界ごとの観測回数で、ユニークなpayload数ではない。world checksum `6241521733461852426` がRelease/SANで一致し、world初期frameでは292の既存chance roll以外のselector RNGが無いことを独立計算で検査した。現象の選択分布、207の自動配布、公式のターン処理を証明するものではない。


## v39：外部指定の悪魔5種と直接効果

`official-execution-subset-v39` は、`DevilAction(target, model, resource?, removed)` で240〜244の5悪魔を外部指定する入口を追加する。Snapshot39は旧1〜38を拒否し、公開viewは `official-execution-view-v37`。Match ABI2の既存構造体幅・phase・opcodeを維持し、32byteの追加C型とnative/Pythonの専用入口を使う。公開scopeは `experimental:236-models;external207;external-devils;not-official-server-equivalence`。通常配布236モデル/重み499とspecial207は変更しない。実在する悪魔itemの所有を必要とせず、悪魔を通常gift poolやplayer inventoryへ入れる機能ではない。旧37profile、official-v2 pins、過去の実測・v10/v16訂正履歴を保存し、学習器を追加しない。

### 公開説明・受信処理と未確定のserver規約

JAは終末以降に「捨てる」が「ささげる」へ置き換わり、神器を授かるたび25%で悪魔が現れると説明する。台帳240/241/242はHP被害10/20/30、243は神器または起こした奇跡を二個盗み、244はHP/MP/CPのいずれか+10。appearanceRateは7/5/3/5/5である。しかし、serverの悪魔選出・244の資源選択・243の候補抽選・再gift連鎖の順や乱数器を受信handlerから決めることはできない。

実clientのuseDevilItemは指定playerと供給itemを表示し、本人の悪魔個体を消してempty placeholderを作れる。続くresource/removed rowsは別eventで供給される。HP増減のplayerId0はcurrent paneを指す受信sentinelで、MP/CP増加はcurrent paneへ適用される。この表示上の宛先と、typed APIが明示する実在PlayerID0を混同しない。旧Flash固定server modulesには対応する悪魔/終末/SACRIFICE処理が見当たらず、旧仕様を補う根拠にはしない。

### typed効果とsourceなしの死亡処理

main境界で明示したalive targetだけに、1external decisionとして適用する。240/241/242はHPだけを10/20/30減らして0にclipする。244はHP/MP/CPの明示resourceを必須とし、+10して99にclipする。他資源への転送や別ownerからの支払は無い。無関係なresource/除去tokenを付けた入力は拒否する。選出RNG・操作cost・操作gift・ownerのtick/GF/auto guardian・混乱減算は起こさない。

HP被害は直接作用で、普通DEF/190/最初のresponse/source宛counterを作らない。正の実HP損失には既存10%のguardian departureを適用するが、悪魔にこの退去判定を行うことは明示的なローカル規約である。致死時は共通208救済・実死亡・112 FIFOを扱い、自然終局をcutoffより先に判定する。後続death/counterのresponseは通常APIで解決し、全work後に生存者へ既予約giftを配り、元ownerが生存なら同owner、死亡なら次aliveへ戻る。GFは増やさない。208に由来する補充は旧来の236モデルpoolを使い、悪魔の連続出現を抽選しない。

### 243の実物除去と公開範囲

243はtargetの現inventoryから、`min(2, hand_count)` 個のdistinct tokenを供給順に指定する。unused、used奇跡、208/209/112やspecial207も候補となる。空手札なら空tupleでよく、少数/重複/他者token等の非canonical入力は先行検査で拒否する。除去だけにconsume/gift/能力発動を付けず、保持していた奇跡が失われても既存curseを解除しない。

fake付きcopyは共通reveal helperでactualへ戻し、供給順のremoved actual modelを公開する。`last_devil` は `{model, target, removed_models:[IDs]}` で、243以外のlistは空。resource、token、元候補数、private anchorを出さず、listを超えたquantity fieldを追加しない。event/list/revealsを全viewerへ配るのはローカルfanout規約である。clientが受信rowsを表示する事実は、serverが全viewerへ同じ情報を送る権限の証明ではない。boardのFog可視性は既存のままで、他の未除去手札を表示しない。

### 243除去と244資源増加の最小例

`PYTHONPATH=python python3` で実行できる。悪魔の所有・出現抽選・終末開始を作る例ではなく、対象と結果を外部指定する。243はused奇跡も除去し、244は指定MPを99へclipする。

```python
from official_state import OfficialState
from official_match import Match, DevilAction

raw = {"gf": 0, "players": [
    {"id": 0, "hp": 40, "mp": 10, "cp": 20, "items": []},
    {"id": 7, "hp": 40, "mp": 95, "cp": 20, "curses": ["dream"],
     "items": [{"id": 10, "modelId": 6, "fakeModelId": 7},
               {"id": 11, "modelId": 235, "used": True}]},
]}
with OfficialState(raw) as scene, Match() as game:
    game.load_scene(scene, owner=0, seed=42)
    game.devil_step(DevilAction(7, 243, removed=(10, 11)))
    print(game.observe()["last_devil"])
    # {'model': 243, 'target': 7, 'removed_models': [6, 235]}
    game.devil_step(DevilAction(7, 244, resource="mp"))
    board = game.observe(7)["board"]
    print(board["players"][1]["status"]["mp"],
          game.status.phase, game.status.owner, board["gf"])
    # 99 main 0 0
```

### 原子的失敗・復元と残件

privateな非再帰の起動前anchor・action・全responseを保持し、完了後も同executorを通じた全snapshot比較で検証する。private devil_actionsは既存external系の累計と分離し、後続actionも全anchorへそのprefix累計をコピーして再生する。次のaccepted main/externalでeventと証拠をclearし、失敗時はHP・除去・RNG・count・giftを含め元状態を保持する。pending死亡workを途中で捨ててquiescent stateへ見せかけない。

この入口はtiebreakGFのgate、25%出現、ささげる、giftの悪魔置換や連鎖を実装しない。207結果選出・Earth/Moon一般selector、公式Dream全分布・実体不適合入力、終末/悪魔の自動schedule等は残件として維持する。指定した5種への分布正規化も行わず、現行serverの全面再現とは呼ばない。

公開 `devil_rule` はheaderと同じ次の文字列である。

```text
experimental:external240-to244-explicit-alive-target-only;main-boundary-one-external-decision;no-owned-devil-or-gift-selector-tiebreak-threshold-sacrifice-or-cascade;standard236-weight499-and-special207-unchanged;240-241-242-direct-hp-minus10-20-30-no-initial-response-def-reflection-or-counter;positive-hp-loss-uses-local-ten-percent-guardian-departure;243-exact-min2-physical-copies-distinct-owned-token-order-including-used-passives207;no-removal-rng-gift-or-ability-trigger;local-allviewer-actual-model-list-and-existing-fake-reveal-not-server-fanout-proof;244-explicit-hp-mp-cp-plus10-cap99;inactive-resource-and-removal-fields-rejected;shared208-revival112-death-and-response-counter-fifo;no-operation-cost-gift-owner-tick-gf-auto-or-confusion-debit;revival-gift-uses-existing-pool-without-new-devil-roll;resume-live-owner-or-next-alive-without-gf;natural-before-cutoff-body-before-pending-cancel;private-preaction-nonrecursive-anchor-full-response-replay-through-completion;public-model-target-removed-models-only-no-resource-token-or-anchor;failure-atomic;appearance-rate-resource-and-copy-choice-and-server-schedule-unverified
```

### v39の検証

[悪魔一次oracle](scripts/official_devil_oracle.cjs) / [保存report](data/official-v2/devil-client-v39.json) は260ケース、749,116bytes、SHA256 `f77dacc0616469a10b204cf07027c57bc23becd088e929ba6625c879a6a8d4b0` を記録する。useDevil60・gift後use6・数値followup120・forced宛先9・除去followup54・decoder11で、実gift/useDevilItem/resource/removeSomething dispatcher、本人cleanup/placeholder、dr/kq/cA/bZのreveal、used-pane削除、資源算術・pane宛先helperを動かす。visual/widget/同期Future等のshellを使い、アプリ起動・ネットワーク・権威的serverは実行しない。

実受信ではuseDevilItem単独はHP/MP/CPを変えず、後続eventが値を供給する。removeSomethingは強制した3rowまで全件表示でき、二個という上限や候補をhandlerが選ぶわけではない。fakeからactualへのrevealはself/foreign/spectator/Fogでも描画され、本人のphysical token削除とpublic used-pane削除は別の処理である。この観測は供給eventの表示内容を示すだけで、全viewerへそのeventを配るserver権限や順序を証明しない。5モデルの型・量と、typed外部選択/二個制限/全viewer fanout/死亡schedule等のローカル契約を区別する。

v39の最終検証はRelease **138/138件**（既存137件18.69秒＋新bench1件0.23秒）、ASan/UBSan **98/98件**（既存97件45.48秒＋新bench1件0.46秒）の分割実行で全て通過した。native168群はmainへ各1回登録し、Python114件・C API/Python focused2件・固定query384件も通過した。旧173/298ケースと実clientの2,360回のlegality照会は維持した。新8群は5モデルの直接効果/資源cap、243のmixed used/fake/passive/207、shapeとphase排他、late update・208gift ID枯渇のrollback/cutoff、208と自然終局、PlayerID0の112 FIFOとMP反撃・生存owner/次alive復帰、local GP退去/Fog、15種の完了proofと4種のpending改竄、後続Fate/shuffleの累計引継ぎを検査した。READMEの243/244例は実行して記載結果と一致した。READMEにあった旧版系Snapshot38という誤記も実値37へ戻し、旧版コード/ABI/pinsは変更していない。suite時間を速度比較へ使わない。

通常/canonical/payload/global phenomenon/targeted phenomenon/shuffle/confusion/fate/devilの各2人数、計18組で2人100,000局またはscene・9人10,000局またはsceneを1worker/4workersで実行し、worker数・時間・速度以外の全fieldが一致した。既存の通常/canonical/payloadはruleset・scope・rule metadataを除くv38のgameplay field・checksumを維持した。global/targeted現象・shuffle・confusion・fateはSnapshot.version39をhashへ含むためchecksumが変わるが、その他のgameplay fieldはv38と同値だった。Release/SANの通常1,000局×2人数・canonical420・payload456・global500・targeted600・shuffle600・confusion600・fate600・devil600の計10組も非時間fieldが一致した。

新[悪魔専用bench](src/official_devil_bench.cpp)は `godfield_devil_bench`、CTest登録は `official_devil_benchmark 600 9 42 2` である。5モデルを固定周期で指定し、244の資源3種、243の0/1/2コピー、alive/dead/ID0/owner死亡・208/112を含むsceneから、1外部効果とその全responseを完了する。world・選択した累計のhashと、private journalを含む全snapshot restoreの検査を区別し、25%出現率や公式のselector分布を検証したものではない。

| 明示Devil・seed42 | scenes | decisions / responses | natural / cutoff | 除去copy | revival | primitive checksum | 後続world checksum |
| --- | ---: | ---: | ---: | ---: | ---: | --- | --- |
| 2人 | 100,000 | 100,000 / 0 | 16,570 / 0 | 31,426 | 15,430 | `8048915018053637964` | `8048915018053637964` |
| 9人 | 10,000 | 26,915 / 16,915 | 579 / 0 | 3,140 | 3,836 | `4013012224972540074` | `6674028000984604963` |

devil smoke600sceneは5モデル各120回、244資源各40回、243の0/1/2コピーを17/17/86回指定し、1,592decisions/992responses、40自然終局を記録した。除去189copy（used21/fake9/passive55/207は9）、資源給付940、revival222、death1,888予約/990開始、counter53予約/2開始、redraw30を含む。直接悪魔の通常chance判定は0、普通ownerのtick/GF/autoも0で、応答は112とその派生に限る。反撃の予約後に相手が死亡していれば既存policyによりskipする。実source frame観測992（PlayerID0は137）は待機境界ごとの回数であり、ユニークpayload数ではない。world checksum `10402233983967069975` がRelease/SANで一致した。独立した10%GP計算、243/244の全world保存則、救済gift以外の初回token増加が無いことも検査した。終末開始・ささげる・悪魔の出現抽選と再配布連鎖は未対応のままである。


## v40：終末GFの限定対応と「ささげる」

`official-execution-subset-v40` はdecoded sceneの `tiebreak_gf` と、閾値に応じたdiscard→sacrificeの操作差し替えを扱う。Snapshot40は旧1〜39を拒否し、公開viewは `official-execution-view-v38`。Match ABI2、既存構造体幅・phase・opcode0〜8を維持し、既存Commandへopcode9 `sacrifice` をappendする。新しいConfig field・C action型・C API関数は追加しない。公開scopeは `experimental:236-models;external207;external-devils;threshold-sacrifice;not-official-server-equivalence`。通常配布236モデル/重み499、special207と外部悪魔5種は変更しない。旧37profileとその実Snapshot37、official-v2 pins、各版の実測・v10/v16訂正履歴を保存し、学習器を追加しない。

### client事実と閾値domain

実clientの終末判定は `threshold != null && gf >= threshold`。主動作UIはこの判定で固定operationモデル1のdiscardを2のsacrificeへ差し替える。公開設定UIの候補はnull/1/50/75/100/150で、教典は終末以降の操作変更と、神器授与ごと25%の悪魔出現を別々に説明する。後者の自動抽選や補充連鎖はこの更新に含めない。

Matchの閾値はnullまたは0〜JS safe integer上限の整数を認める。raw0/GF0もactiveとするのは、decoder入力を扱うための明示的なローカルadapter規約で、UI presetに0が存在するとは主張しない。負値・unsafe値は拒否する。default resetは従来どおりnullで、MatchConfigへ閾値を持ち込まずsceneの既存fieldを使う。公開top-levelの `apocalypse_active` は共通helperのboolで、既存boardのGF/thresholdと対応する。終局後も同じ比較結果を表示し、boolだけでactionを許可するものではない。

### opcode9・実物選択と補充

inactiveではdiscardだけ、activeではsacrificeだけを受理するcanonical operation replacementとする。閾値到達後に旧discardの送信をserverが受理するかは未確認で、この排他制限はローカル規約である。判定は起動前GF/thresholdで行い、その行動後にGFが閾値へ達しても遡ってgiftを付けない。

入力itemsは1〜hand limit個のdistinctな所有physical tokenだけで、仮想operationモデル2の所有tokenを含めない。target/allocationは無く、操作costは0。実clientと同じvisible eligibilityを使い、weaponとrevive208/attractDanger209は除外する。used奇跡は選択できる。互換クラス外Dreamや独自の見かけ変換を新しく許すものではない。

全selectedをdiscard方式でeraseした後、copyごとにgift1を予約する。同modelの別copyも各1個で、used奇跡を保持したり、再取得・MP支払・能力を発動したりしない。178redraw、168selfCurse、207現象等もこの操作では発動しない。選択がpublicへ現れる既存revealの扱いは共通consume経路に従う。

1accepted commandとして通常bodyの自然終局/cutoff・owner tick・死亡/208/112・auto guardianを先に処理し、継続時だけ既存236モデルpoolから遅延giftを配る。cutoffや死亡でgiftが取り消されてもbodyで消した品を自動で戻さない。遅いtoken/GF枯渇等の例外は既存transactional stepで入力前へ戻す。外部DevilActionは従来の特権入口のままで、threshold gateや通常giftの25%へ自動接続しない。

### GF0/threshold0でused奇跡をささげる最小例

`PYTHONPATH=python python3` で実行できる。raw0はローカルadapterが認める閾値で、UI presetの追加ではない。used奇跡の物理token0を消し、既存poolからunusedの新copyを1個授かる。

```python
from official_state import OfficialState
from official_match import Match, Command

raw = {"gf": 0, "tiebreakGF": 0, "players": [
    {"id": 0, "hp": 40, "mp": 10, "cp": 20,
     "items": [{"id": 0, "modelId": 235, "used": True}]},
    {"id": 7, "hp": 40, "mp": 10, "cp": 20, "items": []},
]}
with OfficialState(raw) as scene, Match() as game:
    game.load_scene(scene, owner=0, seed=42)
    print(game.observe()["apocalypse_active"],
          game.legal(Command(0, "discard", items=(0,))),
          game.legal(Command(0, "sacrifice", items=(0,))))
    # True False True
    game.step(Command(0, "sacrifice", items=(0,)))
    board = game.observe(0)["board"]
    print(game.status.phase, game.status.owner, board["gf"],
          [(item["token"], item["used"]) for item in board["hand"]])
    # main 7 1 [(1, False)]
```

### 混乱・sampler・再現性

混乱の専用 `confusion_step` もopcode9を受け、他mainと同じくlaunchでcountを1減らす。通常の混乱main拒否、response/purchaseの通常API・count非減算は維持する。samplerは起動時のstageでdiscard proposalのkindを9へ写し、候補の多重度・方策RNGの使い方を変更しない。default threshold=nullの既存対局・配布RNGは変えない。

通常ActionBatchが起動前GF/thresholdとkindを保存して全response journalを再生するため、新しいprivate proof型は不要である。after-stateのGFではなくanchorのlaunch条件で検証する。read-only stateに閾値があることと、公式の全終末scheduleを実行できることは区別する。ABI2の幅は同じでも、clientは新opcode9とruleset40を認識する必要がある。

残件は25%の自動悪魔出現、悪魔種/resource/copyの選出、gift置換・再授与連鎖、207/Earth/Moonの一般selector、公式Dream全分布・実体不適合入力等である。今回を終末完全対応とは呼ばず、旧Flashに見当たらない終末処理を補助仕様として持ち込まない。

公開 `apocalypse_rule` はheaderと同じ次の文字列である。

```text
experimental:decoded-nonnegative-safe-tiebreak-gf-or-null;active-iff-threshold-present-and-current-gf-at-least-threshold;raw0-active-from-gf0-local-domain-not-an-extra-ui-preset;default-reset-threshold-null-and-standard236-weight499-special207-unchanged;main-discard-before-threshold-sacrifice9-at-or-after-threshold;canonical-ui-operation-replacement-not-server-manual-opcode-acceptance-proof;virtual2-not-owned-or-gifted;one-or-more-distinct-owned-physical-tokens-no-target-allocation-or-mp-fee;visible-discard-eligibility-excludes-weapons208209-includes-used-miracles-compatible-dream-only;erase-all-selected-copies-including-used-then-reserve-one-gift-per-copy;shared-selected-fake-reveal-no-item-ability-retention-or-self-curse-trigger;ordinary-termination-cutoff-owner-tick-revival-death-auto-guardian-deferred-gift-gf-next-owner-schedule;natural-and-cutoff-cancel-gifts-as-normal;confusion-api-one-launch-debit;sampler-single-copy-operation-proposals-switch-kind-without-extra-selector-rng;private-prelaunch-gf-threshold-command-and-whole-pending-journal-replay;public-derived-apocalypse-active-bool-no-private-gift-manifest;no-devil-appearance-selection-gift-cascade-or207-distribution;server-gift-and-turn-order-unverified
```

### v40の検証

[ささげる一次oracle](scripts/official_sacrifice_oracle.cjs) / [保存report](data/official-v2/sacrifice-client-v40.json) は742ケース、157,046bytes、SHA256 `58e87ec7589141b852fb89af1d23af9e8109e11ca9b51f50c0cbba7cae9ccda9` を記録し、rootの `--check` はbyte一致した。内訳はthreshold decoder60・append592・used/fake16・fresh operation50・submit/consume/events24。実 `fu.gbw/sbh`、`tn`、fresh `fv.fq/fF`、`eu/ld/jS`、`fH.ed`、useAttackItems/sacrifice/discard、`e7/kh` をDOM/widget・同期Future等のshellで照合し、18個の消費から18 empty placeholders、usedMCの完全消費/公開used表示削除、費用0、Flash複数選択と送信operation0を確認した。placeholder表示は実配布の抽選・到着数・悪魔連鎖を証明しない。raw負閾値や互換クラス外fakeのreceiver挙動をMatch受理範囲へ広げていない。

v40の最終検証はRelease **139/139件**（既存138件24.85秒＋新bench1件0.21秒）、ASan/UBSan **99/99件**（既存98件52.37秒＋新bench1件0.49秒）の分割実行で通過した。native175群はmainへ各1回登録し、Python119件・C API/Python focused2件・固定query384件も通過した。旧173/298ケースと実clientの2,360回のlegality照会は維持した。新7群はnull/0/境界/MAX/負閾値/reset、used/fake/178/168/207の複数消費・能力不発/MP0、18usedMCとcutoff、role/identity/phase排他、confusion1debitと512seedのsampler対応、tick死/112/208と自然終局、late update/token/GF枯渇rollback、guardian応答中のlaunch kind/閾値/予約数の4種改竄を検査する。READMEの最小例も実行し、`True False True` と `main 7 1 [(1, False)]` が一致した。suite時間を速度比較へ使わない。

通常/canonical/payload/global phenomenon/targeted phenomenon/shuffle/confusion/fate/devil/sacrificeの各2人数、計20組で2人100,000局またはscene・9人10,000局またはsceneを1worker/4workersで実行し、worker数・時間・速度以外の全fieldが一致した。閾値なしの既存の通常/canonical/payloadはruleset・scope・rule metadataを除くv39のgameplay field・checksumを維持した。global/targeted現象・shuffle・confusion・fate・devilはSnapshot.version40をhashへ含むためchecksumが変わるが、その他のgameplay fieldはv39と同値だった。Release/SANの通常1,000局×2人数・canonical420・payload456・global500・targeted600・shuffle600・confusion600・fate600・devil600・sacrifice600の計11組も非時間fieldが一致した。

新[ささげる専用bench](src/official_sacrifice_bench.cpp)は `godfield_sacrifice_bench`、CTest登録は `official_sacrifice_benchmark 600 9 42 2` である。5閾値（null/0/17/18/MAX、初期GF17）から適切なdiscard/sacrificeを一度起動し、全responseを完了する。isolated sceneだけでコピー数・新gift数を厳密比較し、rich sceneではtick/112/auto/redraw等の後続と混同しない。worldと選択した累計のhash、private journalを含む全snapshot restoreを区別する。

| 閾値操作・seed42 | scenes | discard / sacrifice | decisions / responses | 選択copy / max | GF境界到達 | primitive checksum | 後続world checksum |
| --- | ---: | ---: | ---: | ---: | ---: | --- | --- |
| 2人 | 100,000 | 60,000 / 40,000 | 114,873 / 14,873 | 658,899 / 18 | 19,964 | `13222141139226396768` | `5015733505456582577` |
| 9人 | 10,000 | 6,000 / 4,000 | 21,918 / 11,918 | 65,866 / 18 | 2,000 | `9579152132219804691` | `14910632246140220679` |

sacrifice smoke600sceneはdiscard360/sacrifice240、1,302decisions/702responses、3,917選択copy（used718/fake160/207は340）、最大18個、混乱減算400回、threshold17→18到達120回を含む。isolated66sceneで275 replacement giftsを厳密検査し、18usedMC scene21回のうち8回を実際にささげた。通常後続ではdeath64予約/49開始、counter10予約/10開始、revival14、redraw91、自動guardian796を含み、world checksum `5513048592033815892` がRelease/SANで一致した。cloneによるsampler比較600回で342回のoperation kind差替えを観測し、候補順・選択token/target・policy RNGが一致した。全sceneで悪魔の自動出現/新しいselector RNGは追加しておらず、実配布のserver同値性・25%悪魔・連鎖を示す統計ではない。


## v41：opt-inの外部chanceと予約補充phase

`official-execution-subset-v41` は `Config.external_grants=False` を既定値にし、有効化した場合だけ、終末active中の予約giftを外部chance入力で進める。Snapshot41は旧1〜40を拒否し、公開viewは `official-execution-view-v39`。Match ABI2のConfig24/Command184/Status248/Devil・GrantAction32byteは維持する。既存phase0〜3はそのまま、opt-in時だけphase4 `grant` を追加する。C Configのbyte6をstrict0/1のexternal_grants flagへ割り当て、残るreservedは0を要求する。幅が同じでもphase4を認識する必要があるため、clientは正確なrulesetを確認する。旧37profile、official-v2 pins、各版の実測・v10/v16訂正履歴を保存し、学習器を追加しない。

### 分布を作らずに補充を止める

公開scopeは `experimental:236-models;external207;external-devils;threshold-sacrifice;external-reserved-grants;not-official-server-equivalence`。通常配布236モデル/重み499とspecial207は変更しない。flag=false、または終末がinactiveなら従来の予約gift処理を使い、出現判定の追加drawやdecisionを作らない。flag=trueかつactiveの場合はadvanceの予約gift pumpだけをchooserの直前で止め、`actor=None`、`pending_grant={target}` を公開する。人数別の残債務・次のrecipient・候補やRNGは公開しない。これは特権driver用のchance nodeで、playerの通常手選択ではない。

reset、178の即時redraw、売買商品の直接配送はこのphaseへ入れない。売買等の操作によって予約されたgiftは、通常のpumpへ達した時点で対象となる。JAの25%という説明とappearance metadataを参照できても、それを自動chooserへ置き換えない。悪魔種/resource/copyの分布、207の配布、公式の全gift経路を実装したとは呼ばない。

### GrantActionとretry債務

新しい `GrantAction` は既存DevilActionと同じ32byte型を使い、phase4のtargetを明示する。model0は通常補充の要求で、artifact ID0を配る意味ではない。既存236モデル/重み499のdrawと既存Dream maskを使って1copyを授け、元のgift債務を1だけ減らす。実際の通常modelやtokenをpublic last_grantへ追加しない。

model240〜244はv39と同じ明示devil payloadで、HP直接被害/244資源/243除去tokenを使う。出現判定・悪魔種・資源・除去copyのselector RNGは無い。実devil cardを所有・配布・再採番せず、生存したrecipientの元gift債務は保持してretryする。208救済による追加giftは元債務と別に予約し、243の消失には新しい補償giftを付けない。死者の債務、自然終局/cutoffで残った全債務は取り消す。

各accepted grant_stepは1decisionとupdateを消費する。資源が上限の妖精を何度指定してもmax_decisionsで有限となり、内部ループを黙って通常品へ差し替える必要がない。normal0のartifactやdevil bodyはcutoff判定前に適用し、残work/giftは既存の優先規約で取り消す。例外・不正入力ではchoice受理前のworld/RNG/decision/eventを保持する。

### death/FIFO・元の手番・可視性

devil bodyは共有のlocal10% guardian departure、208、112と後続response/counter FIFOを使う。必要な応答を先に解決してから、入力順で最小のlive positive gift seatへ戻る。tickやauto guardian stageを再実行せず、元の通常actionのGF/次ownerは全gift債務を解決した後だけ進める。Fate/Devil等から始まった場合も元のnormal/external scheduleとsource identityを保つ。

phase4では通常commandと通常samplerを拒否し、policy RNGを変えない。`grant_legal` / `grant_step` は特権入力で、243のtoken選択等をplayer viewだけから行うAPIとは呼ばない。responseとpurchaseは既存APIで進める。

公開 `last_grant` は `{target, devil:nullまたは240〜244, removed_models}` の実行済み要約だけで、通常model、token、resource、未来chooserや残債務を出さない。通常補充の受取人本人の手札表示まで隠す意味ではない。243のactual model reveal等のfanoutはv39と同じローカル規約、boardのFogは既存規約に従う。

### opt-inのささげる→妖精→通常補充の最小例

`PYTHONPATH=python python3` で実行できる。driverが244とnormal0を明示する例で、25%出現や資源分布を抽選しない。actorがNoneのgrant phaseは通常player policyへ渡さない。

```python
from official_state import OfficialState
from official_match import Match, Config, Command, GrantAction

raw = {"gf": 0, "tiebreakGF": 0, "players": [
    {"id": 0, "hp": 40, "mp": 95, "cp": 20,
     "items": [{"id": 0, "modelId": 235, "used": True}]},
    {"id": 7, "hp": 40, "mp": 10, "cp": 20, "items": []},
]}
with OfficialState(raw) as scene, Match(config=Config(external_grants=True)) as game:
    game.load_scene(scene, owner=0, seed=42)
    game.step(Command(0, "sacrifice", items=(0,)))
    print(game.status.phase, game.status.actor, game.observe()["pending_grant"])
    # grant None {'target': 0}
    game.grant_step(GrantAction(0, 244, resource="mp"))
    print(game.status.phase, game.observe()["last_grant"])
    # grant {'target': 0, 'devil': 244, 'removed_models': []}
    game.grant_step(GrantAction(0))
    print(game.status.phase, game.status.owner, game.observe()["board"]["gf"],
          game.status.decisions)
    # main 7 1 3
    print(game.observe()["last_grant"])
    # {'target': 0, 'devil': None, 'removed_models': []}
```

### 非再帰proofと未対応部分

private grant_proofは元ActionBatchの非再帰anchorとjournalを持ち、ActionBatch.grantsのGrantResolution.responses_before位置でchoiceを挿入して、応答・gift・死亡・再試行を同executorから全snapshot比較する。完了後も証拠を保持し、Fate/Devilの既存record identityを別actionへ付け替えない。grant_resolutions/artifacts/devilsの累計は独立して全anchorへprefixを保存する。各choiceをdecisionへ課すため、body/revival等の既存decision由来の上限を、無課金の自動連鎖のように緩めない。

この更新は外部chance driverへ制御を渡す基盤である。25%の自動出現、5悪魔の分布・資源/copy選択、全経路のgift置換、207/Earth/Moon一般selector、公式Dream全分布・実体不適合入力等は残る。既存表示・説明を、未取得の権威的server schedulerへ読み替えない。

公開 `grant_rule` はheaderと同じ次の文字列である。

```text
experimental:opt-in-external-grants-default-false;reserved-gift-pump-only-while-apocalypse-active;reset-immediate-redraw-and-trade-unchanged;phase4-grant-actor-absent-pending-recipient-only;explicit-model0-normal-existing236-weight499-draw-or240-to244-devil-payload;no-appearance-resource-or-copy-selector-rng;ordinary-grant-fills-one-debt-with-existing-compatible-dream-mask;devil-retains-original-slot-for-retry-no-physical-devil-token;208-adds-separate-debt-243-adds-no-replacement;dead-debts-cancelled;shared-direct-devil-body-local-guardian-departure-and-death-response-counter-fifo;resume-lowest-live-input-seat-with-positive-debt;one-accepted-grant-is-one-decision-and-update-for-finite-max-decisions-cutoff;no-repeat-owner-tick-auto-or-confusion-debit-original-gf-advance-only-after-drain;natural-before-cutoff-body-before-work-cancel;source-fate-devil-records-preserved;private-nonrecursive-full-original-action-response-prefix-grant-journal-through-completion;public-pending-target-last-grant-target-devil-or-null-removed-models-only;normal-model-token-resource-remaining-count-and-future-choices-private;normal-commands-and-sampler-refuse-grant-phase-with-policy-rng-unchanged;failed-resolution-rolls-back-to-current-pending-state;25percent-and7-5-3-5-5-ui-rates-not-an-engine-selector;server-distribution-retry-and-order-unverified
```

### v41の検証

最終検証はRelease CTest **140/140 passed（25.65秒）**、ASan/UBSan CTest **100/100 passed（54.29秒）**。native183関数とPython126メソッドは重複なく登録され、READMEのささげる→244→normal0の例も記載どおり実行した。これらはローカル契約の検証であり、公式server chooserの検証数ではない。

新native8群は、通常補充と元手番完了、既定flag=false/閾値前/resetの互換、243とretry/decision cutoff、208の追加債務とhistorical tick/112の応答交錯、Fate/Devil/守護神/sourceなし現象の起動元保持、不正shapeと遅いupdate/token/GF失敗のatomic性、順序付きproof改ざんと次actionの累計prefix、Dreamの非漏洩と即時redraw/売買配送の例外を扱う。`hand_limit=1` でも外部Earthの合法2モデル列を扱い、物理手札の上限と外部payload列の長さを混同しない回帰ケースを含む。

一次資料の再現は[official_grant_oracle.cjs](scripts/official_grant_oracle.cjs) と[grant-client-v41.json](data/official-v2/grant-client-v41.json)。`--check` は保存reportとbyte一致し、42ケースの内訳はReference表示20、予約slotへのgift/useDevilItem列12、snapshot decoder probe6、gift event decoder probe4。reportは42,014 bytes、SHA256は `69cdc991cbf06fe36991db69672103d87d6e416716f1b6d833cacc602b3b7143`（現fileから再計算）。実Referenceは5悪魔の出現率を7%/5%/3%/5%/5%と表示し、その合計25%はJA/ENの説明と整合する。実受信queue・既存placeholderへの配送/置換とdecoderを実行し、DOM/演出等のsinkはstubを使う。これらはserverによる悪魔出現・資源/copy選択・債務retryや死亡時の補充scheduleの証明ではない。通常配布のReference分母500も、実行器の236モデル/重み499へ読み替えない。

通常/canonical/payload/global現象/targeted現象/shuffle/confusion/fate/devil/sacrifice/grantの各2人数、計22組で2人100,000局またはscene・9人10,000局またはsceneを1worker/4workersで照合し、worker数・時間・速度以外の全fieldが一致した。Release/SANの小行列12組も同じ非時間fieldが一致した。既存10familyはflag=falseのままv40のgameplay fieldを維持し、通常/canonical/payloadはchecksumも維持した。その他はSnapshot.version41をhashに含むため、そのchecksumだけを旧版比較から除いた。worker間・Release/SAN間ではchecksumも比較した。

新grant benchは5起動元（sacrifice/pray/attack/所有207/standalone devil）を一度起動し、外部driverの明示choice、全応答、残債務を完了する。97scene毎に上限MPへの244を繰り返してmax_decisions96だけで打ち切り、他はnormal0を定期指定する。244の資源を悪魔種の周期から独立させ、600scene以上でHP/MP/CPの全3資源が選ばれたことを検査する。driverのこの修正後も対象CTestはRelease1/1（0.47秒）・SAN1/1（1.74秒）で通過した。worldと選択counterのhashは全private proof hashではなく、全snapshot restoreの完全一致を各境界で別に検査する。

| 外部補充・seed42 | scenes | decisions / responses | normal / devil解決 | 予算cutoff | primitive checksum | 後続world checksum |
| --- | ---: | ---: | ---: | ---: | --- | --- |
| 2人 | 100,000 | 545,191 / 54,477 | 160,346 / 230,368 | 1,031 | `12928503358174816886` | `6425240640703154995` |
| 9人 | 10,000 | 107,590 / 26,804 | 34,569 / 36,217 | 104 | `16124954102603085708` | `8795386664659394638` |

9人600sceneのRelease/SAN一致は6,533decisions、4,271grant解決（normal2,040/devil2,231）、death574予約/460開始、revival169、redraw190、cutoff7を含む。244のHP/MP/CP指定はそれぞれ120/777/116回で、checksumは `13373311760285701770`。全grant境界で通常policy拒否とRNG不変、対象順、元のtick/auto/GF進行、proofの応答prefix順序、public noticeの形を検査した。これらの回数は明示driverのfixture分布であり、公式の出現率やserver対局統計ではない。


## v42：公開出現率を使うローカル補充サンプラー

`official-execution-subset-v42` は、phase4専用の `GrantAction sample_grant(uint64_t& chance_rng) const`、Cの `gfo_match_sample_grant`、Pythonの `sample_grant(chance_rng) -> (GrantAction, next_rng)` を追加する。外部driverは通常補充/悪魔5種と必要なpayloadをこの関数で選び、既存 `grant_step` で適用できる。Snapshot42は旧1〜41を拒否し、公開viewは `official-execution-view-v40`。Config24/Command184/Status248/GrantAction32byte、ABI2、phase0〜4、scopeはv41と同じである。学習器・旧37profile・official-v2 pinsと過去の検証履歴を変更しない。

### 公開資料とローカル選択規約

| 項目 | 確認できた根拠 | v42で実行する規約 |
| --- | --- | --- |
| 悪魔の出現率 | 実Referenceの絶対表示7%/5%/3%/5%/5%、教典の合計25% | 100ticketのうち240=0〜6、241=7〜11、242=12〜14、243=15〜19、244=20〜24、通常0=25〜99 |
| 243の除去 | 神器か起こした奇跡を二個。均等・選択順の記載なし | 現受取人のraw物理inventoryをpartial Fisher-Yatesでmin(2,N)個、一様・非復元・順序付き選択 |
| 244の資源 | HP/MP/CPのいずれかを10増加。同率の記載なし | HP/MP/CPを各1/3で選択。既に99の資源も除外しない |
| RNGと連鎖 | 公開receiverは供給済みイベントを消費する。server chooser未取得 | 外部chance streamのSplitMix64/rejection bounded drawと既存v41の予約補充継続を使う |

表示された確率のmassと、ローカルな区間順・乱数方式・243/244の分布を区別する。旧Flashにも現行悪魔chooserを裏付ける実装は見つからず、そちらのpieces/magics別選択を根拠に転用しない。これは公式server selectorの完全移植ではない。

243はraw所有tokenを対象にし、used奇跡、passive、scene上の207、互換Dream fakeを除外しない。k番目は `j=k+bounded(N-k)` を選んでswapし、一つしか候補がなくてもdrawする。空手札ならcopy drawは0回。通常0はこの関数で神器modelやDream maskを選ばず、後の `grant_step` が既存ゲームRNGで236モデル/重み499を抽選する。207の通常配布や公式237/500には広げない。

### 特権・RNG・transaction

これは通常player policyへ渡すAPIではない。243の実token等は特権chance driver用で、公開viewへ選択候補・資源・未来choiceを追加しない。phase4以外はRNGを進める前に拒否する。成功時はcallerのchance_rngだけを更新し、Matchのworld、game RNG、通常policy RNG、decision、private journal、public eventを変えない。サンプル結果は適用後に既存grant journalへ保存される。全snapshotだけで外部chance streamは保存されないので、controllerはchance_rngもcheckpointへ含める。

sampleとstepは別transactionである。二つをまとめてcommitしたいdriverはchance_rngのcopyからsampleし、`grant_step` 成功後だけ元chance_rngへcommitする。失敗時に既に受理済みの元actionを巻き戻すものではない。通常samplerは引き続きphase4を拒否し、既定 `external_grants=False` の対局や明示GrantActionを使うdriverの動作は変わらない。reset、178の即時redraw、売買商品の直接配送への悪魔置換、207/Earth/Moon/混乱の一般selector、公式Dream全分布・実体不適合入力等は残件である。

公開 `grant_sampler_rule` はheaderと同じ次の文字列で、private seedや実行前payloadを含まない。

```text
experimental:privileged-const-grant-phase-only-local-sampler;caller-owned-chance-rng-separate-from-game-and-policy;splitmix64-rejection-bounded100-one-ticket;published-absolute-appearance-mass240-7-241-5-242-3-243-5-244-5-ordinary75;id-order-devils-first-ordinary-last;243-uniform-ordered-physical-copies-without-replacement-min2-raw-inventory-partial-fisher-yates-one-draw-per-copy-including-singleton-used-passives-and207-eligible;244-uniform-hp-mp-cp-one-draw-including-capped-resources;resource-and-copy-distributions-and-rng-staging-local-not-official;normal0-defers-actual-artifact-and-compatible-dream-draw-to-grant-step;no-match-state-game-rng-policy-rng-decision-or-public-event-mutation;sampling-failure-preserves-caller-rng;sample-and-step-separate-transactions;chance-rng-not-in-snapshot-controller-must-save-it;explicit-grant-api-and-default-behavior-unchanged;no-reset-redraw-trade-or207-fullpool-coverage;not-official-server-selector-equivalence
```

### v42の検証

[sourceとローカルreferenceの生成器](scripts/official_grant_sampler_oracle.cjs) / [保存report](data/official-v2/grant-sampler-client-v42.json) は、実Referenceの表示14件と、**別区分のローカルgolden**（100ticket境界12件・edge10件）を保存する。後者は独立BigIntによるSplitMix64/逆写像・rejection・partial Fisher-Yatesの参照値であり、公式chooserのgoldenとは呼ばない。full17は実際のH18予約境界、full18は現在到達不能の低水準reference専用として明示している。reportは31,427 bytes、SHA256 `c8ea648daba7af3217fe798909277b40befbaae58322bb0a69a535aeb4062cfa`。rootの `--check` はbyte一致した。

Release CTest **141/141（20.97秒）**、ASan/UBSan CTest **101/101（47.07秒）** が全て通過した。native188関数は配列へ各1回・旧183件の順序を保って登録し、Pythonは新3件を加え129メソッドが重複なく登録された。C API/Pythonのfocused2件も通過した。新native5群は逆写像で作る全100ticketの厳密mass、不変query/rejection、raw0/1/2/17copyとused/fake/passive/207・全資源上限、wrongphaseとchance copy-commit/遅い失敗、snapshot＋外部chance RNGの再開、独立BigIntの到達可能21境界/edge vectorを検査する。Cの9固定goldenは32byte全体のinactive/padding、NULL/phase failureのcanaryを含み、Pythonは独立SplitMix/FY参照と全100ticket、strict uint64 seed等を検査する。

テスト追加後、ASanで旧テストの入口にstack-overflowを検出した。mainへの多数関数のinline合算を避けるため、188件を関数ポインタ配列から順に呼ぶrunnerへ変更した。内容と順序、unused-functionによる未登録検出は保ち、修正後の全SAN suiteで通過した。suite時間を実行器の性能差へ読み替えない。READMEの例も実行し `[242, 0] main 7 3 4354685564936845396` が一致した。

通常/canonical/payload/global現象/targeted現象/shuffle/confusion/fate/devil/sacrifice/明示grant/新grant samplerの各2人数、計24組で2人100,000局またはscene・9人10,000局またはsceneを1worker/4workersで実行し、時間・速度・worker数以外の全fieldが一致した。Release/SAN小行列13組もchecksumを含む非時間fieldが一致した。サンプラーを呼ばない既存11familyのgameplay fieldはv41と同値で、通常/canonical/payloadはchecksumも同値だった。残るfamilyの旧版比較だけはSnapshot.version42を含むchecksumを除き、worker間・Release/SAN間ではchecksumも照合した。

新 `godfield_grant_sampler_bench`（CTest `official_grant_sampler_benchmark 600 9 42 2`）は5起動元とrich sceneから、全grant choiceをhelperで選び、必要な応答を含む元actionの完了まで進める。通常policyと独立chance streamを持ち、各grant境界で2個のrestore cloneに同seedを渡してpayload・next_rng・Match不変を照合する。実stepが成功した後だけchance streamをcommitする。97scene毎はmax_decisions2でsample適用後cutoffを検査する。600scene以上では6outcome、全3資源、used奇跡の除去が実際に選ばれたことを検査する。choice列、chance RNG列、world＋選択counterのchecksumを分け、全private snapshotは各checkpointのrestore完全一致で別に検査する。

| ローカル抽選・seed42 | scenes | normal / devil解決 | 妖精HP / MP / CP | chance checksum | action checksum | world checksum |
| --- | ---: | ---: | ---: | --- | --- | --- |
| 2人 | 100,000 | 167,961 / 55,780 | 3746 / 3676 / 3733 | `14045303676355207807` | `5923566862139370756` | `12269019345486087550` |
| 9人 | 10,000 | 32,203 / 10,729 | 701 / 704 / 695 | `4731159512000537428` | `16856585937920654990` | `4831156122709328760` |

9人600sceneでは4,602decisions、2,592grant解決（normal1903/devil689）、death280予約/214開始、revival164、redraw191、cutoff probe7を含む。243はused13・fake37・207は8copyを選び、244のHP/MP/CP指定は38/43/41回（上限資源の指定61回）。chance/action/world checksumはそれぞれ `10419100228575040986` / `14144487001133331999` / `91435489873529096` でRelease/SAN一致。fixtureの終了・死亡・cutoffにより観測したsample数はscene数と異なり、これを公式serverの対局分布や同値性の検証として扱わない。

### Pythonで終末の補充を自動進行する例

`PYTHONPATH=python python3` で実行できる。受理成功後にだけchance seedを更新する。実際のdriverではresponse/purchaseを既存APIで解決してから、再びgrant phaseを処理する。

```python
from official_state import OfficialState
from official_match import Match, Config, Command

raw = {"gf": 0, "tiebreakGF": 0, "players": [
    {"id": 0, "hp": 99, "mp": 99, "cp": 99,
     "items": [{"id": 0, "modelId": 235, "used": True}]},
    {"id": 7, "hp": 99, "mp": 99, "cp": 99, "items": []},
]}
with OfficialState(raw) as scene, Match(config=Config(external_grants=True)) as game:
    game.load_scene(scene, owner=0, seed=7)
    game.step(Command(0, "sacrifice", items=(0,)))
    chance_rng, outcomes = 42, []
    while game.status.phase == "grant":
        action, trial_rng = game.sample_grant(chance_rng)
        game.grant_step(action)
        chance_rng = trial_rng
        outcomes.append(action.model)
    print(outcomes, game.status.phase, game.status.owner,
          game.status.decisions, chance_rng)
    # [242, 0] main 7 3 4354685564936845396
```


## v43：運命のひもを含む237種・重み500の任意配布

`official-execution-subset-v43` は `Config.include_fate=False` を追加し、有効時に公式台帳のpositive giftRate全237モデル・合計500を通常配布対象とする。既定236/499は維持する。207「運命のひも」はID順の500ticket中467の1枠で、sundriesが18→19になり、trade3/weapons107/armor78/miracles30は同じ。抽選枠のID順・SplitMix64・Dreamと補充scheduleはローカル実験規約であり、台帳のmassへの対応を公式server全体の同値性と呼ばない。

Snapshot43は旧1〜42を拒否し、公開viewは `official-execution-view-v41`。C Config24byteのbyte6はexternal_grants、byte7はinclude_fateという独立したstrict0/1のflagで、従来の最後のreserved1byteを使う。tail4byteのzero要求、Command184/Status248/GrantAction32byte、Match ABI2とphase0〜4は維持する。scopeは `experimental:236-default-or237-opt-in;external207;external-devils;threshold-sacrifice;external-reserved-grants;not-official-server-equivalence`。旧37profile、台帳pins、過去のreport/測定値、学習器なしの範囲は保つ。

### 配布経路と207の役割

共通gift入口でpoolを切り替え、reset初期配布、pray、ささげる等の予約補充、208の救済補充、178の即時redraw、外部grantのnormal0すべてに適用する。取引商品の直接配送は選出poolを使わず、既存のcopy配送である。external_grantsとの4通りのflag組合せを独立に扱う。悪魔の置換をreset/redraw/取引へ広げる変更ではない。

207はunused/unmaskedの新tokenとして授ける。互換Dream signatureで207に一致する他modelは全296中0件なので、207を引いた場合はmask coin/face drawを追加しない。既存のeligible146モデル・重み321・順序付きpair992は変わらず、singletonが90→91になる。これは互換クラス限定のローカルDream規約であり、公式の全fake分布を再現したという意味ではない。

授かった207は既存 `fate_step` で、所有tokenと外部指定の287〜296の結果を渡して使用できる。消費、通常手番のtick/auto/補充/GF、混乱減算とprivate proofは既存実装を使う。通常Commandの207 play、通常samplerによるFate生成、Earth/Moonの207 payloadは許さない。普通samplerは既存のdiscard/trade等だけを提案する。超常現象の選出率・対象、296の守護神配列、295の入替列、自動混乱選択は残件である。現象の率が未記載であることを0%と解釈したり、旧Flashのuniform10・旧効果を現行の根拠にしない。

### 現在の配布poolを照会する

| API | 通常Command対象 | 実設定のgift対象・重み |
| --- | --- | --- |
| C++ | global match_supported/match_catalog/match_gift_weightは従来236/499 | Match.gift_supported / gift_catalog / gift_weight |
| C ABI | global gfo_match_supported/catalog_count/gift_weightは従来236/499 | gfo_match_gift_supported / gfo_match_get_gift_weight。statusと別out、NULLは出力不変 |
| Python | supported_modelsは従来236 | gift_models / gift_weightはnative handleの実設定を反映 |

公開 `gift_pool` は `{include_fate, models, weight}` という静的な設定の要約だけで、実配布model、未来ticket、tokenやRNGは出さない。normal0のpublic last_grantにも207のactual modelを追加しない。native restore後のconfigに従ってpoolを選び、drawに依存する完了journalもそのconfigで再現する。defaultfalseでも従来のscene-owned207は受理できるので、pool flagを過去の全配布履歴の証明とは扱わない。

公開 `fate_pool_rule` は次のheader定数に一致する。既存fate/grant/devil ruleのpool表記も、既定236/499と任意237/500を区別する現在の契約へ更新する。

```text
experimental:opt-in-include-fate-default-false;default236-models-weight499-unchanged;enabled-all237-positive-giftrate-models-weight500-in-catalog-id-order;207-one-ticket-at467-unused-and-unmasked;shared-gift-entry-for-reset-immediate178-redraw-reserved-revival-and-external-normal0;no207-compatible-dream-candidates-no-mask-coin-or-face-draw;existing-non207-compatible-dream-policy-unchanged;normal-command-capability-and-global236-weight499-descriptors-unchanged;current-match-gift-supported-catalog-weight-queries;public-static-gift-pool-flag-count-weight-only;owned207-use-requires-existing-explicit-fate-api;no-normal207-play-or-player-sampler-generation-no-earth-moon207-payload;no-phenomenon-resource-target-or-shuffle-selector-added;no-reset-redraw-or-trade-devil-coverage-added;pool-frequencies-not-official-server-rng-or-schedule-equivalence
```

### v43の検証

[配布pool一次調査とローカルreference](scripts/official_fate_pool_oracle.cjs) / [保存report](data/official-v2/fate-pool-client-v43.json) は、実Reference16件（207等の1/500対照と現象10種の率labelなし）、全237positive ID・重み500・category件数を固定する。ID順237ranges・独立BigIntの境界5件・Dream signature比較は**別区分のローカルreference**で、公式chooserのgoldenとは呼ばない。207のticket466/467/468周辺を、同seedの旧499版とも比較する。reportは42,845 bytes、SHA256 `149ca93115e05835cd08e80c3ec78c384b0c7dddf3fa340fadd4fc3e35f3c66a`、rootの `--check` はbyte一致した。

Release CTest **142/142（28.40秒）**、ASan/UBSan CTest **102/102（54.98秒）** が全て通過した。native193関数とPython133メソッドは各1回・重複なしで登録し、C API/Pythonのfocused2件も通過した。新native5群では全500ticketの厳密model/giftRate一致、current-queryとglobal能力の分離、6配布経路の207、Dream追加drawなし、通常role/payloadの境界、授かった207の既存Fate使用とpending/完了proof、modeに依存するdrawの再生、defaultfalseの互換、通常操作/受理済みgrantの遅いtoken枯渇rollbackを検査する。Earth payload不許可はsourceが実際にEarthを持つsceneでも検査し、guardian不一致だけによる拒否と混同しない。

新Python4件とC群では全237 ID・合計500、C Configの両flag/offset/tail4byte、NULL出力canary、全配布経路のunused/unmasked207、既存Fate使用/normal sampler除外、defaultfalseの再現性を確認した。READMEの初期207例を実行し、`237 500 236`、hand token0/face207/未使用非偽装、`main 1 1 {'actor': 0, 'model': 207, 'phenomenon': 288}` が一致した。suite時間を速度比較へ使わず、配布massの検証数を公式serverの抽選や全状態遷移の検証数へ読み替えない。

通常/canonical/payload/global現象/targeted現象/shuffle/confusion/fate/devil/sacrifice/明示grant/grant sampler/新Fate poolの各2人数、計26組で2人100,000局またはscene・9人10,000局またはsceneを1worker/4workersで照合し、worker数・時間・速度以外の全fieldが一致した。Release/SAN小行列14組もchecksumを含む非時間fieldが一致した。新flagを使わない既存12familyのgameplay fieldはv42と同値で、通常/canonical/payloadはchecksumも維持した。その他の旧版比較だけはSnapshot.version43を含むchecksumを除き、worker間・Release/SAN間ではchecksumも比較した。

新 `godfield_fate_pool_bench`（CTest `official_fate_pool_benchmark 600 9 42 2`）はreset/pray/1または3copy sacrifice/178redraw/208救済/外部normal0の6経路を回す。到達した207の所有者がmain ownerなら、結果288を明示した既存Fateを一度使い、その後の補充まで完了する。通常rich matrixとは別に、6経路のclean fixtureについて独立500ticket/Dream参照から207到達seedを探し、6つの実sceneでreceipt・RNG・tokenを厳密比較する。検索中にengineを反復するものではない。10000scene以上では全237model出現と授かった207の明示使用が実際に起きたことを検査する。

| 任意237pool・seed42 | scenes | 出現model種数 | 新token発行 / 観測copy | 観測207 / 明示Fate使用 | primitive checksum | world checksum |
| --- | ---: | ---: | ---: | ---: | --- | --- |
| 2人 | 100,000 | 237 | 451,885 / 451,885 | 901 / 435 | `5426914609901215439` | `9144771191245312904` |
| 9人 | 10,000 | 237 | 155,942 / 155,942 | 320 / 47 | `17957368678633913634` | `999832884538341161` |

9人600sceneでは1559decisions、9364新token・9364観測copy、16個の207、明示Fate1回、redraw100、revival132を含む。独立参照の補助probeは6経路、91copyを照合し、207は6経路すべてで到達、reference checksum `10292494696839945612` が一致した。world＋選択counterのchecksumは `10861144430966346463`、全private proofは各checkpointのrestore完全一致で別に検査する。新token数・観測copy数・独立probeの集計は別であり、これらを公式serverの出現分布や超常現象selectorの検証として扱わない。

### Pythonで初期配布の207を使う例

この固定seedはローカル500ticketの207境界を使う。濃霧288はdriverの明示指定であり、自動現象抽選の例ではない。

```python
from official_match import Match, Config, FateAction, PhenomenonAction

with Match(seed=0x14e9cbeb5fbb6a39,
           config=Config(initial_hand=1, include_fate=True)) as game:
    print(len(game.gift_models), game.gift_weight, len(game.supported_models))
    # 237 500 236
    print(game.observe(0)["board"]["hand"])
    # [{'token': 0, 'face': 207, 'reference': 207,
    #   'disguised': False, 'used': False}]
    game.fate_step(FateAction(0, 0, PhenomenonAction(288)))
    print(game.status.phase, game.status.owner, game.status.decisions,
          game.observe()["last_fate"])
    # main 1 1 {'actor': 0, 'model': 207, 'phenomenon': 288}
```


## v44：運命のひものローカル結果サンプラー

`official-execution-subset-v44` は `FateAction sample_fate(ItemToken token, uint64_t& chance_rng) const`、Cの `gfo_match_sample_fate`、Pythonの `sample_fate(token, chance_rng) -> (FateAction, next_rng)` を追加する。player/controllerが選んだ所有207のtokenに対して、結果と必要なpayloadをchance側で生成し、既存 `fate_step` に渡せる。**現象の選出率・対象・守護神割当・入替分布は公式未確認のローカル規約**で、旧Flashの分布・効果を現行公式の証拠にしない。

Snapshot44は旧1〜43を拒否し、公開viewは `official-execution-view-v42`。scope、Config24/Command184/Status248/FateAction64byteとMatch ABI2、phase0〜4はv43と同じ。既定236/499・任意237/500の配布や、明示FateActionで指定する既存driverは変えない。旧37profile・台帳pins・既存report/実測を保存し、学習器は追加しない。

### chanceで選ぶ範囲

| 項目 | ローカルな生成規約 |
| --- | --- |
| 現象287〜296 | SplitMix64のrejection bounded10から287+ticketを各1/10。公開された公式率ではない |
| 291〜294のrecipient | 生存者の入力順から一様に1人。owner/味方も含み、候補1人でもdrawする |
| 296の日食 | 生存者の入力順で各1draw、Guardian1〜10を各1/10。重複や現在と同種も許す |
| 295の磁気嵐 | 生存者のraw inventoryからsource-qualified(owner,token)列を作り、使うowner/tokenの207だけ1個除く。N→2の後方Fisher-Yatesで各bounded(i)を1draw |
| 295の0/1copy | permutation drawなし。別ownerの同token0、別の207、used奇跡・passive・互換fakeは残し、dead inventoryと未到着の予約giftは入れない |

manifestは消費後の物理copyに対応し、最大9人×18copy−1=161。元のMatchから207を先に実消費するものではない。引いた結果を既存prepare_fateで検証してからchance_rngをcommitする。289の混乱countが表現不能なら、再抽選で分布を変えず、Match/RNGを保持して拒否する。混乱中のownerでもsampleでき、実countの減算は後のFate起動で一度だけ行う。

### 特権・保存・失敗の境界

抽選中はworld、ゲームRNG、player policy RNG、decision、journal、public eventを変えない。公開viewへ追加するのは静的 `fate_sampler_rule` だけで、future outcome、manifest、seedは出さない。このAPIは全員のrawcopyを扱う特権chance controller用であり、返却payloadを通常player policyの選択肢として渡す設計ではない。

sampleとfate_stepは別transactionである。sampleが成功しても、その後の295のfresh token枯渇、update/GFの枯渇等で実行が失敗する場合がある。driverはchance seedのcopyでsampleし、fate_step成功後だけseedをcommitできる。Match snapshotだけでは外部chance streamを保存しないので、controllerのcheckpointにもseedを持たせる。適用後は既存Fate/grant proofが受理したpayloadと応答列を再生する。

通常player samplerは引き続き207を起動しない。player側がひもを使うtokenを選び、chance側が結果を選ぶ分担で、結果/recipient/shuffleをplayerの戦略選択に変えない。混乱の自動command、Earth/Moonの一般payload選出、公式Dream全分布・実体不適合入力、全配布経路への悪魔置換と権威的server scheduleは残件である。

### Cの可変長出力

Cは既存64byte FateActionと、callerが所有するItemSource配列を使う。`gfo_match_sample_fate(match, token, &chance_rng, order, capacity, &out)` に0〜162のcapacityを渡す。capacity0ならNULL配列を使え、non295とempty295は0bufferで成功する。必要なmanifestがcapacityを超えた場合は-4で、chance_rng・out・配列の全byteを変えない。最大162枠を予約すれば現在の最大161copyを含められる。

成功時は必要なprefixだけを書き、tailは変えない。out.shuffle.orderは非empty時にcaller配列を指し、empty295/non295ではNULL。has_shuffleはemptyでも295なら1を保つ。callerは入力としてfate_stepへ渡す間その配列を保持する。Pythonは内部162枠の配列からimmutableなFateAction/tupleItemSourceへcopyし、native pointerの寿命を返却objectへ持ち込まない。

公開 `fate_sampler_rule` は次のheader定数に一致する。

```text
experimental:privileged-const-owned207-main-owner-token-local-fate-sampler;caller-owned-chance-rng-separate-from-game-and-policy;splitmix64-rejection-bounded10-model287-plus-ticket-uniform-local-not-published-rates;291-to294-one-uniform-alive-input-seat-recipient-including-owner-and-allies-singleton-draw;296-one-uniform-present-guardian1-to10-per-alive-input-seat-including-existing-type;295-alive-input-seat-raw-copy-source-qualified-manifest-excludes-only-selected-owner-token207-no-reserved-gifts;295-backward-fisher-yates-from-N-to2-one-bounded-i-draw-per-swap-empty-and-singleton-no-draw;used-passive-compatible-fake-and-other207-copies-eligible-dead-copies-excluded;confused-owner-allowed-launch-debit-only-in-fate-step;prepare-fate-validation-before-chance-commit-no-reroll-on-domain-overflow;no-match-state-game-policy-rng-decision-or-public-event-mutation;sampling-failure-preserves-chance-stream;sample-and-step-separate-transactions-late-execution-failure-possible;external-chance-rng-not-in-match-snapshot-controller-checkpoints-it;normal-player-sampler-still-does-not-select207;no-automatic-confusion-or-earth-moon-general-selector;all-outcome-recipient-guardian-and-permutation-laws-local-not-official-server-equivalence
```

### v44の検証

[一次資料と独立ローカルreferenceの生成器](scripts/official_fate_sampler_oracle.cjs) / [保存report](data/official-v2/fate-sampler-client-v44.json) は、実Reference等5ケースと、**別区分のLOCAL BigInt golden17件**を保存する。全10ticket、zero-word rejection、owner/ally宛先、0/1/7/161copy、dead除外・他ownerの同token0・別207保持、9人分guardianを含む。これは公式chooser goldenではない。reportは93,246 bytes、SHA256 `a9a9ff64258836e073b713f84b3954c59149ec0190d2182ceba501a9663ce207`、rootの `--check` はbyte一致した。

Release CTest **143/143（41.92秒）**、ASan/UBSan CTest **103/103（99.69秒）** が全て通過した。native198関数とPython136メソッドは各1回・重複なしで登録し、C API/Pythonのfocused2件も初回で通過した。README例の `290 0` → `main 1 1 11400714819323198527` も実行して一致した。suite時間を速度差や公式server適合数へ読み替えない。

新native5群では全10ticketと既存executorへの適用、pure queryとzero-word rejection、alive順の自己/味方/敵対象と全guardian、混乱countのsafe整数境界・289のno-reroll失敗、0/1/2/143/161copyのsource-qualified manifest、別owner token0/別207/deadの区別、phase/token拒否、late token枯渇とchance copy-commit、snapshot＋外部RNG再開、独立BigInt17goldensの全payload/next_rng一致を検査した。

Cは10固定goldenの64byte全header、NULLと容量上限、short0/2の全byte保持、prefix/tailとpointer、empty/singleton295、289overflowを検査する。Python新3件は独立参照による256seed/全10結果と対象・guardian、0/1/161copyのimmutable tupleとnative bufferの寿命分離、全phase/strict token・seed、無再抽選・未来step失敗を検査した。抽選結果をplayer選択へ流すことなく、既存Fate/grant proofとゲームRNGを保つことを別々に確認する。

通常/canonical/payload/global現象/targeted現象/shuffle/confusion/fate/devil/sacrifice/明示grant/grant sampler/Fate pool/新Fate samplerの各2人数、計28組で2人100,000局またはscene・9人10,000局またはsceneを1worker/4workersで実行し、worker数・時間・速度以外の全fieldが一致した。Release/SAN小行列15組もchecksumを含む非時間fieldが一致した。新helperを呼ばない既存13familyのgameplay fieldはv43と同値で、通常/canonical/payloadはchecksumも維持した。その他の旧版比較ではSnapshot.version44由来のchecksumを除き、外部chance/action、pool/referenceのversion非依存checksumは引き続き比較した。worker間・Release/SAN間では全checksumを比較する。

新 `godfield_fate_sampler_bench`（CTest `official_fate_sampler_benchmark 600 9 42 2`）は全choiceをhelperに任せ、一つのowned207の起動から全response/grantを経てmainまたは終局/cutoffまで進める。各sample前に2restore clonesへ同chance seedを渡し、action/next_rng/Match不変を照合し、実step成功後だけchance streamをcommitする。include_fate/external_grants、混乱、複数207、他owner token0、used/fake/passive、dead、postconsume0/1copyを含み、97scene毎はmax_decisions2でcutoffを検査する。600scene以上で10結果・自己/敵のrecipient、3人以上では別の味方recipientも実際に選ばれることを検査した。normal207 command/通常policyの除外、sourceなしframeと実source0、既存Fate/grant proofとowner/GF/混乱減算も照合する。

| ローカルFate抽選・seed42 | scenes | 結果種数 | 自己 / 別味方 / 敵対象 | 入替copy | chance checksum | action checksum | world checksum |
| --- | ---: | ---: | ---: | ---: | --- | --- | --- |
| 2人 | 100,000 | 10 | 19835 / 0 / 20061 | 200021 | `2423874525599104466` | `9734039227367663945` | `11787821991074239004` |
| 9人 | 10,000 | 10 | 435 / 593 / 2959 | 92642 | `6605635147833536315` | `10123717752832217994` | `17117080911192469713` |

9人600sceneは2292decisions、1249responses、443grant、自己/別味方/敵対象32/38/159、545guardian割当を含む。295は57scene/5282copy（used500）、empty1・singleton0、他owner token0参照444を検査した。混乱launch減算400、revival147、redraw131、cutoff probe7も含む。chance/action/world checksumは `18250023012487255026` / `18054689840843982637` / `4349042520208376490` でRelease/SAN一致。world hashは選択counter等を含む部分hashであり、全private proofの一致は各checkpointのrestoreで別に検査する。このfixture統計はローカルpolicyのもので、公式の現象分布やserver対局の実測ではない。

### Pythonで配布されたひもの結果を抽選する例

この例はplayer/controllerがtoken0を使うと決めた後、chance側が結果を選ぶ。seed42のローカル結果は竜巻290で、responseを必要としない。responseが生じた場合は既存の応答APIで進める。

```python
from official_match import Match, Config

with Match(seed=0x14e9cbeb5fbb6a39,
           config=Config(initial_hand=1, include_fate=True)) as game:
    chance_rng = 42
    action, trial_rng = game.sample_fate(0, chance_rng)
    print(action.outcome.model, game.status.decisions)
    # 290 0
    game.fate_step(action)
    chance_rng = trial_rng
    print(game.status.phase, game.status.owner, game.status.decisions, chance_rng)
    # main 1 1 11400714819323198527
```


## v45：混乱中のローカル行動サンプラー

`official-execution-subset-v45` は `Command sample_confusion(uint64_t& chance_rng) const`、Cの `gfo_match_sample_confusion`、Pythonの `sample_confusion(chance_rng) -> (Command, next_rng)` を追加する。正のconfusion_countを持つmain ownerに限り、既存のローカル通常方策と同じ候補生成でコマンドを作り、既存 `confusion_step` で実行できる。**公開資料に混乱専用の選択分布はなく、この方策を公式自動選択や全合法手の一様抽選とは呼ばない。**

Snapshot45は旧1〜44を拒否し、公開viewは `official-execution-view-v43`。scope、Config24/Command184/Status248、Match ABI2とphase0〜4はv44と同じ。既定236/499・任意237/500の配布、通常player sampler、明示confusion command、Fate/grant APIを保つ。旧37profile・台帳pins・過去のreport/測定を保存し、学習器は追加しない。

### 再利用する方策と実行の境界

| 項目 | v45の契約 |
| --- | --- |
| 入口 | mainかつactor=owner、そのownerのcount>0。count0/response/purchase/grant/finishedではdraw前に拒否 |
| 候補と重み | visible faceを使う通常proposal列の順序・多重度を維持。追加品coin、MP/FREE、target、SELL商品と相手、EXCHANGE配分も共通builderを使う |
| 終末 | 同じdiscard proposal位置をactive時だけsacrifice9へ写す。候補順・選択回数の追加はない |
| 207 | 通常proposalと同じくFate起動は除外する。discard/trade等は既存規約、Fate使用は別の明示Fate APIで行う |
| 応答・購入・補充 | この入口では生成しない。通常の応答/購入sampler、既存grant APIで進める |
| countと費用 | sample中はcount不変。accepted confusion_stepのlaunchで一度だけ減算し、body/cutoff/死亡/復活/元の手番進行を既存executorが処理する |

実装は通常samplerのbuilderを共有し、正のcountに対するgateだけを専用入口で許可する。実Matchのcountを0にしたり、private履歴やproofを書き換えたりしない。コマンド完成後にconfusion用prepareで合法性を検証してからcallerのchance_rngだけをcommitする。失敗時に再抽選で分布を変えない。

world、ゲームRNG、player policy RNG、count、decision、journal、public eventはsampleで変えない。公開追加は静的 `confusion_sampler_rule` だけで、future command、seed、残りの数字のcountは出さない。sampleとstepは別transactionであり、late update/token/GF失敗があり得るため、driverはchance copyからsampleしstep成功後だけcommitできる。Match snapshotとともに外部chance seedもcontroller側へ保存する。

通常sample_actionは正の混乱mainを従来どおり拒否する。response/purchaseでownerの混乱countが残っていても、通常APIの操作では減算しない。公式の混乱候補・MP/target分布・反応や購入の自動化、Earth/Moon一般payload選出、公式Dream全分布・実体不適合入力、全経路の悪魔置換と権威的server scheduleは残件である。

公開 `confusion_sampler_rule` は次のheader定数に一致する。

```text
experimental:privileged-const-positive-confused-main-owner-only-local-command-sampler;caller-owned-chance-rng-separate-from-game-and-player-policy;shared-visible-normal-proposal-generator-same-order-multiplicity-extra-item-coins-free-mp-target-sell-and-exchange-laws;not-uniform-over-all-legal-actions-and-not-an-official-confusion-distribution;count-gate-bypass-only-no-count-zeroing-or-history-rewrite;final-confusion-prepare-validation-before-chance-commit;no-match-state-game-rng-policy-rng-count-decision-journal-or-public-event-mutation;sampling-failure-preserves-chance-stream-no-reroll;apply-with-existing-confusion-step-one-launch-debit-before-body-and-cutoff;response-purchase-and-grant-not-sampled-here-use-existing-apis;207-fate-generation-excluded-existing-explicit-fate-api-remains;active-threshold-discard-proposal-maps-to-sacrifice-with-same-draws;sample-and-step-separate-transactions-late-step-failure-possible;external-chance-stream-not-in-match-snapshot-controller-checkpoints-it;default-player-sampler-and-existing-manual-confusion-commands-unchanged;server-command-selection-mp-target-and-event-schedule-unverified
```

### v45の検証

[証拠と独立ローカルreferenceの生成器](scripts/official_confusion_sampler_oracle.cjs) / [保存report](data/official-v2/confusion-sampler-client-v45.json) は、JS/JA/EN/cardsとv37のarchive hashを照合する。v37の実receiver検証11ケースを**継承**し、今回の新しいreceiver実行は0件と明記した。生存者+3、useAttackItems後の1減算、補助/防御event単独で減算しないこと等の事実を、自動方策の証明へ読み替えない。

別区分のLOCAL BigInt golden18件は、empty handとraw{199}の4候補、閾値null/0/17/18、token0/maxsafe、play時の既選択baseへの追加coinを固定する。候補数1/4のboundでは最初のzero wordを拒否せず、singletonも1drawする。reportは38,669 bytes、SHA256 `0a794fd302c6b556c0f177d1098d0ed877c8d9457f3972b8db6dabeb5cf4c9ee`、rootの `--check` はbyte一致した。複雑な全方策をgeneratorへ鏡写しする代わりに、豊富なfixtureでは通常count0版の独立sceneと結果・RNGを比較する。

Release CTest **144/144（55.54秒）**、ASan/UBSan CTest **104/104（122.71秒）** が全て通過した。native203関数とPython139メソッドは各1回・重複なしで登録し、C API/Pythonのfocused2件も通過した。README例の `pray 0` → `main 7 1 True 11400714819323198527` も実行して一致した。suite時間を速度比較や公式serverの適合数へ読み替えない。

新native5群ではsimple4proposal/empty/zero-word、閾値によるkind差替えとextra coin、9種のrich fixture×512seedでcount0版とCommand/next_rng一致、MP0+FREE・交換/売買・used18・Dream・207skip、本物の289履歴から実行した時のlaunchだけの減算、全phase拒否、late update失敗時のcount/盤面rollbackと外部seed copy-commit、独立BigInt18goldensを検査する。比較用worldはcountだけを0にして別Matchへload_sceneするもので、実samplerや履歴を改変しない。

Cは512seedの184byte全Command/RNG parity、NULLと出力canary、zero-word、count0拒否とlate failureを検査した。Python新3件はMP/FREE/資源交換/売買/Dream/threshold9/207の除外、全phase・strictseed、実行時だけの1減算、late update/token/GF rollbackを確認する。

通常/canonical/payload/global現象/targeted現象/shuffle/confusion/fate/devil/sacrifice/明示grant/grant sampler/Fate pool/Fate sampler/新confusion samplerの各2人数、計30組で2人100,000局またはscene・9人10,000局またはsceneを1worker/4workersで照合し、worker数・時間・速度以外の全fieldが一致した。独立familyは最大2系統を並行実行し、時間を速度比較には用いない。Release/SAN小行列16組もchecksumを含む非時間fieldが一致した。

新helperを呼ばない既存14familyのgameplay fieldはv44と同値で、通常/canonical/payloadはchecksumも維持した。その他の旧版比較ではSnapshot.version45由来のchecksumを除き、外部chance/action、pool/reference、Fateのmanifestのversion非依存checksumは引き続き照合した。worker間・Release/SAN間では全checksumを比較する。通常samplerの共有化が既存の候補順・乱数順を変えていないことを確認する検査である。

新 `godfield_confusion_sampler_bench`（CTest `official_confusion_sampler_benchmark 600 9 42 2`）は289を1回または2回適用し、1pulseでは最大4round、2pulseでは最大3roundを進める。10fixture列にempty/18usedMC/Exchange/MP0+FREE/SELL/BUY/active sacrifice/207only/解除/HP低い208・112-richを置く。各confused mainで2unchanged restore clonesとcount0 rawworldの別load_sceneを同seedで比較し、accepted confusion_step後だけchanceをcommitする。無混乱main/response/purchaseは通常policy、grantはnormal0で進め、全checkpointを復元する。include_fate/external_grantsを混在し、通常main・response・purchase・grantと主要command/FREE/used/解除の実出現を600scene以上で要求する。

| ローカル混乱抽選・seed42 | scenes | 混乱 / 通常main | response / purchase / grant | chance checksum | action checksum | world checksum |
| --- | ---: | ---: | ---: | --- | --- | --- |
| 2人 | 100,000 | 639,161 / 29,694 | 386,278 / 8,569 / 282,753 | `11178267938257247156` | `14037867797094491107` | `4857307360174753880` |
| 9人 | 10,000 | 280,936 / 14,660 | 583,284 / 12,840 / 168,448 | `13860660018009438040` | `14649245508736495735` | `677400468030987553` |

9人600sceneでは64289decisions、混乱launch16870（sampler照合・count減算とも同数）、通常main874、response34830、purchase727、grant10088を含む。FREE合成249・used選択3512・解除600、empty60scene・18used60scene、revival588・redraw2446を検査した。chance/action/world checksumは `5906155505931735076` / `8664201943862005947` / `17601305694920062430` でRelease/SAN一致。world/count＋選択counterの部分hashと、全private proofのrestore完全一致を区別する。このfixture分布はローカル通常方策の転用を検証するもので、公式の混乱行動分布やserver対局統計ではない。

### Pythonで混乱中のmainを一度進める例

空手札なのでこのローカル方策の候補はprayだけになる。sourceのcount3はsampleでは変わらず、実行後に2となる。公開viewは残数を出さずconfused boolだけを示す。

```python
from official_state import OfficialState
from official_match import Match, Config

raw = {"gf": 0, "players": [
    {"id": 0, "hp": 40, "mp": 10, "cp": 20,
     "confusionCount": 3, "items": []},
    {"id": 7, "hp": 40, "mp": 10, "cp": 20, "items": []},
]}
with OfficialState(raw) as scene, Match(config=Config(initial_hand=0)) as game:
    game.load_scene(scene, owner=0, seed=7)
    chance_rng = 42
    command, trial_rng = game.sample_confusion(chance_rng)
    print(command.kind, game.status.decisions)
    # pray 0
    game.confusion_step(command)
    chance_rng = trial_rng
    print(game.status.phase, game.status.owner, game.status.decisions,
          game.observe(0)["board"]["players"][0]["status"]["confused"], chance_rng)
    # main 7 1 True 11400714819323198527
```


## v46：Earth/Moonの限定payloadサンプラー

`official-execution-subset-v46` は `GuardianPayloadAction sample_guardian_payload(PlayerId source, uint64_t& chance_rng) const`、Cの `gfo_match_sample_guardian_payload`、Pythonの `sample_guardian_payload(source, chance_rng) -> (GuardianPayloadAction, next_rng)` を追加する。既存external payloadが受理するEarth120/Moon26の範囲で列と対象を生成し、`guardian_payload_step` へ渡せる。**公式一般selectorではなく、カテゴリ以外の重み・列長・追加・対象規約はローカル**である。

Snapshot46は旧1〜45を拒否し、公開viewは `official-execution-view-v44`。scope、Config24/Command184/Status248/GuardianPayloadAction64byte、Match ABI2とphase0〜4はv45と同じ。既定canonical自動Earth285/Moon286は変更せず、今回のhelperを25%機会schedulerへ接続しない。旧37profile・台帳pins・既存report/測定・学習器なしの範囲を保つ。

### 選出規約と現在の範囲

| 項目 | v46のローカル規約 |
| --- | --- |
| source | main境界の生存Earth/Moon所持者。guardianをsource状態から推定し、普通ownerと同一でなくてもよい |
| 基底 | 既存match_guardian_payload_supportedのEarth120/Moon26をID順で列挙し、一様に1種。giftRate重みは使わない |
| 追加候補 | same-domain・supported・既存trade/danger禁止・そのprefixへのcan_append_mainを満たす全ID。基底capabilityで追加候補を絞らない |
| 続行 | 候補が無ければcoinなしで終了。有ればbounded2の0で終了、1なら候補から一様1種を追加。重複ID可 |
| 上限 | モデル列18個。config.hand_limitと独立し、18到達後のstop coinは引かない |
| 対象 | composed attack_power>0または基底targets_enemy_by_defaultならsourceの生存敵、それ以外は生存味方（source自身を含む）。入力順から一様1人、singletonも1draw |

Earthは武器106＋雑貨14の120基底で、そのうち88基底に26種の追加候補（追加防具124/136/150/162や203等）がある。実appendで2288の2列を確認した。Moon26はこの厳格なsame-domain規約では追加候補が0なので常に単品で、追加coinも引かない。Aura/Wide/FREE等を含む一般MC合成や、取引・通常防具・passive・207・285/286wrapperの全Earth/Moon処理へは広げない。

対象は普通ownerでなく指定sourceを基準に `is_enemy` で分類する。solo同士を同じteam0だから味方とはしない。百分率攻撃はUI default-targetだけで判断せず正のpowerで敵向けとし、58はMP0でもweapon defaultにより敵向けになる。202の符号は未抽選のため非攻撃の味方向けのまま扱う。これらのtarget分布は公式確認済みではない。

### MP・手札・RNG・実行

sourceのMP、所持手札、空枠、confusion、Fog/Dreamをeligibilityへ使わない。MP0でもMoonの有料MCを選べ、実手札を持たないmodel列なので物理tokenを生成・消費しない。先頭58のlaunchMP×2とMP0、202の符号、counter/死亡/208/112/補充は、既存payload executorのゲームRNG/実行に残す。通常MC fee、operation gift、owner tick/GF/autoを新しく加えない。副作用による資源変動と、一般の使用料を支払うことは区別する。

helperは外部chance seedのcopyで全列と対象を生成し、最後に既存prepare_guardian_payloadを通ってからseedをcommitする。候補不足や検証失敗で再抽選せず、Matchとseedを保つ。成功sampleもworld/game RNG/player policy RNG/decision/journal/public eventを変えない。sampleとstepは別transactionなので、late update/token等の実行失敗までまとめて扱うdriverはtrial seedを使い、step成功後だけcommitする。checkpointにはMatch snapshotと外部chance seedの両方を保存する。

公開追加は静的 `guardian_payload_sampler_rule` のみで、future列・対象・seedは出さない。source選出や自動機会判定もhelperには含めず、通常samplerや既定auto schedulerは従来のまま。全公式Earth/Moon selector、Dream全分布・実体不適合入力、全補充経路の悪魔置換と権威的server scheduleは残件である。

公開ruleは次のheader定数に一致する。

```text
experimental:privileged-const-main-boundary-alive-earth-or-moon-source-inferred-not-necessarily-owner;caller-owned-chance-rng-separate-from-game-and-policy;uniform-id-ordered-existing-supported120-earth-or26-moon-base;not-giftrate-weighted-no-source-mp-hand-slot-confusion-or-curse-filter;prefix-under18-scan-id-order-same-domain-supported-ordinary-append-excluding-trade-and-danger;empty-extra-candidates-stop-without-coin;nonempty-bounded2-zero-stops-one-appends-uniform-candidate-duplicates-allowed;earth88-bases-have26-extras-including-addon-armor-moon-no-extras-and-singleton;18-cap-independent-of-config-hand-limit-no-coin-after-cap;hostile-iff-composed-attack-power-positive-or-base-default-enemy-else-friendly;one-uniform-live-input-seat-enemy-of-source-or-ally-including-source-target-singleton-still-draws;202-sign-and58-mp-effects-deferred-to-body-no-game-rng-peek;final-existing-guardian-payload-prepare-before-chance-commit-no-reroll;no-match-state-game-rng-policy-rng-decision-journal-or-public-event-mutation;sampling-failure-preserves-chance-stream;sample-and-step-separate-transactions-late-step-failure-possible;controller-saves-external-chance-stream-alongside-match-snapshot;existing-64byte-payload-output-no-inventory-tokens-or-mp-payment-added;canonical-auto285286-and-default-behavior-unchanged-not-integrated-into-opportunity-scheduler;local-narrow-subset-not-all-earthmoon-trade-armor-passive-fate-wrapper-or-official-server-selector-equivalence
```

### v46の検証

[一次appendと独立ローカルreferenceの生成器](scripts/official_guardian_payload_sampler_oracle.cjs) / [保存report](data/official-v2/guardian-payload-sampler-client-v46.json) は、実client appendで基底120/26・Earth88×26=2288追加pair・Moon0を照合し、LOCAL BigInt20goldensの全prefix候補も検査する。受信処理とMP表示の事実はv33reportの固定hashからの継承であり、今回の受信再実行とは呼ばない。

独立LOCAL goldenはsource≠owner、MP0/99同choice、solo/ally/self/dead/singleton、58/202/MC/百分率、重複追加・full18・zero-word rejectionを含む。実appendに一致する候補であっても、その候補からの一様選出やcoin確率を公式selectorの証明にはしない。reportは95,565 bytes、SHA256 `9213e59fb379d621a3a442428119b00ca2c23ddc162eed0e01f0b909bdce35bb`、rootの `--check` はbyte一致した。

Release CTest **145/145（69.28秒）**、ASan/UBSan CTest **105/105（141.95秒）** が全て通過した。native208関数とPython142メソッドは各1回・重複なしで登録し、C API/Python focused2件も初回で通過した。READMEのMP0 Moon例も実行し、`moon (235,) 42 0` → `main 7 17 0 50` が一致した。suite時間を速度比較や公式server適合数へ読み替えない。

新native5群は120/26全基底ticket、source基準の敵/味方、全26extrasと追加防具・重複、独立full18 vectorとhand_limit1、MP/手札/curse/confusion/gameRNG不変な選出、MP58がstep時だけ34power/MP0となること、phase/source/自然終局拒否、late update失敗とchance copy-commit、snapshot＋外部seed再開、独立BigInt20goldensを検査する。Moonのextra draw0と、shapeと物理hand上限を混同しないことも確認した。

Cは11固定goldenの64byte全header・padding/canary、NULL/source/phaseと失敗時の全出力保持を検査する。Python新3件は全120/26base witnessと独立参照、全26extras/18列/重複、source0/非owner17・solo/自己/味方/敵/dead、MP0/99/empty/full/giftpool、先頭58のbodyだけのMPdrain、strictinputとlate step rollbackを検査する。公式未確認のselectorと、既存payloadの効果解決を別々の範囲として扱う。

通常/canonical/payload/global現象/targeted現象/shuffle/confusion/fate/devil/sacrifice/明示grant/grant sampler/Fate pool/Fate sampler/confusion sampler/新guardian payload samplerの各2人数、計32組で2人100,000局またはscene・9人10,000局またはsceneを1worker/4workersで照合し、worker数・時間・速度以外の全fieldが一致した。独立familyは最大2系統を並行実行し、時間を速度比較には使わない。Release/SAN小行列17組もchecksumを含む非時間fieldが一致した。

新helperを呼ばない既存15familyのgameplay fieldはv45と同値で、通常/canonical/payloadはchecksumも維持した。その他の旧版比較ではSnapshot.version46由来のchecksumを除き、外部chance/action、pool/reference、Fate manifestのversion非依存checksumは引き続き比較した。worker間・Release/SAN間では全checksumを照合する。既定auto285/286・通常配布・既存callerの振る舞いを変えていないことの回帰検査である。

新 `godfield_guardian_payload_sampler_bench`（CTest `official_guardian_payload_sampler_benchmark 600 9 42 2`）はsource席・Earth/Moon・MP0/1/99・team/curse/confusion/holdingを変え、1payloadから全response/grantを解決する。2restore clonesの同choice/nextchance/Match不変と、MP/手札を変えてhand_limit1にした別sceneでも同choiceとなることを比較する。全prefix、source-relative target、Moon singleton/high-cost、先頭58/14/202、sourceの物理消費や一般MP feeがないこと、owner GF/tick/autoの維持、全checkpoint restoreを検査する。600scene以上でcompound/自己/敵/highcost/nonowner、3人以上では別味方、10000以上で全146基底と58/14/202の出現を要求した。full18は独立native/API vectorで確定的に検査し、稀な偶然に頼らない。

| ローカルguardian選出・seed42 | scenes | 基底種数 | Earth複合列 / 追加数 | 自己 / 別味方 / 敵対象 | chance checksum | action checksum | world checksum |
| --- | ---: | ---: | ---: | ---: | --- | --- | --- |
| 2人 | 100,000 | 146 | 18402 / 37009 | 14623 / 0 / 85377 | `14542496587766292104` | `4504792565828746021` | `9409136792974185313` |
| 9人 | 10,000 | 146 | 1810 / 3648 | 1070 / 362 / 8568 | `15147013353315001003` | `1658180899462700799` | `17891271245382900376` |

9人600sceneでは2406decisions、Earth300/Moon300、compound116列/233追加、重複9・setter列17、source≠owner528、自己/別味方/敵73/16/511を含む。MP不足Moon200、58/14/202は4/2/4、revival23・redraw184・guardian summon14も検査した。chance/action/world checksumは `2777545096584598747` / `17215112275432670988` / `1434169688558901987` でRelease/SAN一致。worldと選択counterの部分hashと、全private proofのrestore完全一致を区別する。このfixture統計を公式の選出分布や自動機会schedulerの実測とは呼ばない。

### PythonでMP0のMoon payloadを生成する例

この固定seedはローカル基底26種から235を選ぶ。普通ownerは7、sourceは別の42で、選ぶ列は手札に依存しない。既定のMoon自動行動を差し替える例ではない。

```python
from official_state import OfficialState
from official_match import Match, Config

raw = {"gf": 17, "players": [
    {"id": 42, "hp": 40, "mp": 0, "cp": 20, "guardian": "moon", "items": []},
    {"id": 7, "hp": 99, "mp": 99, "cp": 20, "items": []},
]}
with OfficialState(raw) as scene, Match(config=Config(initial_hand=0)) as game:
    game.load_scene(scene, owner=7, seed=7)
    chance_rng = 0x733c8152bee07cd1
    action, trial_rng = game.sample_guardian_payload(42, chance_rng)
    print(action.guardian, action.models, action.target, game.status.decisions)
    # moon (235,) 42 0
    game.guardian_payload_step(action)
    chance_rng = trial_rng
    board = game.observe(42)["board"]
    print(game.status.phase, game.status.owner, board["gf"],
          board["players"][0]["status"]["mp"], board["players"][0]["status"]["hp"])
    # main 7 17 0 50
```


## v47：任意設定でEarth/Moonの限定選出を自動発動へ接続

`official-execution-subset-v47` は `MatchConfig.local_guardian_payloads`（既定false）を追加する。trueの場合、通常主動作後の既存の自動守護神処理で、Earth/Moonの25%機会が成功した時だけv46の限定選出器を使う。敵sourceの入力席順、owner病気tick後・予約補充前、他8守護神のcanonical処理は従来どおり。falseのEarth285/Moon286と既存callerの挙動を保つ。**公式のEarth/Moon全処理・選出分布を再現したという意味ではない。**

| 項目 | v47の契約 |
| --- | --- |
| 選出対象 | 地球120基底・月26基底、追加候補・上限18・対象はv46と共通。地球の一様基底＋継続coin、月単品はローカル規約 |
| 有効な機会 | 普通ownerの生存敵sourceだけ。25%の判定成功後に選出し、失敗時に基底・追加・対象を引かない。owner自身・味方・死者を追加しない |
| 乱数 | 自動の機会・列・対象・bodyはMatchのgame RNGを使い、主動作のanchorと応答journalから再生する。外部const helperは引き続きcaller-owned chance RNGを使う |
| 手番と資源 | 仮想列は実手札を生成・消費せず、通常MC使用料・追加operation gift・内部decisionを加えない。元の主動作と各response/grantは従来のdecisionを使う |
| 58 / 202 | 選出後、58はsource launch MPを保存して1回だけMP0へ。202は対象選出後に符号を1回抽選。選出条件にはMP・手札・空枠を使わない |
| 14 | 同じ初期対象へ2つのfresh frame。death→counter FIFOを挟み、保存した列・guardian・MP・符号から作る。1発目の属性filter/反射/redirectを2発目へ継承しない |
| 死亡と離脱 | owner死亡は残hitと将来sourceを中止、source/target死亡は残hitを中止。GP離脱は既に選ばれた列を書き換えない。即時自己効果で自然終局したら補充/GF/次ownerへ進めない |
| ログと計数 | `auto_guardian_actions` はcanonicalとpayloadの合計。`guardian_payload_actions` と `last_guardian_payload` は外部入口専用のまま。公開 `last_auto_guardian_payloads` は起動済みのsource/guardian/models/targetだけ |

元の主動作batchとそのmodels/targets/cursorを置き換えず、private `automatic_payload` に未完了hitとlaunch値を保持する。pending responseと予約補充のproofは、主動作開始時のgame RNG・全応答・grant順序を再生し、選出済み列、cursor、MP、符号、ログまで全snapshot一致を要求する。公開viewに将来のsource列、seed、launch MP、符号、cursor、pending FIFOを追加しない。Fogのguardian変更は従来のevent-time viewer maskを使う。

複数の201/239自己召喚や202自己減少が内部decisionを増やさず起こり得るため、snapshotの召喚・GP離脱回数の安全上限を複数sourceに対応させた。普通SELL/BUY/Danger後の自動205/206除去で、元の取引/危険物eventと除去eventが同じ手番の公開要約に共存する場合も検証する。`last_removal` は最後に解決した除去の要約であり、body総数の台帳ではない。

Snapshot47は旧1〜46を拒否する。公開viewは `official-execution-view-v45`、Match ABI2、phase0〜4、Config24/Command184/Status248/Payload64 byteは維持する。C Configの旧reserved byte20をstrict0/1の `local_guardian_payloads` に割り当て、byte21〜23は0を要求する。Python `Config(local_guardian_payloads=True)` はbool以外を拒否し、reset後も設定を保持する。旧37profile・pins・保存report・過去計測・学習器を含めない構成は維持する。

静的 `guardian_payload_auto_rule` は次のheader定数に一致する。v46 helperの静的ruleも「既定canonical維持・任意設定の自動処理とは別入口」と明確化し、外部helperの選出列やchance消費自体は変えない。

```text
experimental:opt-in-local-guardian-payloads-default-false;existing-enemy-input-order-quarter-opportunity-after-owner-tick-before-deferred-gifts;only-successful-earth-moon-opportunities-use-shared-local120-26-selector;automatic-selection-and-body-use-game-rng-replayed-from-main-anchor;external-const-helper-keeps-caller-owned-chance-stream;no-additional-decision-source-items-ordinary-mp-fee-or-operation-gift;58-saves-launch-mp-and-drains-once-202-draws-one-sign-after-target;14-two-fresh-original-frames-with-death-then-counter-fifo-between-hits;fixed-launch-models-guardian-mp-sign-through-redirection-and-departure;stop-remaining-and-future-sources-on-main-owner-death-stop-remaining-on-source-or-target-death;self-nondamaging-body-immediate-with-shared-death-pump;launched-payload-log-only-no-future-rng-or-private-launch-values;external-payload-counter-unchanged-auto-counter-includes-both-kinds;other8-guardians-and-default-canonical285286-unchanged;uniform-base-geometric-extras-and-target-distribution-local-not-official-server-equivalence
```

### v47の検証と残件

[独立LOCAL stage reference](scripts/official_guardian_payload_auto_oracle.cjs) / [保存report](data/official-v2/guardian-payload-auto-local-v47.json) はBigIntの21例を収録する。四分の一機会の境界・zero word rejection、単品と複合14/58、MP0/17/99、202の符号と条件付きGP離脱、月のsingleton、地球201＋月239の同一decision自己召喚を含む。reportは56,877 bytes、SHA256 `4400ef264fb7e30f55e42682711dd1fe41d67a3243a05fb806aa4eba3512eb7b`。v46の一次append証拠を固定hashで継承し、今回新しく公式selectorやreceiverを発見・実行したとは主張しない。referenceは記載した抽選stageで止まり、対局全体・response/counter/gift/GFを模倣しない。

Release CTest **146/146（68.70秒）**、ASan/UBSan CTest **106/106（136.50秒）** が通過した。native216関数・Python144メソッドを重複なしで登録し、C API/Python focused2件も初回で通過した。新native8群は全146基底の自動到達/完了、他8守護神のflag正規化全snapshot同値、外部const helper不変、58の反射後も保存MP威力を維持、14のcounter FIFOとfresh次hit、owner/source死亡による打切り、複数即時召喚、tick履歴、SELL/BUY/Dangerと除去の共存、private launch/log/乱数の偽造16種拒否、late update失敗rollback、cutoff、外部補充proofを検査する。独立BigInt stageからは14、MP0/17/99の58、202の正負/GP離脱、月235、同一decisionの201＋239をnative実行と照合し、即時自己減少による自然終局も検査した。

C/PythonはConfig byte20・予約byte21〜23・strict bool/reset保持、default/explicitfalseのcanonicalMoon同値、MP0からのEarth複合列とMoon、非owner対象、0追加decision/MP fee/source消費なし、公開ログ、応答失敗atomicを検査した。READMEの通常祈り→月235自己回復例は実行し、`main 0 18 50` が一致した。suite時間は速度比較や公式適合数ではない。

通常/canonical/payload/global現象/targeted現象/shuffle/confusion/fate/devil/sacrifice/明示grant/grant sampler/Fate pool/Fate sampler/confusion sampler/guardian payload sampler/新auto payloadの各2人数、計34組で2人100,000局またはscene・9人10,000局またはsceneを1worker/4workersで照合し、worker数・時間・速度以外のfieldが一致した。Release/SAN小行列18組もchecksumを含む非時間fieldが一致した。独立familyは最大2系統を並行実行し、時間を速度比較へ使わない。

新flagを有効にしない既存16familyのgameplay fieldはv46と同値で、通常/canonical/payloadはchecksumも維持した。その他はSnapshot.version47に依存するchecksumを旧版比較から除き、外部chance/action、pool/reference、Fate manifestのversion非依存checksumを比較した。worker間・Release/SAN間では全checksumを比較する。ログ出力への静的auto rule追加後は、全17benchmarkのruntime metadataがheaderと1回だけ一致し、出力変更前後のgameplay field/checksumが同値なことを確認した。benchmark focused16件もRelease/SANで再通過した（3.08秒/22.56秒）。

新 `godfield_guardian_payload_auto_bench`（CTest `official_guardian_payload_auto_benchmark 600 9 42 2`）は通常prayからmain/endまでを進め、true2clonesの同command/policy RNG/全snapshot、各checkpoint restore、全sceneのfalse対照、20%の他8守護神のみのflag正規化全snapshot同値を検査する。MP0/1/99、team/Fog/Dream/confusion、owner病気tick、208/112・反応品・usedMC、外部補充を含む。原主動作batchを保ったprivate auto context、source消費/一般MP fee/余計なgiftなし、confusion非減少、owner/GFの1回継続も検査する。base種数は新payloadの基底unionであり、起動がないsceneも分母へ含む。新旧world/選択counterの部分hashとは別に、restoreはprivate proof全体を比較する。

| 自動payload・seed42 | scenes | 基底種数 | Earth / Moon | 複合列 / 追加数 | launch checksum | default対照checksum | world checksum |
| --- | ---: | ---: | ---: | ---: | --- | --- | --- |
| 2人 | 100,000 | 146 | 10041 / 10018 | 3623 / 7254 | 2417696602088359053 | 9594325949124055296 | 7593984257819276415 |
| 9人 | 10,000 | 146 | 6664 / 3910 | 2360 / 4729 | 6253263127398765636 | 13508072783508571530 | 16205755181632469720 |

9人600sceneでは3577decisions、payload631（Earth396/Moon235）＋canonical363、compound145列/314追加、重複列15・setter列27を含む。pending517、MP0 Moon pending49・MP不足Moon pending110、58/14/202は4/2/1、counter368・death bow2・revival63・redraw286・summon15も検査した。falseの全600sceneではcanonical Earth/Moon fallback646、他8のみのtrue/false同値120sceneを確認した。launch/default/world checksumは `17428102963242652128` / `14858246853641918064` / `6685247023367793678` でRelease/SAN一致。このfixture統計を公式の発動分布やserver実測と呼ばない。

地球の通常防具/取引/passive/207や一般wrapper、月の一般合成、公式の重み・継続率・対象選択は未対応または未確認のままである。25%機会や発動順序も実験規約であり、この設定はローカル対局を通して限定payloadを使えるようにするもの。Dream全分布・実体不適合入力、全補充経路の悪魔置換・再授与連鎖、権威的server scheduleとの一致も引き続き残件である。

### Pythonで通常手番から月神を自動発動する例

このseedはMoon235の自己回復を選ぶ。ownerの18枚は使用済み奇跡なので祈りが可能で、満杯のため祈り補充の乱数を消費しない。自動発動の列はsourceの手札に依存しない。

```python
from official_state import OfficialState
from official_match import Match, Config, Command

raw = {"gf": 17, "players": [
    {"id": 7, "team": 2, "hp": 99, "mp": 0, "cp": 20,
     "items": [{"id": i, "modelId": 235, "used": True} for i in range(18)]},
    {"id": 0, "team": 1, "hp": 40, "mp": 0, "cp": 20, "guardian": "moon", "items": []},
]}
with OfficialState(raw) as scene, Match(config=Config(initial_hand=0, local_guardian_payloads=True)) as game:
    game.load_scene(scene, owner=7, seed=12)
    game.step(Command(7, "pray"))
    view = game.observe()
    print(view["last_auto_guardian_payloads"])
    # [{'source': 0, 'guardian': 'moon', 'models': [235], 'target': 0}]
    print(game.status.phase, game.status.owner, view["board"]["gf"],
          view["board"]["players"][1]["status"]["hp"])
    # main 0 18 50
```


## v48：公開スナップショットの先頭攻撃を別APIで投影

v48はread-only表示APIの追加世代である。`ActiveAttackState` / `ActiveAttackView` と `project_active_attack(board, attacks)` / `active_attack_json`、Cの `gfo_state_with_active_attack` / `gfo_state_active_attack_json`、Python `OfficialState.active_attack()` を追加した。**対局rulesetはv47、Match Snapshot47・view-v45、既存boardの `official-fresh-view-v1`、catalog/state ABI1・Match ABI2を維持する。** 既存 `view()` の内容や対局の抽選・効果は変えない。新JSONは独立した `official-active-attack-view-v1`、Cの新frame構造体は72byteである。

Pythonはraw `attacks` を全体保存しながら先頭だけをstrictにデコードする。Cは既存boardをowned cloneし、先頭frameを別のoptionalとして保持する（NULLでclear）。`SnapshotState` 自体へserver queueを追加せず、既存board-only APIを保つ。`active_attack()` は攻撃オブジェクトまたはNone、C/native JSONは `{schema,attack}` を返す。raw snapshotやraw widgetの引数を公開viewへそのまま転記しない。

| 表示項目 | 確認したクライアントの規則 |
| --- | --- |
| active | 受信列の先頭だけを使い、後続攻撃・eventsを公開しない。isOverではactive表示を返さず、source/targetのlookupにも進まない |
| source / target | source0も実player0なら通常ID。非終局でsourceがrosterに無い場合は失敗。targetはwire ID>0の時だけ表示し、そのIDも存在が必要。実player0があってもtarget0は表示対象なし |
| guardian / 向き / 商品 | frameのguardian・reversedを保持する。buying_modelは正のtargetと正のモデルIDの両方がある時だけ返す |
| models / summary | 元のカード列は全て保持。空列だけをprayerとし、長さ2以上で先頭が207ならsummary用の列から1個だけ外す。[207]や[207,207,6]を再帰的に展開しない |
| 攻撃判定 | summary先頭の正ATK、先頭category=weapons、または受信の動的ATK>0。追加カードの合計威力が正というだけでは攻撃summaryにしない |
| summaryの数値 | 攻撃時だけpower/hit_rate/elementを返す。正の動的ATKを基底override、正の動的hitRateをrate overrideとし、それ以外は台帳値を使う。非攻撃の潜在合成威力は公開しない |
| response | summary列が非空なら、先頭のcategoryWeaponsをweaponsへ読み替えた分類、合成属性、攻撃判定を返す。guardianの絵だけから分類を決めない |
| 列の長さ | activeカード列には自己手札の18枠制限を流用しない。19モデルの実表示を確認した。任意の長さがserverで合法との主張ではない |

`cards` は特別カードで実際に表示される値だけを持つ。58は受信ATKとfrom_mp、112は最終のcard ATK/rate、181/184は非0の受信ATK、187は非0のMP、188は非0のCP、counterCurseはカードのcurseを返す。一般カードには未描画のwire数値を付けない。58や184の受信値を再び×2したり、盤面MPから再計算したりしない。

112は動的ATK>0の時だけ受信ATK/rateをcard表示へ使い、それ以外は台帳1/25を表示する。raw0/0とraw1/25の同じ表示は同じviewへ正規化し、隠れたoverride有無を追加しない。summaryのrate判定は別なので、動的ATK0/rate75ではcardは1/25、summaryは1/75になる。負のwire値も、描画される分岐と無視される分岐を別々に検証する。

unknown source/positive targetは、実fresh callerのroster lookupが例外となるためnative/C queryでも拒否する。下位 `fu` に人工的なnull sourceを渡すと描画できることと、full freshの受理を混同しない。typed adapterは既知fieldのexact型・JS-safe整数・既知model/enumを検査するため、ブラウザdecoderの不正型coercionや終局時に描画されない不正モデルまで受け入れる契約ではない。checked int64合成のoverflowも拒否し、JSの安全域外の丸めまで一致とは主張しない。

この表示は現在のactor・手番・server受理・未来の命中/乱数・与damageを認定しない。response辞書は既存 `ResponseContext` へ渡せるが、選択照会と送信権限は別である。Pythonの非空attacks付きsceneは従来どおりMatchへimport不可で、Cも新しくattachされたactiveを `load_scene` へ渡した場合はunsupported(-5)で全出力・Matchを保持する。表示frameを実行checkpointへ読み替えない。

### v48の一次照合と検証

[実クライアントoracle](scripts/official_active_attack_oracle.cjs)、[保存report](data/official-v2/active-attack-client-v48.json)、[native用TSV](data/official-v2/active-attack-vectors-v48.tsv) はp6/tn decode、fu、iI/gf2 summary、iP cardラベル分岐、fq/aT fresh callerとroster lookupを実行する。400 frame例（296単体＋順序付き16＋動的26＋override36＋card境界26）、fresh46例（expected lookup error22）、decoder10例、人工null source2例を区別した。nativeは443行（expected error22を含む）で公開JSONが全て一致した。

card/summaryの数値・分岐を実行する一方、text formatterはnumeric/symbolic sink、DOM・board widget・rating・network・command activationはscaffoldingである。画面全体やlocaleの字形・過去イベントの再現とは数えない。新reportは828,308bytes、SHA256 `6820f68a525bc409ae0e132ab1d6e65b79596168a356af32656e49745fbe3bdf`、TSVは218,259bytes、SHA256 `01ae0033e11e223a51bd4425f1376a810cc918ccc5a0fc94d44460eb82edfe30`。clientはv2固定hash `5ece06c224b6a7a5d877eb45d22ea18b82a03aaf1b7161cbbdf6a1e23ceab2cd`、cardsは `8e465e2d296a2dd039d6e3b6e22956447c1fc4a5572a31e2597cdf20c551dc2a`。旧pins・report・版別の計測は保存する。

Release CTest **148/148（41.72秒）**、ASan/UBSan CTest **108/108（135.31秒）** が通過した。新native8群、実client由来443vectors、State Python12メソッドとC API境界を検査し、既存Match native216/Python144群も維持する。Release focused8件も初回で通過した（20.22秒）。新unitは207の1回だけの除去、58/112/各counterの最終card表示、負wire、112同ラベルのview同値、categoryWeapons、positive override、source/target照会順、ending/先頭のみ/19列、未描画値とboardの非干渉、strict入力とchecked overflowを検査する。

Cは72byte layout、旧ABI幅、deep copy/NULL clear、short/NULL/canary/全出力保持、物理extent上限を検査する。Pythonは先頭だけのexact型検査とraw tail保持、未知endpoint時のquery失敗、ending時のlookup抑止、旧view不変を検査する。C/Pythonのpending scene import拒否も回帰検査した。README例を実行し、`36 fire` / `True` / `official-fresh-view-v1` が一致した。一次照合は固定clientの表示分岐に対するものであり、suite数や時間をserver適合数・速度比較へ置き換えない。

既存17familyの2人100,000局またはscene・9人10,000局またはsceneを1worker/4workersで照合し、計34組で時間・速度・worker数以外の全fieldが一致した。Release/SAN小行列18組も全checksumを含む非時間fieldが一致した。さらにv47保存行列の全104recordに対し、全gameplay fieldと全checksumが同値だった。旧行列の一部に無かった静的 `guardian_payload_auto_rule` の出力有無だけは、v47最終のmetadata整備を踏まえ旧版比較時に正規化した。現worker間・Release/SAN間ではmetadataの有無も含めて比較している。

| 既存対局・seed42 | 2人100,000局/scene checksum | 9人10,000局/scene checksum |
| --- | --- | --- |
| 通常対局 | `13564747736715185154` | `17158162465123773162` |
| 外部guardian payload sampler | `6134552781244886811` | `14743928072819055564` |
| 任意設定のauto payload | `7593984257819276415` | `16205755181632469720` |

この比較は読み取り機能を足した際の既存実装への非干渉を確認するもの。公式のselector・死亡順・補充scheduleとの同値性は証明しない。独立familyは最大2系統を並行実行し、計測時間を版間の速度比較へ使わない。

併せて現行overviewの配布236/499＋任意237/500、Fate/現象/悪魔/終末、自己BUY・満杯SELL、load_scene、Snapshot47、sampler/rolloutの範囲をsourceへ同期した。版名付きの旧規約・旧hashは当時の履歴として残す。未投影のincremental history/rating/animation、公式server schedule、Dreamの互換外、94+Wide、一般Earth/Moon選出や全補充悪魔chain等は引き続き残件である。

### Pythonで公開攻撃を読み、防御の追加可否を照会する例

```python
from official_state import OfficialState, ResponseContext

raw = {"players": [{"id": 0}, {"id": 7, "items": [{"id": 0, "modelId": 190}]}],
       "attacks": [{"playerId": 0, "itemModelIds": [58, 66], "atk": 34,
                    "mp": 99, "cp": 88, "targetPlayerId": 7},
                   {"itemModelIds": [999], "atk": "opaque"}]}
with OfficialState(raw) as state:
    attack = state.active_attack()
    print(attack["summary"]["power"], attack["summary"]["element"])
    # 36 fire
    context = ResponseContext(**attack["response"])
    print(state.can_append_hand([], 0, player_id=7, response=context))
    # True
    print(state.view(player_id=7)["schema"])
    # official-fresh-view-v1
```

後続attackはopaqueなraw証拠として保存し、先頭のviewへ混ぜない。未描画のraw MP99/CP88もviewへ出ない。この照会結果はplayer7が現在コマンドを送れるという認定ではない。


## v49：プレイヤーの使用意図とchanceを分けたローカルdriver

v49は対局controller用の追加APIである。native `PlayerChoice = variant<Command,FateRequest>`、`choice_legal` / `sample_choice` / `choice_step` / `rollout_step` と、Python `chance_rollout` を追加する。`FateRequest{actor,token}` は「所有する207を使う」という意図だけを表し、現象・recipient・guardian割当・295 permutationを含まない。**対局ruleset47・Snapshot47・observe view-v45・全既存phase/struct幅/Command opcode0〜9は維持する。** 新driver API versionは1で、独立したrule getterを持ち、旧observeへキーを増やさない。

### 選択と抽選の境界

通常mainのownerで混乱count0の時だけ、unused/unmaskedな本人の207コピーをinventory順に列挙する。N枚あれば、普通Commandを選ぶbranchを1個、各207のFateRequestをN個として `bounded(N+1)` で1個を選ぶ。ticket0は従来の `sample_action` を呼び、それ以外は対応するactor/tokenだけを返す。これは **Fate側N/(N+1)、普通branch1/(N+1)の明示したローカル方策**であり、全合法手の一様分布でも公式の選択分布でもない。207のdiscard・売買等の普通の役割は従来どおり残る。

N0、response、purchaseでは追加drawなしで旧samplerへ直接委譲し、Commandとnext policy RNGが完全に一致する。`include_fate` は配布poolの設定であり、既に所有する207をRequestで使う能力を制限しない。positive-confusion mainとgrantではプレイヤー選択入口を拒否する。Requestのpreflightは所有・unused/unmasked・主手番・count0だけを調べ、将来の289 overflowや295 token枯渇を先読みして候補を除かない。

`choice_step(choice,chance_rng)` は明示選択を実行し、Commandなら従来step、Requestなら既存 `sample_fate` → `fate_step` と同じ結果を得る。sampleとstepは別transactionなので、callerはtrial policy seedを保持し、step成功後だけcommitする。Requestには未来の抽選結果を入れない。

| driver開始時の局面 | 選出・実行 | 使う外部RNG |
| --- | --- | --- |
| grant phase4 | sample_grant → grant_step | chanceのみ |
| positive-confusion main | 既存sample_confusion → confusion_step（207生成なし） | chanceのみ |
| count0 mainの普通Command | sample_choice → 普通Commandを実行 | policyのみ |
| count0 mainのFateRequest | sample_choice → sample_fate → fate実行 | policy＋chance |
| response / purchase | 追加envelopeなしの旧policy → 普通Commandを実行 | policyのみ |
| finished | 一手driverは拒否。multi-stepはその場で終了 | 進めない |

Match自身のgame RNGは従来の命中・body・配布等に残す。新driverは既存内部executorをtransaction copy上で呼び、追加decision・使用料・gift・手番を作らない。`rollout_step(policy_rng,chance_rng)` は必ず1 accepted decisionだけを進め、Match・両seed・resultが全て完成してからcommitする。late update/GF/gift/token枯渇やFate選出失敗で、先行したpolicy envelopeも含め全て戻す。失敗時に再抽選や別行動へのfallbackはしない。2つのRNGが同一storageを指す場合は拒否し、同じ数値を持つ別storageは許す。

C ABI2へ208byteの `gfo_match_choice` とsize/version/rule getter、choice/sample/step/rollout入口を追加した。184byte Commandは変えず、command0またはFateRequest1のtagで選ぶ。Request時はcommand全184byteを0、Command時はfate_actor/fate_tokenを0にし、予約bytesも0を要求する。actor0/token0は有効なidentityで、空値とは扱わない。C wrapperはstatus encodeまで完了してからMatch・両RNG・出力をcommitする。

Pythonはfrozen `FateRequest` / `RolloutResult(status,policy_rng,chance_rng,steps)` を提供する。`chance_rollout` は一手transactionの列であり、全体の一括transactionではない。途中で失敗すると `RolloutError.cause` / `__cause__` に元の例外、`.progress` に最後に成功したstatus・両seed・実stepsを残す。失敗した一手のseedへ進めず、成功済みの対局を巻き戻さない。max_steps0はno-opである。

C++で再開するには既存Match snapshotと2本のseedに加えdriver API/version/ruleを保持する。外部でsample済みの意図を保留しているなら、そのchoiceも保存するか選択前seedへ戻す。Pythonの分割rolloutとerror progressは**同じlive Matchの継続**であり、statusだけから盤面を復元できるという意味ではない。C/Pythonに対局checkpointのserialize/restoreを追加したわけではなく、既存 `load_scene` は引き続きquiescent初期化だけである。

旧 `sample_action` / `sample_confusion` / `sample_fate` / `sample_grant` / `random_rollout` の分布と意味は変更しない。新driverは明示して使う。混乱中の207使用生成、一般Earth/Moon・全補充悪魔chain・Dream互換外・94+Wide・公式server scheduleは未対応または未確認のままで、全合法手・公式完全対応とは呼ばない。学習器・学習済みモデルも追加しない。

独立getterの契約文字列はheaderと一致する。

```text
experimental:player-choice-v1-command-or-fate-request-actor-token-only;no-outcome-recipient-guardian-assignment-or-shuffle-in-player-choice;normal-main-owner-count0-visible-unused-unmasked207;gift-flag-does-not-gate-already-owned207;inventory-order-per-copy-fate-envelopes-plus-one-normal-command-envelope;uniform-bounded-Nplus1-zero-delegates-old-command-policy-rest-select-one-token;no207-response-purchase-delegate-old-policy-with-no-extra-draw;ordinary-discard-and-trade207-roles-preserved;not-uniform-over-all-legal-actions-or-official-policy;grant-and-positive-confusion-main-refuse-with-policy-unchanged;pure-world-game-rng-and-chance-unchanged;request-preflight-does-not-probe-future-outcome-validity;sample-and-choice-step-separate-transactions
```

```text
experimental:local-driver-api1-one-accepted-decision;grant-uses-chance-sample-grant-step;positive-confusion-main-uses-chance-existing-confusion-policy-no207;other-main-response-purchase-use-policy-player-choice;fate-request-resolved-only-by-chance-existing-fate-helper;ordinary-command-does-not-use-external-chance;game-rng-remains-owned-by-match;copy-world-and-trial-policy-and-chance-commit-only-after-full-step-and-result;failure-preserves-world-both-streams-and-outputs-no-reroll;distinct-policy-chance-storage-required-equal-values-allowed;no-new-phase-decision-fee-gift-or-observation-fields;save-match-snapshot-policy-chance-and-driver-api-policy-rule-for-resume;multi-step-rollout-is-sequence-of-transactions-not-one-transaction;existing-sample-action-confusion-fate-grant-and-random-rollout-unchanged;not-all-legal-actions-all-gift-devils-or-official-server-equivalence
```

### v49の検証

[LOCAL reference生成器](scripts/official_driver_oracle.cjs) / [保存report](data/official-v2/driver-local-v49.json) は独立BigIntでenvelope23例、別区分のFate chance stage6例、dispatch契約4行を記録する。reportは55,335bytes、SHA256 `6f8ba9de99f5c6ee3f6440b27faa1c64f4fb80a6729a038a42126e42c381b438`。新しい一次クライアント選択器の発見・実行は0であり、既存v44等の固定証拠を継承する。N0やresponse/purchaseのreferenceは旧samplerへ委譲する直前で止まり、旧sampler自体のdrawまで0と呼ばない。

N1/2/18、actor/token0・sparse/unsorted順、zero-word rejection、normal branchとFate branchを含む。Fate sectionはplayer-facingではないprivileged controller証拠で、287/289/291/295/296等の未来値やmanifestを方策へ渡してはならない。maxsafe tokenを持つfull18の295 referenceは抽選stageだけの値であり、nativeでは後のfresh-token確保が失敗し得ることを別に検査する。

Release CTest **150/150（47.25秒）**、ASan/UBSan CTest **110/110（131.47秒）** が通過した。既存Match native216群とState Python12件を維持し、専用native8群・Python Match148メソッド・C208byte境界を検査した。full18のlate295 token枯渇assert追加後もRelease driver1件が通過した（0.25秒）。新nativeはno207の旧policy完全一致、他者のhidden hand/resource等への非干渉、独立N1/2/18 envelope、別区分のFate stage、全10結果とpending継続、各phaseの手動routing同値、alias拒否、late update/289/295失敗時の全rollback、snapshot＋両stream再開を検査する。

Cは208byteのoffset・inactive branch全zero・reserved・token/actor0・NULL/canary、両RNGとstatusの失敗時保持を検査する。Python新4メソッド群はpure選択・明示choice step・全phase routing・RolloutError progress・split resume・max_steps0・strict input/closedを確認した。テストfixtureのgrant開始は合法な祈りから構成する。README例を実行し、`FateRequest(actor=0, token=0) 0` → 8steps時 `8 8 grant` → 再開8steps時 `8 16 grant` が一致した。suite時間は版間の速度改善や公式server適合数ではない。

旧17family＋新driverの各2人数、計36組で2人100,000局/scene・9人10,000局/sceneの1worker/4workersを照合し、時間・速度・worker数以外の全fieldが一致した。Release/SAN小行列19組も全checksumを含む非時間fieldが一致した。v48保存行列の全104recordとも、全gameplay field・metadata・checksumが同値だった。新APIを使わない旧sampler/実行器への回帰を別に確認し、新driverの選択法を旧samplerへ混ぜていない。

`godfield_driver_bench`（CTest `official_driver_benchmark 600 9 42 2`）は8周期fixtureで普通主動作・1/複数207・混乱・空手札からの補充・準備済purchase/response・終末閾値を扱う。include_fate/external_grants/local_guardian_payloads・Dream/Fog・used miracles・MP0/99・dead・sparse IDを混ぜる。2clones＋手動routingと全snapshot/両RNGを比較し、各decisionでrestoreして次へ再開する。setup済みの購入/攻撃のdecisionsはprepared_decisionsとして分離し、driver決定を二重計上しない。world/選択counterの部分hashと、private proof全体のrestore一致は別の検査である。

| 新driver・seed42 | decisions | FateRequest | 混乱command | grant | policy checksum | chance checksum | world checksum |
| --- | ---: | ---: | ---: | ---: | --- | --- | --- |
| 2人100,000scene | 217087 | 14476 | 12500 | 81032 | `11743515757574249284` | `3387543646297078825` | `6015453629773162075` |
| 9人10,000scene | 31689 | 1457 | 1250 | 10331 | `8471754564315384525` | `13253041463957902413` | `8772286365323507381` |

9人600sceneでは1951decisions=main450＋response744＋purchase93＋grant664、FateRequest88・混乱command75を含む（後者2つはmainの内数）。旧entropy同値1062、copy-order law150、resume/手動routing各1951を検査した。policy/chance/choice/world checksumは `7885916754374112615` / `18063975136686103100` / `12136982605830070376` / `3799550219682677017` でRelease/SAN一致。大規模の両人数とも全10Fate結果を実際に通った。この集計を公式の選択分布や勝率・学習性能の測定とは呼ばない。独立familyは最大2系統を並行実行し、時間を速度比較へ使わない。

### Pythonで207・混乱・補充をまたいで対局を進める例

```python
from official_state import OfficialState
from official_match import Match, Config

raw = {"gf": 0, "tiebreakGF": 0, "players": [
    {"id": 0, "hp": 99, "mp": 99, "cp": 99, "items": [{"id": 0, "modelId": 207}]},
    {"id": 7, "hp": 99, "mp": 99, "cp": 99},
]}
with OfficialState(raw) as scene, Match(config=Config(
        initial_hand=0, max_decisions=32, include_fate=True, external_grants=True)) as game:
    game.load_scene(scene, owner=0, seed=42)
    choice, trial_policy = game.sample_choice(0)
    print(choice, game.status.decisions)
    # FateRequest(actor=0, token=0) 0
    result = game.chance_rollout(0, 6, max_steps=8)
    print(result.steps, result.status.decisions, result.status.phase)
    # 8 8 grant
    result = game.chance_rollout(result.policy_rng, result.chance_rng, max_steps=8)
    print(result.steps, result.status.decisions, result.status.phase)
    # 8 16 grant
```

ここでは最初のsampleは純粋な照会で、rolloutは選択前seed0から始めている。明示的にchoice_stepを使う場合は成功後だけtrial_policyをcommitする。RolloutErrorのprogressはlive Matchと共に保持し、方策にはchance seedやprivileged manifestを渡さない。


## v50：公開攻撃表示の増分reducer

`ActiveDisplay` は、**受信済みの nextAttack / setTargetPlayer / advanceGF の3種類だけ**を適用する独立した表示componentである。初回のfresh投影を毎eventで呼び直すと、まだ表示していないtarget/商品を先行公開したり、既存paneを誤って消すため、source/反対側pane・選択arrow・カード/summary・表示GFを別の状態として保持する。

native `official_display.hpp` / `ActiveDisplay`、Cの独立opaque `gfo_display_*`、Python `official_display.ActiveDisplay` を追加した。新API version1・JSON `official-active-display-view-v1`・event wire88byteである。**既存State ABI1/freshview-v1/active-attack-view-v1、Match ABI2/ruleset47/Snapshot47/observe view-v45、driver API1は変更しない。** Model0を物理品へ変換したり、UI状態をMatch checkpointとみなしたりしない。

| 起点・受信event | pane/core表示へ適用する規則 |
| --- | --- |
| 初期fresh | v48のhead攻撃を復元。activeが無ければwire attackTurnPlayerId相当のturn_playerをleft source paneへ置く。endingならattack/panesを空にしsource/target/turnのlookupをしない（known fieldの型域検査は別） |
| nextAttack | 実fu/aaと同様に両pane・選択・前の商品をclearし、reversedで定まるsource側だけを置く。受信attack内のraw target/buyingはfresh callerのようには復元しない |
| setTargetPlayer | eventのplayerをlookup。0も実player0なら表示できる。sourceと同じ人ならsource側arrowだけを選び、既存opposite pane/商品を残す。それ以外はopposite paneを更新して選択する |
| advanceGF | 表示GFを1増やし、attack/選択/旧paneをclear。reversedをfalseへ戻し、event playerをleft source paneへ置く。サーバーの手番認定やMatchのGF更新とは別 |

公開JSONは `gf` / `reversed` / `panes.left,right` / `selected_side` と、v48のsanitized attackオブジェクトを含む。`attack.target` は現在選択したpaneのID（明示0も可）、`attack.buying_model` は既に描いた商品である。選択を自分へ戻してもopposite側の以前の人・商品が残り得るので、単一のtarget値だけから全paneを再構成しない。names/status等の全pane内容・画像/DOM/layoutはこのcomponentの外である。

初回freshではtarget0が未指定sentinelだったのに対し、独立したsetTargetPlayer(0)はroster0をlookupして明示選択する。nextAttackのunknownだがJS-safeなrawtargetはlookupしない。未描画のrawbuy/MP/CP等は出さず、特殊カードで実描画される値とsummaryだけを既存sanitizerで返す。known model型の入力域は未描画fieldにも検査するため、不正buying_model999まで受け入れる契約ではない。

保持するcontextは最大9人の**roster IDだけ**で、初期化に渡した手札・資源・災い・認証UID・名前等を保持しない。source/targetを勝手に選ばず、乱数・新token・消費・補充・HP/MP/CP・災い・死亡・守護神・報酬・Match phaseを生成しない。GF更新は表示counterだけで、transport updateCountやイベントのidempotenceも推定しない。新Display handleを `Match.load_scene` へ渡す経路は提供しない。

### componentの範囲と例外

実クライアントのcomplete handlerと、このpane/core投影は分ける。例えばending直後の空paneへsetTargetPlayer(0)を渡すと、aTはright pane/arrowを更新した後、後段overlay処理がsource不在で例外になる。新APIは完了したcore pane更新だけを表し、そのoverlay例外まで再現するAPIではない。ending後のnextAttack/advanceGF branch自体にはfreshのending gateが無いことも実行して確認した。

一方、rosterに無いsource/targetは構造的lookup失敗として拒否する。実advanceGFではpane clear/GF+1後のlookupで例外となり部分更新が残る場合があるが、**typed native/C/Python APIは全batchをcopy上で処理し、成功時だけcommitする別契約**である。raw clientの失敗後の部分画面と、nativeのrollback状態が一致するとは主張しない。安全域外GF+1やchecked int64合成overflowも拒否する。

Cは `gfo_display_create` でlive同library Stateから初期化し、`gfo_display_apply` は新しいowner handleを返す。eventはattack72byte＋player_id8byte＋tag/reserved8byte。next branchでは外側player_id0、target/GF branchではinactive attack72byte全て0を要求し、借用model列をdeepcopyする。count/pointer/padding/known enumを検査し、NULL/short/errorは全出力と元handleを保持する。空batchは独立cloneである。

Pythonはfrozen `NextAttack`（modelsをtupleとして所有）/`SetTargetPlayer`/`AdvanceGF` を受け取る。`apply` は入力Displayを変えず、新しいDisplayを返す。元OfficialStateをcloseした後も独立して使用できる。未対応eventを黙ってskipする入口や、raw events全体を自動で再生する入口は追加していない。C++のcopy／Pythonの空applyでこのcomponentを複製して続けられるが、対局の保存・復元ではない。

### v50の一次照合と検証

[実client oracle](scripts/official_active_display_oracle.cjs)、[保存report](data/official-v2/active-display-client-v50.json)、[native TSV](data/official-v2/active-display-vectors-v50.tsv) は実ks.kの3受信branchとaa/fu/fq/aT・roster lookupを実行し、47受信シーケンス・53event callを記録する。full handler error8／core error7を分け、native ERROR10にはtyped adapterのstrict未描画field拒否・safeGF上限も含める。native用47vectorはJSON/core結果または指定されたERRORと全て一致した。

reportは214,677bytes、SHA256 `887973ed33cadc6a9f230d12d5d9705f7e12b6e50984ff101facc0aafce3ba43`、TSVは19,266bytes、SHA256 `9ad37e05a6fb56260556f4359354a6ae39951bc9a414f4069b2e6c5cbcd98344`。v2固定clientとv48のfactory prologをhashで固定して再利用し、既存source/report/pinsは変更しない。Future同期・描画sink・board shells・timed overlayはstubを含み、全アプリ・全incremental history・backend・公式event fanoutの再現とは呼ばない。

Release CTest **154/154（94.32秒）**、ASan/UBSan CTest **113/113（213.45秒）** が通過した。関連focused12件も全通過（14.15秒）。native新8群、実client由来47vector、C88byte境界、Python新5メソッド群を検査し、旧active443vectors・State/Match/driverの全テストも維持する。nativeではfresh/増分target0、sameSourceのopposite/商品保持、Nのclearと未描画値非干渉、reversed/selected_side、Gのreset、ending、順序合成/copy再開、途中失敗の全batch rollback、safeGF/checked overflow、roster private情報非保持、19列/head-onlyを確認した。

Cはinactive72byte sweep・予約padding・extentをpointer参照前に検査すること、NULL/short/canary/全出力保持、deepcopy/空clone/元State破棄後の所有を検査する。Pythonはtyped eventのtuple所有、immutable applyとcontext/close、strict入力、unknown event拒否、元State/Display不変、Matchへ渡せないことを確認した。C11としての88byte/offsetと旧Command184/Choice208/frame72のlayoutも検査した。READMEのN→T0→G例を実行し、right source7→left target0→GF18/left source7/attackNone、元Displayのleft0保持が一致した。suite時間はCPU並行実行を含み、版間の速度比較や公式server適合数ではない。

既存18familyの2人100,000局またはscene・9人10,000局またはsceneを1worker/4workersで照合し、36組で時間・速度・worker数以外の全fieldが一致した。Release/SAN小行列19組も全checksumを含む非時間fieldが一致した。v49保存行列の全110recordに対し、gameplay field・metadata・checksumが全て同値だった。

| 既存処理・seed42 | 2人100,000局/scene checksum | 9人10,000局/scene checksum |
| --- | --- | --- |
| 通常対局 | `13564747736715185154` | `17158162465123773162` |
| 任意設定のauto payload | `7593984257819276415` | `16205755181632469720` |
| typed choice/chance driver | `6015453629773162075` | `8772286365323507381` |

この比較は独立display API追加による既存対局への非干渉を確認するもので、公式serverの抽選・進行との同値性を証明しない。独立familyは最大2系統を並行実行し、速度改善の主張には使わない。

gift/replaceItems/useDevilItemのhand placeholder・used-pane更新、全resource/病気/取引/ending/rating event、公式server schedule・selector、一般Earth/Moon・Dream互換外・94+Wide等は引き続き別の残件である。学習器・学習済みモデルも追加していない。

### Pythonで受信した表示更新を適用する例

```python
from official_state import OfficialState
from official_display import ActiveDisplay, NextAttack, SetTargetPlayer, AdvanceGF

raw = {"gf": 17, "attackTurnPlayerId": 0, "players": [{"id": 0}, {"id": 7}]}
with OfficialState(raw) as state, ActiveDisplay(state) as initial:
    with initial.apply([NextAttack(source=7, models=(58, 66), atk=34,
                                  reversed=True, target=0, buying_model=235)]) as incoming:
        print(incoming.view()["panes"], incoming.view()["attack"]["target"],
              incoming.view()["attack"]["buying_model"])
        # {'left': None, 'right': 7} None None
        with incoming.apply([SetTargetPlayer(0)]) as targeted:
            print(targeted.view()["panes"], targeted.view()["selected_side"],
                  targeted.view()["attack"]["target"])
            # {'left': 0, 'right': 7} left 0
            with targeted.apply([AdvanceGF(7)]) as cleared:
                print(cleared.view()["gf"], cleared.view()["panes"], cleared.view()["attack"])
                # 18 {'left': 7, 'right': None} None
    print(initial.view()["panes"])
    # {'left': 0, 'right': None}
```

受信nextAttack内のtarget/buyingは表示復元へ使わず、対象は後続eventで明示する。元のState/Displayは変えず、どの呼出しも対局コマンドを送信・実行しない。


## v54：通常攻撃奇跡の自己使用と再使用

公式の2人CPU修行で、213＜大木＞の自己使用（HP40→34、MP10→7）、19鎖ガマへの空防御（HP34→29）、使用済み213の再使用（HP23→17、MP4→1）を確認した。いずれも前後の公開資源と次G.F.を記録し、自己使用2件には追加の本人防御操作がなかった。大木は同じ使用済みコピーとして残り、手札は初回9→10、後の再使用11→12となった。途中の別の再使用はCPUの次行動を取り逃したため、完了遷移には数えない。[観測記録](data/official-v2/live-transition-witnesses-v54.json)を参照。

対局を `official-execution-subset-v50` / Snapshot50 / view-v48へ更新した。通常mainの明示自己targetで、能力なし・hitRate0の攻撃奇跡もDEF0・1decisionで解決する。MPの支払い・奇跡の保持・各使用時の補充は既存処理を使う。大木の実例から同条件の8種へ限定して一般化し、その他の特殊自己攻撃・確率攻撃・redirect等の経路は維持する。Match ABI2の幅・opcodeは変えない。公式の全遷移・特殊効果の順序・乱数分布の同等性は未確定である。

検証はRelease全155/155件、ASan/UBSan全114/114件が通過。native218群・Python150ケースを含み、新しい3観測を加えた7件の公開投影504比較と別枠72経路検査を行った。main/driverのRelease/ASan・1/4worker再現性比較16recordも、時間・worker以外の全fieldが一致した。


## v55：特殊自己攻撃の選択・表示経路と実験実行規約

公開clientの実関数で、自己使用の選択・送信と、単一受信frameの自己target表示を分けて照合した。[生成器](scripts/official_self_attack_oracle.cjs) / [保存report](data/official-v2/self-attack-client-v55.json)を参照。これらはserverからそのframeが生成されることや、効果計算・死亡・補充の内部順を証明するものではない。新しい完了実対局観測は0件で、累計7完了例と別枠routing-only1例を維持する。

対局ruleset51 / Snapshot51 / view-v49は、通常mainの明示自己target・hitRate0・Wideなしに限り、吸収27/45/216、二回攻撃14、MP武器58、同被害53を追加のdefendなしで進める。既存の実験的なbody算術を使い、費用と消費は1回。14は起動時の同じ威力・属性で2回処理し、自然終局か本人の実死亡で残りを打ち切る。58は支払い前に固定した威力を使い、53は1body内の二段減算を保つ。反射・跳ね返し後のframe、敵対象、他の特殊selfは従来の経路を使う。Match ABI2の幅・opcodeは維持する。これら特殊selfの効果順は引き続き公式での実対局確認が必要である。

検証は全Release155/155件と、ASan/UBSanの公式関連37/37件が通過した。native221群・Python151ケース、client oracleの76＋65ケース、main/driverの16record再現性比較も通過。公式serverの効果順や乱数同等性を示す数値ではない。


## v56：精霊の杖の自己使用と自己売却の実対局照合

[公式CPU修行の追加記録](data/official-v2/live-transition-witnesses-v56.json)に、売る4＋なぎなたクラシック26の自己売却、つるぎ焼き69（石2）への空防御、精霊の杖46の自己使用という3完了遷移を加えた。累計10完了例・別枠routing-only1例・full wire trace0件である。自己売却では両者のHP/MP/CPを保ち、手札9枚と商品26の再表示を確認した。46自己使用は追加defend無しでHP38→26、MP10/CP20維持、G.F.4のCPU次攻撃前まで捕捉した。

対局ruleset52 / Snapshot52 / view-v50は、武器baseの `cut_cost` も明示自己target・hitRate0・非WideのDEF0即時処理へ含める。固定台帳でこの条件に入る新モデルは46だけであり、防具143/163/173や雑貨204へ主攻撃範囲を広げない。MP費用・奇跡の保持・消費・補充は既存処理を使う。46単品の観測から合成列への適用には限定一般化を含み、46baseだけで後続奇跡が全て無料になる規則は加えていない。Match ABI2は維持する。

新しい補充モデルをローカルseedへ合わせず、nativeでは10例×72＝720公開投影と、別の72経路検査を行う。自己売却後の新gift46は後のtooltipで同定できたが、抽選一致の比較には使わない。元の選択コピーの消費・商品の配送・手札総数を、新giftのモデル寄与から分けて検査する。その他の特殊self6種の効果順、杵＋全体化、終末中の一新の順は、引き続き実対局の確認が必要である。

検証はRelease全155/155件、ASan/UBSan重点4/4件が通過した。native222群・Python151ケースと720公開投影比較＋別枠72経路検査を含み、main/driverのRelease/ASan・1/4worker再現性比較16recordも一致した。


## v57：終末中の取引・夢・悪魔後の再授与を実対局で確認

[公式CPU修行の追加記録](data/official-v2/live-transition-witnesses-v57.json)に、他者への売却、通常攻撃、夢の付与、女神の石けんによる使用済み奇跡除去、杖＋オーラと悪魔、購入と悪魔、祈りと悪魔という7完了遷移を追加した。累計17完了例・別枠routing-only1例・full wire trace0件である。石けんで使用済み222を除去しても本人の夢は残った。祈りでは中悪魔の20被害後に神器1枚を受け取り、HP50→30・手札8→9・G.F.8→9を連続して確認した。

今回の実測は既存処理と整合するため、本体ruleset52 / Snapshot52 / view-v50とMatch ABI2を維持する。先の4例と祈りの計5例をnative比較へ追加し、累計15例×72＝1080公開投影と別枠72経路検査を行う。祈りの悪魔結果は外部chance入力として与え、抽選率や新神器のモデルは比較しない。残る複合2例は観測記録として保存し、未取得のCPU手札や補充を仮定した完全再生へ置き換えない。

悪魔を挟んでも神器の授与が続く単独の祈り例は得られたが、画面のアニメーション順からサーバー内部の受取人順を確定しない。一新178の終末grant、杵94＋全体化、夢の互換外候補や特殊自己攻撃の順序は引き続き残件である。

変更した公開遷移テストはRelease・ASan/UBSanで各1/1件が通過し、両ビルドのwarning/errorは0件。今回は本体コードを変更しておらず、前版の全suite・ベンチマークを再実行したとは扱わない。


## v58：夢見る帽子による一新を実対局で確認

[追加記録](data/official-v2/live-transition-witnesses-v58.json)に、本人の守護封印のつぼ201→CPUの守護神「まばたき」271→夢見る帽子178での防御→一新と後続授与→G.F.12までの1完了遷移を保存した。累計18完了例である。DEF14で闇2を防ぎ、Dream付与、使用済み226・214を含む旧手札の置換、その後の妖精によるCP＋10・MP＋10を確認した。最終状態は本人HP38/MP10/CP30・Dream・未使用手札10、CPU5/4/20だった。

一新の置換品と後続補充品のどちらに悪魔が含まれたかは、並べ替えを伴う画面から特定できない。今回は本体ruleset52 / Snapshot52 / view-v50を維持し、一新へ悪魔grantを必須接続する変更は加えない。従来の15例1080公開投影と72経路検査に加え、別枠72コンポーネント検証で帽子の防御・Dream・旧使用済みコピーの消失を照合する。原発201の予約補充を含まない合成境界なので、今回の最終枚数・次手番の完全再生には数えない。

更新した公開遷移テストはRelease・ASan/UBSanで各1/1件が通過し、warning/errorは0件。再取得した公式JSで一新・授与の既存oracleもbyte一致を確認した。今回の検証を、全suiteの再実行や公式完全対応の証明とは扱わない。

## v59：公式の受信状態を使った再現検証

CPU修行で受信した連続4状態を[匿名化fixture](data/official-v2/wire-game-updates-v59.json)へ保存し、自己攻撃・空防御によるCold付与・自己解除の3完了操作を追加しました。累計21完了観測です。新しい[再現テスト](tests/official_wire_trace_test.cpp)は、8 seedで4 checkpointずつ、資源・実手札・病気・進行中の攻撃を照合します。CPUの真手札を含む検証専用データで、方策向け観測には渡しません。

対局v53 / Snapshot53 / view-v51では、外部補充へ通常モデルを明示指定できます。設定中の配布プール内・Dreamなしに限定し、ゲーム乱数を消費せずfresh tokenで受け取ります。既存の通常抽選0と悪魔入力、ローカルsampler、Match ABI2を維持します。受信イベントはCPU補充の一部を省略するため、全内部イベントや公式完全対応の証明にはなりません。[確認範囲と残件](docs/official-spec-gap.md)を参照してください。

## v60：空スロットの分離と、補充中の死亡・終局

公式CPU修行の開始から終局まで、連続する受信game更新2状態を[追加取得](data/official-v2/wire-game-updates-v60.json)しました。複合ATK22へ二枚DEF8で応答した後、悪魔241→242の連続被害で死亡し、G.F.1のまま終局する1完了例です。CPUの243による除去と、受信eventsに現れないCPU通常補充は別々に扱います。全内部イベントの取得を意味しません。

Python `ReceivedState` は、公式データの `{id:0,modelId:0}` を空スロットとして分離し、実物だけを既存のC++状態投影へ渡す明示的な読み込み口です。元データと空スロット情報は検証専用に保持します。死亡後にも空スロットが残るため、補充の予約数へ読み替えません。既存 `OfficialState` の厳密検査とMatchの再開制約、ruleset53 / Snapshot53 / view-v51・ABIは維持します。

## v61：Dreamの見かけを明示する補充API

C++ / C / Pythonへ `ArtifactGrantAction(target, model, fake_model)` と `artifact_grant_step` を追加しました。予約補充phase4で、実モデルと、既存の互換範囲に入る見かけを外部chance結果として指定できます。見かけ無しはDream中でも明示的な非偽装です。ゲーム乱数・ローカルDream抽選統計を進めず、常にfresh tokenの未使用品を1枚与えます。

ruleset54 / Snapshot54 / view-v52へ更新し、Match ABI2・既存GrantAction32 byteと通常0/samplerは維持しました。新C型は別の16 byteです。非互換Dream、公式の50%分布・選出、一新中の授与などは未対応・未確定のままです。今回の追加は条件付き再現の入口で、新しい公式完了観測は加算していません。

## v62：通常対局の補充を外部結果で再現

`external_artifacts=True` を追加し、終末以外でも予約補充をphase4で停止して、記録した通常神器・見かけを入力できるようにしました。悪魔の許可は従来どおり `external_grants=True` かつ終末中だけです。normal-onlyの `sample_grant` は0を返して外部chance RNGを進めず、通常0を解決する際のゲーム内抽選は既存のままです。

ruleset55 / Snapshot55 / view-v53、Config24 byte・Match ABI2を維持します。既定falseで従来の挙動を保ち、reset・帽子の即時一新・取引商品の配送は外部入力へ切り替えません。公式では天国草198の自己使用とHeavenによるHP＋5・補充を新たに受信記録へ保存しました。詳細は[観測記録](docs/official-live-observations.md)と[差分表](docs/official-spec-gap.md)を参照してください。


## v63：夢の木づちと同一番号のDream偽装

公式CPU修行で、80夢の木づちの自己使用が防御入力なしにHP40→36・Dream付与まで進み、補充が `modelId=135, fakeModelId=135` になる例を取得しました。自己使用経路をDream付与武器へ広げ、受信した同一番号の偽装を明示unmaskedと区別して保持します。ローカルDream抽選の候補・率は変更しません。

ruleset56 / Snapshot56 / view-v54、Match ABI2と既存構造体幅を維持します。実測は80、同じ能力の36への適用は限定一般化です。[差分表](docs/official-spec-gap.md)と[観測記録](docs/official-live-observations.md)に、受信後状態1件と復元初期sceneの境界を記録します。公式完全対応は継続中です。


## v64：Dream偽装の受領から実使用まで

公式CPU修行で135/135防具と195/195の花を実際に使用し、さらに見かけ195（MP＋5）・実体197（MP＋15）の使用でMP20→35を確認しました。受信update4〜7を保存し、再構成sceneから既存実装と照合します。新しい異番号pairは既存の9key互換範囲内です。

今回は公式記録と回帰テストの拡充で、runtimeはruleset56 / Snapshot56 / view-v54を維持します。[観測記録](docs/official-live-observations.md)と[差分表](docs/official-spec-gap.md)へ反映し、ブラウザでホーム復帰とNetwork Monitorの終了まで確認しました。


## v65：運命のひも・濃霧と守護神の発動順

公式CPU修行で207の使用から濃霧288・双方Fog・補充・次CPUの闇3攻撃、DEF6による無被害までを取得し、既存Fate APIで再現します。別対局では201によるNeptune召喚と、壁233の防御後に276のHP＋5→通常補充という受信順を保存しました。召喚・自動発動のselectorは未再現で、276の効果単体だけを別に検査します。

新4観測で累計32例、全体のnative投影対象は27例です。runtime56 / Snapshot56 / view-v54とAPI幅は維持し、観測記録と回帰テストを拡充します。


## v66：自己吸収と自己二回攻撃の実測

リアルゴーストソード45の自己使用と、のこぶんぶん14の自己使用を公式CPU修行で取得しました。14は3被害→nextAttack→3被害・消費1回・補充1回でHP40→34となり、既存実装と整合します。

45は受信が12被害→12回復、現実装の内部計算は回復→被害です。HP40では両方とも40になるため、終点の一致と順序の検証を区別します。低HP・上限・死亡の条件をまだ取得しておらず、今回はruntimeを変更しません。[差分表](docs/official-spec-gap.md)へこの残件を明記しました。


## v67：受信HPイベントの途中状態を検証

C++の独立した `HpDisplay` に、受信した `dealDamage` と `boostHP` を順番に適用する機能を追加しました。v66の自己吸収の表示40→28→40と、自己二回攻撃後に対象を切り替えてCPUの回復を表示する経過を検査します。固定公式client由来のHP算術2,617行も両分岐から照合します。

これは受信HPの検証用部品です。対局本体の吸収・死亡順は未確定のまま維持し、公式完了観測34例・native状態投影対象29例も変更しません。C/Python APIは追加せず、既存ruleset56 / Snapshot56 / view-v54・ABIを維持します。使用条件は[状態と表示の契約](docs/official-state-view.md)を参照してください。


## v68：低HP自己吸収の実測に基づく修正

公式CPU修行で、HP3の本人がゴーストソード27を自分に使い、被害7→回復7を経てHP7で生存する例を取得しました。従来の回復→被害ではHP3になるため、自己吸収を被害→全量回復へ修正しました。実HP損失3で回復量を削らず、死亡・復活の判定は効果全体の後に行います。他者への吸収と同被害武器53の順序は維持します。

初期状態からの6受信更新と5操作を保存し、固定8seedで48checkpointを照合します。新規の直接実測は27で、45/216や反射後など同じ自己吸収能力への適用は限定一般化です。対局は `official-execution-subset-v57` / Snapshot57 / view-v55、C ABI2と既存構造体幅は維持します。[差分表](docs/official-spec-gap.md)と[観測記録](docs/official-live-observations.md)へ反映しました。

同じ対局で39月光のオノ＋162の自己攻撃が防御入力なしに25被害で完了することも確認し、39の直接自己使用経路を修正しました。反射能力のある防具全般へ広げる変更ではありません。


## v69：能力が異なるDream偽装を受信・使用

公式CPU修行で、実体19鎖ガマ／見かけ27ゴーストソードを受領しました。プレビューは攻7・吸収ですが、自己使用の実結果は5被害・回復なし（HP40→35）でした。従来の9field互換では拒否していたため、この方向の受信pairを追加しました。選択は見かけ、効果は実体で計算し、逆方向や他の未観測pairへは広げません。ローカル偽装の生成候補・率・RNGも維持します。

同じ対局で178の非終末一新8枚＋通常補充1枚、偽装防具の実DEF1、両替での所持金0化も記録しました。所持金0・MP30で価格5の商品を引いたBUYは公式も拒否し、HP/MPを減らしませんでした。一新の抽選とBUY商品選出は全再生に含めず、一新後の5状態を固定8seedで比較します。対局はruleset58 / Snapshot58 / view-v56、既存C ABI2と型幅は維持します。


## v70：一新の受信結果を外部指定して再生

178「夢見る帽子」の防御に、既知の置換結果を渡す `RedrawResponseAction` を追加しました。C++／C／Pythonから使えます。全選択の消費→各帽子のDreamと即時一新→既存の被害・復活・反撃を、通常の1応答として処理します。通常補充の予約枠は置換へ混ぜません。

通常の `step`・driver・samplerと一新の抽選は維持します。検証用の外部結果は新しいprivate履歴に保存し、pending・補充待ち・完了・cutoffでも同じ局面へ復元します。ruleset59 / Snapshot59 / view-v57、C ABI2と既存型幅は維持し、新しい置換item/result型だけ8/16 byteを追加します。

新しい公式観測は増やさず、v69の受信8枚を使って一新前から6状態を再生します。抽選分布や終末中の一新を公式同等とする変更ではありません。[実行規約](docs/official-execution-subset.md)に使用例と検証範囲を記載します。


## v71：購入商品の外部指定とCP0拒否の再生

既存の公式受信記録にある購入商品を、`PurchaseResponseAction` で正確な売り手コピーとして指定できます。商品抽選だけを差し替え、公開・CP判定・確認・返却・配送は既存の処理を使います。通常のCommand・driver・samplerの抽選経路は維持します。

v69の一新前から7状態を固定8seedで再生し、CP0で価格5の商品を購入できず同じコピーを返却すること、本人への237補充、返却品を使う次のCPU BUYまで照合します。新規公式観測は0、既存45例中の条件付きnative対象は40例、記録のみは5例です。公式の商品選出分布と、最後の未応答CPU BUYの結果は未検証です。

ruleset60 / Snapshot60 / view-v58、C ABI2と旧構造体幅を維持し、追加の購入選出型は16 byteです。[再生report](data/official-v2/supplied-purchase-replay-v71.json)へ範囲を記録します。

検証はRelease CTest157/157（39.39秒）、Debug ASan/UBSanのcore/C API/wire3対象3/3（14.55秒）が通過。native241群・Python175method、compile warning/error0、C11 -Werrorの新旧layout/prototypeと実行規約に掲載した購入APIのPython使用例も確認した。時間はsuiteの経過で、速度比較には使わない。全SAN suite/Python SANの実行とは区別する。
