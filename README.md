# Godfield RL core

**配信最新版との差分確認（2026-11-02）:** fresh取得の公式JA/JSは固定snapshotとhashが異なる。296モデルの数値・能力・curse・giftRateは比較済みで変更0、差は画像名19/26/117と117の表記「石版→石板」等である。これら表示名/画像名の追従は未反映とし、既存pinsを維持する。JAにはスペイン語の言語項目追加とcostMP/priceCP文言の再編もある。最新版の除去handler・描画・reveal・decode経路は識別子を正規化した制御構造/リテラル比較と手読で照合したが、新JS全体の同値性は保証しない。[取得物と差分証跡](data/official-v1/upstream-review-2026-11-02.json)を参照。過去のfresh一致記録を今日のhash一致として扱わない。 v20/v21/v22/v23の再取得もこのNov2 reviewの新hash/bytesと一致し、新しい変更は検出していない。元pinsとの差が解消したという意味ではない。

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
224モデル・重み485に限定した2〜9人/チーム対局を実行できます。
配布・死亡・手番のscheduleは実験規約であり、公式サーバー同等性は未検証です。

[公式一次調査](docs/official-current-rules.md)、[状態と表示の契約](docs/official-state-view.md)、
[限定実行器の契約](docs/official-execution-subset.md)、
[適合計画と未完了条件](docs/official-conformance-plan.md)、
[296件の出典・取得hash・旧版比較](data/official-v1/README.md)を参照してください。
限定範囲では主動作・応答・効果・死亡・補充・次手番・チーム終局を進め、物理反射・属性消去・遮断・跳ね返し・FREE・属性付替・倍化・単純確率攻撃・攻撃/資源反撃4種・HP吸収/同被害・閃光/暗雲の付与/反撃/解除・未使用手札/使用済み奇跡の除去・MP武器58・二回攻撃14・全敵化232・HP増減202・自動復活208・杵94/臼209・両替3・売却4・購入確認付きBUY5・病気4状態と付与6種・自己病気防具168/MP+Heaven雑貨198を扱います。
公式の237モデル全配布や全固有効果、多段queue、
吸収/閃光以外の固有効果付き確率攻撃、閃光/暗雲以外の災いcounter、自己購入・満杯配送等の未対応取引、霧・夢、守護神・終末は実行できません。
旧版37profileの対戦Env・C ABI・Pythonは保存し、公式版へ改称しません。学習器と学習済みモデルは含めません。

**旧Flash版を参考に、全7属性・回復/攻撃/状態・解除/追加ATK/倍化/全体化奇跡の保持・順序付き追加・物理品238の追加ATKとMP無料化・確率攻撃・反撃6種・HP吸収・HP増減賭け・購入確認と強制売却・杵/臼・自己被害付き武器・手札除去・忘却・資源交換・自動蘇生・物理品による魔法跳ね返し・魔法/万能反射・病気・一部harmsまで実装しています。現行公式の完全再現ではありません。**

| ルールセット / API | 用途 | 神器・接口 |
| --- | --- | --- |
| `official-catalog-client-rules-v1` / fresh state view | 現行公式の台帳・公開clientの表示計算、最大9人の状態の部分投影と見かけに基づく選択照会。対局実行不可 | 296モデル、`gfo_*` ABI1と `gfo_state_*` / Python `OfficialData`・`official_state.py` |
| `official-execution-subset-v23` | 公式データによる実験規約の2〜9人/チーム対局。公式serverとの同等性未検証 | 224モデル/重み485、全7属性・単純回復・奇跡保持・反射/消去/遮断/跳ね返し・FREE/属性付替/倍化・単純確率攻撃・攻撃/資源反撃4種・HP吸収/同被害・閃光/暗雲・除去2種・MP武器58/二回攻撃14・全敵化232・HP増減202・自動復活208・杵94/臼209・両替3・直近除去結果の公開・満杯配送を除く売却4・自己対象/満杯配送を除く購入5・病気4状態/付与6種・168/198、`gfo_match_*` ABI2 / Python `Match` |
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
公式対局traceは今回未取得で、公開clientから権威的serverの全実行器が得られたわけではありません。
対局シミュレーターとしての公式完全対応は未達です。

## 公式データによる限定対局実行器

