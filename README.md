# Godfield RL core

God Fieldを強化学習で扱うための、C++20製ローカルシミュレーターです。
描画・通信・外部ライブラリに依存せず、自己対戦を高速に回すための基盤を用意しています。

**旧Flash版を参考に、全7属性・回復/攻撃/追加ATK/倍化/全体化奇跡の保持・順序付き追加・物理品238の追加ATKとMP無料化・確率攻撃・反撃6種・HP吸収・HP増減賭け・購入確認と強制売却・杵/臼・自己被害付き武器・手札除去・忘却・資源交換・自動蘇生・物理品による魔法跳ね返し・魔法/万能反射・病気・一部harmsまで実装しています。現行公式の完全再現ではありません。**

| ルールセット | 用途 | 神器・接口 |
| --- | --- | --- |
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

新ルールは旧版の重みを各profileの対象プール内で正規化して抽選します。公式の配布分布とは扱いません。
ここで学習した方策が本家でも強いとは主張できません。
まだ学習器・学習済みモデルはなく、ランダム合法手による実行・ベンチマークができます。

旧版系のC++コアは共通1317特徴・Snapshot version35です。既存ABIの出力幅は変更せず、
`gl_*` / `gm_*` / `ge_*` / `gc_*` / `gr_*` / `gd_*` / `gh_*` / `gs_*` / `go_*` / `gb_*` / `ga_*` / `gn_*` / `gk_*` / `gx_*` / `gy_*` / `gz_*` / `gu_*` / `gv_*` / `gw_*` / `gp_*` / `gq_*` / `gt_*` / `gi_*` / `gj_*` / `gg_*` / `g26_*` / `g27_*` / `g28_*` / `g29_*` / `g30_*` / `g31_*` / `g32_*` / `g33_*` / `g34_*` は128 / 164 / 186 / 210 / 236 / 258 / 283 / 301 / 334 / 356 / 375 / 393 / 515 / 661 / 679 / 696 / 713 / 732 / 750 / 903 / 924 / 945 / 993 / 1017 / 1043 / 1062 / 1085 / 1110 / 1135 / 1175 / 1221 / 1251 / 1278 / 1300特徴を維持します。
各profileで無効な拡張の末尾は0になり、旧ABIは対応する先頭部分だけを返します。
Snapshotは同じビルド・Config内のfixture用であり、旧形式を自動移行する永続checkpointではありません。

## ビルド・実行

macOS/Linux、CMake 3.20以上、C++20コンパイラーが必要です。パッケージのダウンロードは不要です。

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
ctest --test-dir build --output-on-failure
./build/godfield_legacy_bench 100000 4 42 sundry-boost
python3 python/sundry_boost_env.py
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
defensive-miracles / attribute-change / attribute-removal / counter / counter-extensions / money-counter / magic-reflection / any-reflection / mana-weapon / magic-block / additive-miracles / doubling-miracles / wide-miracles / magic-free / forgetting / exchange / revival / item-removal / hp-absorption / self-damage / pestle-mortar / selling / buying / hp-gamble / magic-bounce / sundry-boostも選べます。
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

## 旧版を参考にした中立ルールセット

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

カードIDは公式IDと関係ありません。武器3種（威力3/6/10）、防具3種（防御3/6/10）、
回復2種（HP+5/+10）、MP回復1種（MP+5）、回復魔法1種（MP6でHP+10）の全10種。
全種類を等確率・復元抽出します。開始手札にも補充にも同じ分布を使います。

防御終了時に攻撃側→防御側の順で消費枠を補充し、防御側が次の通常手番を得ます。
回復・捨ては即補充して手番終了。パスでは補充しません。防御で使用した防具は全て消費します。
回復と捨てによる手番終了も本環境の独自仕様です。

属性相性、奇跡の攻撃、神器の固有効果、反射・跳ね返し、状態異常、守護神、
売買・所持金、複数人戦、公式の神器出現分布・補充条件などは未実装です。
公式サイトへの接続・アカウント操作はありません。第三者実装・画像は同梱していません。

## 学習インターフェース

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

1. 現行版の神器データとルールを根拠付きで整理し、独自ルールとの差分を固定する。
2. 神器分布・補充・属性・固有効果を追加し、具体的な対戦例で遷移を照合する。
3. バージョン別のrulesetを保持したうえで、自己対戦学習・固定相手との評価を追加する。

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