[official_match.hpp](include/godfield/official_match.hpp) の `godfield::official::Match` は、
224モデル/重み485を対象に、全7属性、通常武器・普通防御、HP/MP/CPの単純回復、
奇跡のused保持・再使用、物理反射/消去/遮断/跳ね返し、FREE5種・属性付替2種・倍化231、単純確率攻撃21種・攻撃/資源反撃181/184/187/188・HP吸収/同被害5種・閃光付与/反撃/解除7種・暗雲・除去・MP武器58/二回攻撃14・全敵化232・HP増減202・自動復活208・杵94/臼209・両替3・売却4・購入5・病気4状態/付与6種・168/198、2〜9人とチームを扱います。
rulesetは `official-execution-subset-v23`、scopeは `experimental:224-models;not-official-server-equivalence` です。
対象外のモデル・fake・病気4種/閃光/暗雲以外の災い・守護神・混乱・終末設定を持つ局面は、黙って通常品へ置換せず拒否します。
read-onlyな `OfficialState` が表示できる局面を、全てこの実行器へ渡せるわけではありません。

主動作から応答へ進み、body効果・死亡の後に自然終局/cutoffを優先します。全action完了時に元ownerの病気tickと再死亡/終局判定を挟み、継続なら補充・次ownerへ進みます。
通常の自己攻撃もresponseを作り、自己回復は即時、他人への単純回復は空応答または適合する反射・跳ね返し・魔法遮断を待ちます。
通常品の消費には補充を予約し、奇跡はusedとして残して空きがある場合だけ予約します。捨てるには補充を付けません。
初期9枚/HP40MP10CP20/総枠18、固定入力順の主手番、残余 `max(0,ATK−DEF)`、
配布・死亡・終局scheduleは明示したローカル実験規約です。
公式clientで照合した算術・表示と、権威的serverで未照合の順序を[契約文書](docs/official-execution-subset.md)で分けています。

反射は元のpower/category/abilityとownerを保ち、現在のsource/targetだけを交換します。
属性消去は反射後も無属性を維持し、原発奇跡の再課金・新sourceへの取得は行いません。
各応答者が使った品の補充は連鎖後へ遅延し、Snapshot version23の全action journalで消費・保持・MP・補充と抽選/反撃来歴を検証します。旧version1/2/3/4/5/6/7/8/9/10/11/12/13/14/15/16/17/18/19/20/21/22は拒否します。
普通防具→反射は現行clientの追加条件で不受理で、反射前に許すのは連続した属性消去だけです。
自己反射にもresponseを作るのは明示的な実験規約です。公開viewに未来RNG・消費token来歴は出しません。

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
公開JSONは `official-execution-view-v21`、Snapshot23、MatchのC ABIは2です。read-only catalog/state ABI1は維持します。`pending.reflections` は反射、`pending.bounces` は跳ね返し件数、`pending.hit_rate` は命中率、`pending.counter` は反撃payloadかを示します。181の75は反撃開始後の表示であり、初期全敵chanceとは別です。

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
次main受理でclear、失敗時は保持、reset/loadではclear。response処理自体はclear契機ではありませんが、単一bodyのaction完了eventなのでresponse phaseとの同居は認めません。解決済みcutoffは保持し、未解決cutoffはnullです。token/slot/RNG・元候補数・非除去手札は公開しません。公開 `removal_event_rule='experimental:resolved-body-only;public-removed-models-in-draw-order;duplicates-and-empty-preserved;clear-on-next-accepted-main;no-tokens-or-rng'` に寿命を含む実験規約を明示し、全履歴を投影したとは主張しません。受信clientの他者対象表示は根拠がありますが、全参加者/観戦者へのserver配信や空結果eventの必ずの発行は未確認です。完了snapshotは構造整合だけを検査し、歴史的到達証明ではありません。

v18は両替3（trade/価格5/重み20）を加え214モデル/重み437（trade1/98武器/74防具/16雑貨/25奇跡）にします。`Command(actor, kind="exchange", items=(token,), allocation=(hp,mp,cp))` を主手番で使い、targetは無し、各0〜99・配分前と総和一致、同値も許可します。単品消費/gift1予約・MPfee0（価格5は費用ではない）→3資源を直接代入→既存208救済/死亡→自然終局/cutoff優先→gift/次手番です。HP0も合法で、復活+10後には総和が増え得ます。配分抽選はpolicy RNGだけ、環境RNGは後のgiftだけです。
公開 `exchange_rule='experimental:single-owned-exchange;targetless-self;three-resources-0-99;sum-conserved-before-revival;zero-mp-fee;no-response;one-deferred-gift;death-and-revival-before-gift'`、Snapshot18旧1〜17拒否/view-v16。typed配分追加により `gfo_match_*` はABI2、command168→184byte（末尾hp/mp/cp/has_allocation/reserved_tail[3]）で、旧168byteのまま呼べません。構造体API呼出前にversion/sizeを検査し、ABI1 Match clientは移行してください。config24/status248、catalog/state ABI1・旧37は維持。sacrificeは終末依存で未実装、94+Wideのgapも維持します。

v20は売却4を加え215モデル/重み457（trade2/98武器/74防具/16雑貨/25奇跡）、Snapshot20旧1〜19拒否/view-v18、Match ABI2の構造体サイズは据置です。`Command(actor, kind="sell", items=(sell_token, commodity_token), target=target)` で本人の4と別tokenの未使用商品を使い、自己/味方を含む生存targetを明示します。usedMCは売れませんが武器・208/209・別4・未使用MCは商品にでき、MP0/価格5は操作料ではありません。
4と商品を消費して操作分gift1だけ予約し、商品MCの保持化や商品分の補充はしません。他者は190のみのnonattack response、自己は即時。現在targetへのCP請求（不足MP→HP）→現在sourceへの名目全額CP加算cap99→新tokenで未使用商品を直接配送→208復活/死亡→自然終局/cutoff優先→gift/次手番とします。自己aliasも逐次、反射で品/priceを変えず、配送は環境RNGを使いません。
配送先の実物満杯は明示unsupported（-5）/sampler除外です。自己は二品消費後の空きを数え、旧Flashの配送失敗無視を移植しません。現行clientのoverflowItemから実物eviction規約を確定できないため、完全SELL対応とは主張しません。`last_sale={origin,source,target,model,price}` は解決後だけ公開し、次main受理/reset/loadでclear・失敗保持・解決済みcutoff保持/未解決null。last_danger/last_removalとは排他で、token/他手札/RNGは公開しません。

公開 `sell_rule` は `experimental:one-unused-commodity;explicit-alive-target;zero-mp-fee;one-operation-gift;foreign-response-and-reflection;buyer-debit-before-current-source-credit;fresh-token-delivery-before-death;full-recipient-unsupported`。商品の232/Aura231/MP武器58/杵94/復活208等のabilityを売却payloadの構成時に実行・継承せず、単一対象・power0・属性無し・商品priceだけを用いる。配送後208の独立した救済hookは別に扱う。商品fakeも既存unsupportedのまま。構造体は同じABI2でも、clientは新opcode5の認識とruleset20のloader照合が必要である。

直接配送後の予約giftは `min(old_gift, hand_limit−new_items_count)` へ防御的に正規化する。実物を追い出す処理ではない。現scopeの正常経路では原発二品消費/gift1、190消費/gift1が作る空きにより通常は超過しない。実物満杯のunsupportedをこのclampで回避しない。

ベンチの `sell_plays` は操作回数、`sell_resolved` は自己即時/他者空応答で最終bodyが完了した数（190反射途中は除く）、`sell_nominal_cp` は名目price総額で実CP支払/HP損失/加算clip量ではない。`sold_miracles` は配送商品が奇跡だった件数でcast/学習ではない。`sold_miracles<=sell_resolved<=sell_plays` を確認する。v20の `physical_used` はSELL一回につき操作4だけ+1、商品は移送として除外し、通常play/defend/exchangeの選択使用や奇跡first/recastへ混ぜない。

v21は購入5を加え216モデル/重み477（trade3/98武器/74防具/16雑貨/25奇跡）、Snapshot21旧1〜20拒否/view-v19、Match ABI2同サイズです。kind buy6/accept7/decline8、phase purchase3を追加しfinished2は維持します。`Command(actor, kind="buy", items=(buy_token,), target=seller)` は本人以外のalive targetに限り、自己BUYは明示unsupported（-5）。単品5/MP0/gift1、play5・MC/FREE・allocationは不可です。
190反射後の現在sourceがbuyer/targetがseller。最終bodyでused奇跡を除く手札一個を抽選し、元slot/tokenを特権escrowへ一時預かりします。空はdraw無し/見積無し、1候補もdraw1回。CP不足なら元の品を戻して金銭移動無しで終了。購入可ならbuyer actorのpurchase phaseと公開 `{origin,buyer,seller,model,price}` を作り、buyerが空items/target無しのaccept/declineを選びます。
YESはbuyer CP減→seller名目全額CP加算cap99→buyerに新token unused商品を配送→既存復活/死亡/gift等。NOは元slot/tokenへ返し、未完了cutoffでも返却してgift取消、配送/返却自体のRNGは0です。満杯buyerへのYESは-5/NOは可。商品MCはcast/学習でなく、商品giftも無し。`last_purchase={origin,buyer,seller,model,price,bought,reason}` のreasonはbought/declined/empty/insufficient-cp/cutoff、emptyはmodel0/price0、商品未選定cutoffはnull。見積中は完了event無し、次main受理/reset/loadでclear・失敗保持・他完了eventsと排他です。
usedMC除外・YES/NOとbuy/setBought/canNotBuy表示は公開clientの根拠ですが、抽選/不足CP終了・自己対象拒否・escrow同一性・cutoff/返却scheduleはserver未確認の実験規約です。原slot/token・RNG・候補数・非選定手札は公開せず、完全BUY対応とは呼びません。

公開 `buy_rule` は `experimental:foreign-only;one-operation-gift;uniform-unused-commodity;empty-no-draw;cp-only-affordability;buyer-accept-or-decline;escrow-return-on-decline-or-cutoff;fresh-token-delivery;full-accept-unsupported`。`bought` はreasonがboughtかどうかから派生するbooleanで、独立した履歴flagではない。特権 `PurchaseOffer` はoriginal item（token/model/used=false）と原slotを保持するが、公開purchaseにはこれらを追加しない。承認/拒否も新しい意思決定として既存cutoff規約に従う。

購入決定をjournalへ一件加えるため、BUY actionだけ既存の保守的response/journal boundへ+1する。非BUYの最大3,725を変えず、全体の保守的な上界へさらに+1を見込めば3,726となる。実際のBUYは単一target・190の物理反射だけなので、これより小さい。購入承認でも商品配送後のgift予約を `min(gift, hand_limit−new_items_count)` に正規化し、実物evictionを行わない。

ベンチは `main_commands+responses+purchase_commands=decisions` へ三分離する。`buy_plays` は操作数、`buy_quotes` は公開見積phase開始数、`purchase_commands` はaccept/decline数、`bought`/`declined` は各決定結果、`buy_empty`/`buy_insufficient_cp`/`buy_cutoff` は各完了reason、`buy_nominal_cp` はboughtの名目price総和（sellerのclip後実増分ではない）。`bought+declined=purchase_commands<=buy_quotes<=buy_plays`。`physical_used` はBUY操作5だけ+1で、商品移動・購入決定・奇跡cast/学習を加えない。v20以前のmain+responses=decisionsは過去版の定義として残す。

v22は病気4状態Cold/Fever/Hell/Heavenと付与六モデル30/33/48/62/219/220を加え、222モデル/重み483（trade3/102武器/74防具/16雑貨/27奇跡）、Snapshot22旧1〜21拒否/view-v20とします。v22時点で168selfCurse/198MP+Heavenは未対応だった。Match ABI2・opcode0〜8・phase3を変更しません。
病気無しならincoming、既存ならmax(incoming,current+1)、Heaven再付与はHP0/Heaven維持。非病気Flash/DarkCloudは独立でHeavenfallを起こさず、mildはCold/Fever/Flash、allは対応六curseを解除します。再付与fallをcounter基準へ加えず、regular target HP被害だけを使い208救済後のsurvivorを判定します。
元ownerの全action完了後advance冒頭だけ、生存/病気ありなら固定5%（bounded100<5）で悪化→変化後Cold−1/Fever−2/Hell−5/Heaven+5、HeavenfallはHP0でheal無し→208/死亡→自然終局→gift/次owner。body自然終局/decision cutoff成立時はtickせず、反射/連撃/全体/counter/BUY見積の途中にもtickしません。購入決定で完了しても元ownerだけが対象です。教典の5%/継続量と、このrank/抽選順/境界のserver未確認実験規約を区別し、旧worseChance増加式は採用しません。
native last_tickはbodyとtickのHP変化・蘇生を分ける特権情報で、内部record/token/RNGは公開しません。公開はdisease_ruleとboardの状態だけです。

公開 `disease_rule` は `experimental:one-disease-plus-flash-and-darkcloud;reapply-max-incoming-or-next;heaven-reapplication-hp0;owner-only-after-whole-action;natural-and-cutoff-before-tick;fixed5-percent-before-hp;post-upgrade-tick;post-tick-revival-before-gifts`。raw stateは病気最大一種→Flash→DarkCloudのenum順・重複無し、最大三curseのcanonical集合だけを受理する。tick recordは次main受理でclearし、現在の公開boardから過去のroll/tokenを復元して返す機能は作らない。 現行clientの病気表示は新たな受信curseで旧病気一種を置き換え、非病気Flash/DarkCloudは独立に残す。旧FlashのHeaven中に非病気を付けても転落させる挙動は流用しない。

ベンチの `disease_ticks` / `disease_worsened` / `disease_falls` はtick/自然悪化/Heavenfall件数、`disease_tick_revivals` はtick由来208救済数。`disease_hp_gained/lost` は復活前のtick HP増減を集計し、救済時はrawAfter=0として208の+10を除き、Heavenfallの実HP損失も含める。累計 `revivals` はbody由来とtick由来を含む。HP増減202のbody集計からtick HPdeltaとtick救済を外し、205/206のtoken消失数から同targetのtick208消費を除いて公開除去eventと照合する。これらの内部会計のためにlast_tick/tokenを利用しても、公開JSONには出さない。

v23は168熱狂仮面（Fire普通DEF10/自己Fever/p10w1）と198天国草（NE雑貨/MP20+Heaven/p20w1）を加え、224モデル/重み485（trade3/102武器/75防具/17雑貨/27奇跡）、Snapshot23旧1〜22拒否/view-v21とします。Match ABI2の幅・commands0〜8・phase3は据置、guardian/MP費用はありません。
168はmain不可、属性適合時の普通防御に複数copy可、Flashは一枚、filter前置可/特殊反応併用不可。全response消費後に選択順で自己Feverを各copy一回→incoming body→208/死亡とし、完全防御でも自己付与します。prefixHeavenfallでHP0になっても本体前に蘇生せず、HP0&&!deadを待機snapshotに許しません。counterはbodyのtarget HP実損失だけで、prefix落下を加算しません。自己吸収aliasがHP0→回復→被害を起こす場合の正のbody損失は別に数えます。
198はmain play専用（response品には不可）でMP20clip99→Heaven付与→救済/死亡、自己即時/他人190のみ。能力自体にRNG/holding/FREEは無く、後のowner tick/giftは別。discard/売買商品では両品のabilityは発動しません。v22の病気/5%/owner境界・自然終局/cutoff・救済順を維持します。
公開 `disease_item_rule='experimental:response-self-curse-in-selection-order-after-consumption;prefix-before-body;death-and-revival-after-whole-body;counter-basis-body-target-loss-only;mp-boost-before-disease;no-trigger-on-trade-or-discard'` に付与順を実験規約として明示します。現行selfCurseはevent.indexの演出だけで、198のtooltip/独立MP・curse受信handlerもserverの付与順/HP0/208までを証明しません。 198は全response後のbodyでMPを増やし、nonattackへの反応は190だけなので同batchの有償MP反応予算を増やさない。journal上界は非BUY3,725/BUY3,726の保守的上界を維持する。新ベンチ `fever_mask_uses` は応答で使った168コピー数、`heaven_herb_plays` は198のmain開始数で、捨てる/商品移動を含めない。`heaven_herb_resolved` は自己即時または他者空応答の最終body解決数（190反射途中は除く）、`heaven_herb_mp_gained` は当該targetのMP after-before実増分で、99cap後の値を数える。同body後のtick/giftはMPを変えないため分離でき、名目20倍値に置き換えない。resolved<=plays/実増<=20*resolvedを検査する。未解決cutoffなら開始だけ、解決済みcutoffならbody実増も計上する。

意思決定の打切りは自然終局と分け、報酬0です。不正・未対応入力で状態/RNG/出力を一部更新しません。
raw scene/checkpointのimport・restoreはC/Python公開APIへ追加していません。

`python3 python/official_match.py` はローカル実行例です。C++ベンチマークは
`./build/godfield_official_bench GAMES PLAYERS SEED WORKERS`、省略時は `10000 2 42 1`。
これは限定プールとローカル方策での実行速度を測り、公式全モデル・対局trace・サーバーとの一致や学習性能を示すものではありません。

以下は過去の **v1・132モデル/重み308** のRelease計測（seed42、1 worker）です。
現行v23の性能やchecksumを示すものではありません。

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
これらのv1〜v8結果は現行v23の224モデルの速度・checksumではありません。

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
以上のv4計測も過去の証拠として保持し、224モデルのv23の計測とは分けます。
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

## v23の検証と仮面/天国草集計

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

詳細は[限定実行器v23記録](docs/official-execution-subset.md#v23の検証と仮面天国草集計)。

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
3. 閃光/暗雲以外の災いcounter・吸収/閃光以外の固有効果付き確率攻撃・自己購入・満杯配送等の未対応取引・霧/夢・守護神・終末等を含む9人/チームの全範囲へ広げる。限定playableと公式全体の一致を別に検証する。

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
