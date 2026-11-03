# 現行公式データによる限定実行器 v47

**公式公開資料のv2同期（2026-11-02）:** [official-v2](../data/official-v2/README.md) に最新版JA/JSの出典を別世代として固定し、ID19/26/117の画像名と117「石板」を反映した。296モデルの数値・能力・curse・giftRate・ID順はv1から変更0。[v1→v2比較](../data/official-v2/upgrade-review.json) を保存し、旧official-v1のpins・crosswalk・legacy37 fingerprintは変更しない。新JSの抽出関数/分岐の再照合は下記v27節に記録する。全JS・全UI・権威的serverの同値性を保証するものではない。過去のfresh一致記録と各版の計測は、当時の取得物に対する履歴として保持する。

この実行器の `official-execution-subset-v47` は、**公式公開台帳と確認できたクライアント計算を使う実験用の部分環境**である。scopeは `experimental:236-default-or237-opt-in;external207;external-devils;threshold-sacrifice;external-reserved-grants;not-official-server-equivalence`。公式サーバーと同じターン進行、補充、抽選、死亡順を再現したという主張ではない。

v1は132モデル/重み308、v2は物理反射6種と属性消去189を加えた139モデル/重み319、v3は遮断9種を加えた148モデル/重み328。v4はFREE5種・属性付替2種・倍化231を加えた156モデル/重み337。v5は跳ね返し8種を加えた164モデル/重み351。v6は能力noneの確率攻撃21種を加えた185モデル/重み372。v7は攻撃反撃181/184を加えた187モデル/重み374。v8は資源反撃187/188を加えた189モデル/重み376。v9はHP吸収4種・同被害53を加えた194モデル/重み381。v10は閃光付与/反撃/解除7種を加えた201モデル/重み404。v11は暗雲付与/反撃3種を加えた204モデル/重み407。v12は未使用手札/使用済み奇跡の除去2種を加えた206モデル/重み409。v13はMP武器58と二回攻撃14を加えた208モデル/重み411。v14は全敵化232を加えた209モデル/重み412。v15はHP増減202を加えた210モデル/重み414。v16は自動復活208を加えた211モデル/重み415。v17は杵94/臼209を加えた213モデル/重み417。v18は両替3を加えた214モデル/重み437。v19はモデルと効果遷移を増やさず、除去結果の公開要約を加える。v20は売却4を加えた215モデル/重み457。v21は購入5と確認phaseを加えた216モデル/重み477。v22は病気4状態と付与6モデルを加えた222モデル/重み483。v23は自己病気防具168/MP+Heaven雑貨198を加えた224モデル/重み485。v24はFog付与4モデルと観測/対象選択を加えた228モデル/重み489。v25は112の通常/死亡時攻撃を加えた229モデル/重み490。v26は同じ229モデル/重み490で取引配送overflowとFogのforeign SELLを扱う。末尾のv1〜v25計測・検証記録は各版の歴史的な証拠として残し、v27の検証結果へ読み替えない。v27は公式台帳v2・最新版clientの照合・静的provenanceを同期し、ゲーム規約と229モデル/重み490を変更しない。

一次根拠があるものと、動かすためにこの実行器が定義する規約を分ける。

| 区分 | 内容 |
| --- | --- |
| 公式公開データ | 296モデルの属性・数値・MP・giftRate。旧Flashの数値IDとは別体系 |
| 実クライアントで確認したローカル計算 | `tw/pq/l9/tx/la` の表示威力・属性・MP、visible append、防御適合、HP/MP/CPの加減と不足時の資源連鎖 |
| クライアントの消費表示 | 通常品の消去と空枠、奇跡のused保持、表示枠が空いている場合の補充用空枠、捨てる時の空枠を作らない処理 |
| この実行器の実験規約 | 初期配布/資源/上限、配布対象プール、乱数器、固定入力順の主手番、全効果解決・死亡・補充・次手番の順、通常被害の残余式、終局と報酬、意思決定の打切り |

公式JSのイベントhandlerは、受信した `useAttackItems`、`dealDamage`、`die`、`gift`、`advanceGF` 等を順に表示する。handlerの存在だけでは、それらを権威的サーバーがどの条件・順序で発行するかは確定しない。たとえば通常被害は `event.damage` を受け取って減算しており、サーバーのATK−DEF算式を独立に検証する処理ではない。[一次調査](official-current-rules.md)と[fresh viewの契約](official-state-view.md)も参照する。

## 対象モデルと拒否する機能

[official_match.hpp](../include/godfield/official_match.hpp) の `match_supported` / `match_catalog` が既定の236種配布対象を定義する。既定配布対象は236モデル、giftRateの合計499。`include_fate=true` では207も含む全positive-giftRate237モデル/重み500を使う。category別のsundriesが18→19となり、trade3/weapons107/armor78/miracles30は同じ。207の効果解決は既存のFate専用入口を使い、通常Commandのcapabilityへは含めない。台帳の配布massへの対応と、serverの乱数・Dream・補充scheduleの同値性は区別する。

v1の基礎集合は、正のgiftRate、命中率0、categoryがweapons/armor/sundries/miracles、abilityがnone/boostHP/boostMP/boostCPのいずれかという条件である。固定済みの現行台帳に対するこの基礎条件が132モデル/重み308を作り、v2はさらに後述7モデルを加える。属性は省略属性を含む全7種類を扱う。確率攻撃は後述v6の21種とv9の吸収98とv10の閃光224に限り、未対応能力を持つカードの一部roleだけを黙って通常品として使う処理はしない。正確な対象可否は名前の推測ではなく公式モデルIDで照会する。将来別版の台帳へ同じ条件を適用するだけで、自動的に互換範囲を広げる契約ではない。

上記のv1集合へ、次の7モデルを追加する。これらは反応時だけの断片的なモデルではなく、武器の通常ATKや非matching時の防具DEFも扱う。

| 現行ID | 画像識別子 | 能力 | 通常の値 | giftRate |
| ---: | --- | --- | --- | ---: |
| 38 | reflection-sword | reflectWeapon | 無属性ATK10 | 3 |
| 39 | moonlight-axe | reflectMiracle | 無属性ATK10 | 1 |
| 156 | moonlight-helm | reflectMiracle | 無属性DEF8 | 1 |
| 167 | moonlight-shield | reflectMiracle | 無属性DEF10 | 1 |
| 174 | moonlight-armor | reflectMiracle | 無属性DEF12 | 1 |
| 190 | super-mirror | reflectAnything | DEF0 | 1 |
| 189 | rainbow-curtain | filterAtkElement | DEF0 | 3 |

v3では次の9モデルを追加する。v2と重複せず、全てgiftRate1・無属性。物理8種の価格は15でMP費用無し、奇跡233はMP6。

| 現行ID | 画像識別子 | 能力 | 通常の値 | MP |
| ---: | --- | --- | --- | ---: |
| 42 | angel-knife | blockMiracle | ATK11 | 0 |
| 50 | angel-sword | blockMiracle | ATK13 | 0 |
| 55 | angel-axe | blockMiracle | ATK15 | 0 |
| 56 | angel-bow | blockMiracle | ATK15・追加ATK15 | 0 |
| 165 | angel-gauntlet | blockMiracle | DEF9 | 0 |
| 171 | angel-cap | blockMiracle | DEF11 | 0 |
| 177 | angel-shield | blockMiracle | DEF13 | 0 |
| 179 | angel-armor | blockMiracle | DEF15 | 0 |
| 233 | wall | blockWeapon | 主動作不可の防御奇跡 | 6 |

v4では次の8モデルを加える。204のみgiftRate2、残りは各1で、v3と重複しない。

| 現行ID | 画像識別子 | 能力 | 通常の値/属性 | MP |
| ---: | --- | --- | --- | ---: |
| 46 | spiritual-staff | cutCost | 無属性ATK12、単独武器可 | 0 |
| 66 | wand-of-ignition | setElement | 火ATK2・追加ATK2 | 0 |
| 83 | wand-of-mystic-water | setElement | 水ATK5・追加ATK5 | 0 |
| 143 | spiritual-socks | cutCost | 無属性DEF6 | 0 |
| 163 | spiritual-hood | cutCost | 無属性DEF9 | 0 |
| 173 | spiritual-sash | cutCost | 無属性DEF12 | 0 |
| 204 | spiritual-doll | cutCost | 雑貨、単独主動作不可 | 0 |
| 231 | aura | doubleAtk | 無属性・追加専用奇跡、累積ATK×2 | 6 |

v5は次の8モデル/追加重み14を加える。全て無属性で、21のみgiftRate7、残りは各1。奇跡234以外はMP費用無し。

| 現行ID | 画像識別子 | 能力 | 通常の値 | MP |
| ---: | --- | --- | --- | ---: |
| 21 | bouncing-sword | bounceWeapon | ATK5、追加ATK無し | 0 |
| 34 | sky-harpoon | bounceMiracle | ATK9・追加ATK9 | 0 |
| 114 | sky-boots | bounceMiracle | DEF1 | 0 |
| 125 | sky-gauntlet | bounceMiracle | DEF3 | 0 |
| 137 | sky-helm | bounceMiracle | DEF5 | 0 |
| 151 | sky-shield | bounceMiracle | DEF7 | 0 |
| 164 | sky-armor | bounceMiracle | DEF9 | 0 |
| 234 | turbulence | bounceMiracle | 主動作不可の防御奇跡 | 5 |

v6はabilityがnoneで命中率が正の次の21モデルを加える。全てgiftRate1、追加重み21。武器15種・奇跡6種で、状態付与や吸収等を持つ確率攻撃は含めない。

| 現行ID | 画像識別子 | category | 属性 | ATK | 命中率 | MP |
| ---: | --- | --- | --- | ---: | ---: | ---: |
| 95 | spark-bag | weapons | 火 | 1 | 75 | 0 |
| 96 | shadow-hand | weapons | 闇 | 2 | 50 | 0 |
| 99 | lightning-kids | weapons | 光 | 3 | 25 | 0 |
| 100 | flame-cup | weapons | 火 | 4 | 75 | 0 |
| 101 | chill-cup | weapons | 水 | 4 | 75 | 0 |
| 102 | plant-cup | weapons | 木 | 4 | 75 | 0 |
| 103 | rock-cup | weapons | 土 | 4 | 75 | 0 |
| 104 | oversize-snowball | weapons | 水 | 5 | 50 | 0 |
| 105 | cliff-hammer | weapons | 土 | 6 | 25 | 0 |
| 106 | light-orb | weapons | 光 | 6 | 75 | 0 |
| 107 | fire-shower | weapons | 火 | 7 | 75 | 0 |
| 108 | jinn-s-rocking-horse | weapons | 木 | 8 | 75 | 0 |
| 109 | rain-deity-s-saber | weapons | 水 | 9 | 50 | 0 |
| 110 | flare-axe | weapons | 火 | 10 | 50 | 0 |
| 111 | petit-saturn | weapons | 土 | 20 | 25 | 0 |
| 225 | smoke | miracles | 火 | 5 | 75 | 4 |
| 226 | avalanche | miracles | 水 | 8 | 75 | 6 |
| 227 | thunder | miracles | 光 | 10 | 25 | 4 |
| 228 | mudflow | miracles | 土 | 12 | 50 | 6 |
| 229 | magma | miracles | 火 | 15 | 75 | 10 |
| 230 | ice-age | miracles | 水 | 30 | 75 | 30 |

108は通常防御として木DEF6も保持する。確率武器へのboost/setter/Aura追加はclientの保証命中基底条件で不受理。確率奇跡の直後FREEは許可し、費用・used保持は既存MC経路を使う。

v7は181（mars-ring、火、counterAtk、命中75%）と184（saturn-ring、土、counter2xAtk、必中）を加える。どちらもcategoryはarmor、通常ATK/DEF/MP0、価格10・giftRate1。単独主動作ではなく防御品として選び、元の属性適合に従う。v8はさらにMP反撃187/CP反撃188を加え、災い反撃等はまだ含めない。

v8は次の2モデルを加える。どちらもarmor・無属性・ATK/DEF/MP0・価格10・giftRate1。guardianはカードの表示metadataであり、playerへ守護神を宿す能力ではない。

| 現行ID | 画像識別子 | 能力 | guardian metadata | 通常被害を受け生存したときのローカル生成量 |
| ---: | --- | --- | --- | --- |
| 187 | neptune-ring | counterBoost2xMP | neptune | 実HP減少×2のMP、power0 |
| 188 | venus-ring | counterTakeCP | venus | 実HP減少のCP、power0 |

v9は次の5モデルを加える。全てgiftRate1、普通DEF/追加ATKの役割は無い。

| 現行ID | 画像識別子 | category | 属性 | ATK | 命中率 | MP | 価格 | 能力 |
| ---: | --- | --- | --- | ---: | ---: | ---: | ---: | --- |
| 27 | ghost-sword | weapons | 無属性 | 7 | 0 | 0 | 10 | absorbHP |
| 45 | real-ghost-sword | weapons | 無属性 | 12 | 0 | 0 | 15 | absorbHP |
| 98 | vine-shoot | weapons | 木 | 3 | 75 | 0 | 10 | absorbHP |
| 216 | absorption | miracles | 光 | 10 | 0 | 10 | 0 | absorbHP |
| 53 | evil-broadsword | weapons | 無属性 | 14 | 0 | 0 | 1 | dealSameDamage |

216は奇跡のused保持・再使用/直後FREEを扱う。98は既存chanceのtarget無し全敵batchを使い、確率武器への追加は許さない。

v11は41（hexagon-doom、無属性武器ATK11/addCurseOnDamage暗雲、価格15）、186（pluto-ring、闇armor/ATKDEF0/counterCurse暗雲、価格10）、223（dark-cloud、闇奇跡/power0/addCurse暗雲、MP5）を追加する。全てgiftRate1で、追加3モデル/重み3。186の闇は属性表示であり、power0の純状態payloadを闇即死damageへ読み替えない。

v12は205夜空のホウキ（nocturnal-broom/removeItems）と206女神の石けん（goddess-s-soap/removeUsedMiracles）を加える。どちらもsundries・無属性・power0・MP0・価格10・giftRate1。台帳のabilityValueは0だが、効果量の上限は現行JA/EN説明から3/2を派生させ、旧版の1枚除去を使わない。

v13は58マジカルステッキ（NE武器/基礎ATK0/atkBy2xMP/価格10/giftRate1）と14のこぶんぶん（NE武器/ATK3/attackTwice/価格10/giftRate1）を加える。どちらも追加ATK roleを持たない。

v14は232＜蜃気楼＞（miracles/NE/ATK0/isPlusAtk/attackEveryEnemy/MP5/giftRate1）を加える。単独で使わず、必中weapon列への追加だけを扱う。

v15は202ドキドキ涙（sundries/NE/ATK0/boostHPOrDealDamage/値10/MP0/価格1/giftRate2）を加える。

v16は208太陽のお守り（sundries/NE/revive10/MP0/価格10/giftRate1）を加える。手動play/response/discardには使わない。

v17は94あぶないキネ（Light weapon/ATK30/danger/p5w1）と209あぶないウス（sundries/attractDanger99/p1w1）を加える。ただし94+232の組合せは明示unsupportedとする。

v18は3両替（trade/exchange/MP0/価格5/giftRate20）を加え、専用kind exchangeで使用する。v20は4売却（trade/sell/MP0/価格5/giftRate20）を加え、専用kind sellで商品一個を指定する。v21は5買う（trade/buy/MP0/価格5/giftRate20）と購入確認phaseを加える。

全プレイヤーのraw手札を含め、実行状態に207のunused/unmasked特例を除く未対応モデル、互換クラス外または不正なfake、病気4種/Fog/Dream/閃光/暗雲以外の災い、不正な守護神identity、不正な混乱count、負/unsafeな終末閾値等があれば拒否する。本人に見えない他者の未対応カードでも、この実行器が将来通常品へ誤変換して進めることはない。`MatchError` の `unsupported` と、不正な入力を示す `invalid` は区別する。

互換クラス外Dream・Earth/Moonの一般神器/奇跡選出・207の現象抽選・混乱の自動選択・磁気嵐の自動分配・悪魔・終末等を、台帳にあるという理由だけで実行可能とはしない。`official_state` のread-onlyな9人状態表示が扱う範囲と、この部分実行器が受理する局面は別である。

## 初期化と入力状態

`MatchConfig` の既定値は初期手札9、HP40/MP10/CP20、総手札表示上限18、意思決定上限512。HP/MP/CPや9/18には公式表示側の根拠があるが、**この組合せのサーバー開始状態・実配布数・物理容量を対局traceで検証したものではない**。ここでは実験用の初期条件として固定する。usedの奇跡も総手札数へ含める。

設定可能域は意思決定上限1以上、手札上限1〜18、初期手札0〜その上限、初期HP1〜99、初期MP/CP0〜99。初期手札0はテスト等のための実験設定として受理する。

プレイヤー数は2〜9人。入力列の順に扱い、PlayerIdとTeamを保持する。主手番の巡回もこの入力順に固定する。公式サーバーのシャッフルや順番抽選を再現したという意味ではない。seedはこのローカル実行器の抽選を再現するものであり、公式サーバーの乱数列と対応しない。

resetでは先頭プレイヤーをowner、GFを1にし、入力プレイヤー順に初期手札を配る。次ownerは現在ownerの次から巡回して最初の生存者を選び、GFを1増やす。seedを使う配布はこの実行器のSplitMix64と偏りを除いた有界抽選による。方策のサンプリング用乱数は配布用の乱数とは別に渡す。

`reset` は指定プレイヤー列とseedで開始する。`load_scene` は対応範囲内の既知局面を主手番ownerとseed付きで取り込む。raw snapshotを取り込めたことは、その局面が公式サーバーで到達可能と証明されたことを意味しない。

開始時から全員が同じ非Soloチームなら競合相手が無いため、即時に自然終局して全員をwinnerとする。取り込むsceneでも安定した局面の自然終局を先に判定し、全員deadならdrawを認める。HP0なのにdead=falseの中間sceneは、このclosed runtimeに必要な死亡待ちqueueが無いので拒否する。これらもローカル規約である。

`load_scene` は入力のisOverがfalseの安定sceneを受け取り、この実行器の終局規則で結果を判定する。既存のalias/userIdはraw state内に保存し、表示には既存のview規則を用いる。資源0〜99・設定内の総手札数・usedを持てるのは奇跡だけ、といったclosed runtimeの制約も検査する。

## 行動・選択・応答

`Command` はactor、kind、順序付きの本人instance token列、任意の明示targetを持つ。modelIdを物理tokenの代わりに使わない。同じモデルの別コピーと、同じtokenの重複を区別する。kindは `play`、`defend`、`discard`、`pray`、`exchange`、`sell`、`buy`、`accept`、`decline`、`sacrifice`（opcode9）。exchangeだけが型付きallocationを持つ。

exchangeはtarget無しの自己再配分とし、通常のplayとsellは生存targetを明示し、buyは自己を含む生存targetを明示する。自己BUYは即時照会で金銭移動や購入確認を行わず、他者BUYは応答後に購入見積/accept/declineへ進む。v17の94はtarget無し/既知aliveヒントを受理し、その指定を選択に使わない。確率攻撃playはtarget無しに限定し、v14の232を含むweapon列はtarget無し/任意の既知alive targetを受理して指定をrosterに反映しない。ただし94＋232の同列は未対応として拒否する。owner/actor/phaseと本人の所有品を検査する。表示モデルの選択・MP・category制約には公開クライアント由来の関数を使う。既存 `official_state` のappend照会だけで済ませず、実行器の現在phaseも検証する。

臼誘引による即時bodyを除き、通常の武器やATK奇跡は、自己対象でも解決待ちのeffectとresponse phaseを作る。確率攻撃では各生存敵への命中を先に抽選し、hitした対象にだけresponseを作る。応答者はそのtargetであり、ownerとactorが同じ人でもphaseを分離する。普通防御では適合する品の選択順・合成と所有権を検証する。v2から物理反射と属性消去、v3から遮断、v5から跳ね返しを扱う。旧版の自己攻撃時の応答省略を暗黙に流用しない。

HP/MP/CPを増やす品は、自己対象なら即時解決する。他人を対象にした場合には応答待ちを作る。v1では空応答だけ、v2から適合する物理反射、v3から奇跡に対する魔法遮断、v5から奇跡への跳ね返しを許す。普通防具で回復量を減らさない。非攻撃の回復へ属性消去を前置することはUIのincoming_is_attack制約により許さない。

prayは本人の手札に武器がない時に許可する。v16は満杯でも受理し、その場合gift0/環境RNG0で通常の手番・cutoffへ進む。v1〜15の空き必須というscope制限は維持しない。閾値前のdiscardは対象の制限を検査して捨て、補充予定を付けない。終末activeではdiscardに代えてsacrificeを使い、選択copyごとに補充を予約する。旧Flashの「捨て枚数に応じた配布」を公式ルールとして移さない。

discard/sacrificeのitemsは擬似モデル「捨てる」「ささげる」自身を含めず、実instanceを1個以上列挙する。discard/sacrifice/pray/defend/exchangeはtarget無し、prayはitemsも空である。自分、味方、敵のいずれであっても、通常playは生存targetを明示する。確率攻撃playにはtargetを付けず、明示target付きのaliasは拒否する。232を含むweapon列と94は前述の任意target規約を使う。対象の敵味方による別制限をサーバーから検証した主張ではない。

## 消費・奇跡保持・補充

消費する通常品は、本人の物理instanceを除去し、1個につき補充予定1を作る。奇跡は初使用時にそのinstanceへusedを付けて保持し、再使用でも同じtokenを参照する。奇跡は総枠に余裕がある場合だけ補充予定を作る。18枠あるときに、usedとして残す品へ無条件にさらに1枚を授けない。

この対応は `fG.ee`（固定公式JSの12694–12735行）の消費予告・placeholder処理を参考にしている。サーバーが別イベントで補充数を決める全規則を取得したという意味ではない。奇跡のMPは公開クライアントの費用計算で支払い、支払えない行動は実行前に拒否する。

補充は未解決effectが無くなった後にまとめて行う。対象は生存者に限り、自然終局している場合には配らない。入力プレイヤー順とこの実行器のseedで再現できるようにする。手札上限を超えた配布や、未対応モデルへのfallbackは行わない。この補充時点・順番は実験規約である。

## 資源計算と効果解決

`change_resource` のHP/MP/CP更新は公式クライアント `c5.eW/dH/bc`（13234–13244行）から確認できるローカル算術を使う。

- HPの加減は0〜99へ制限する。
- MPの加減は上限99。減算で0を下回る分はHPへ請求し、MPを0にする。
- CPの加減は上限99。減算で0を下回る分はMPへ請求し、それでも不足すればHPへ請求する。

この資源更新だけでは自動的にdeadを立てない。公開クライアントもHPの更新と `die` を別イベントとして扱う。

通常攻撃は、合成したraw威力から普通防御の値を引き、`max(0, raw−DEF)` を残余とする。闇属性で残余が正なら対象HPを0にし、残余0なら即死させない。他属性では残余を通常HP被害として適用する。属性適合と合成は確認済みclient計算を使うが、**残余式・effect適用と死亡処理の接合はローカル実験規約**であり、権威的serverの解決traceとの一致は未検証である。

効果は `EffectFrame` にorigin/source/target、順序付きモデル列、category、属性、威力・能力を保持する。応答を待つpayloadは最大1個であり、反射/跳ね返しはそれを移動して応答を続ける。複製や新しい奇跡の再発動ではなく、一般の多段反撃・守護神・全体攻撃queueを実装したものでもない。

## v2の反射・属性消去

`reflectWeapon` は無属性の武器、`reflectMiracle` は属性を問わず奇跡、`reflectAnything` は全categoryへの反応として公開clientの `e1` が判定する。v2で入力できるpayloadは対応範囲の武器・奇跡・単純回復に限るため、万能反射の台帳説明を理由に未対応counter等を通さない。

matchingする反応品を普通DEFとして選ぶopt-outは無い。非matchingの反応付き防具は通常のDEFを使える。普通DEFの後にmatching反射を追加する列は現行clientのappendで不受理であり、反射の前置として認めるのは先頭から連続した属性消去だけである。属性消去→普通DEFは許されるが、その普通DEFの後へ反射を継ぎ足すことはできない。matching物理反射の後にFREE等の普通品を追加するsuffixも許さない。防御奇跡233→FREEという別のUI経路はv4から扱う。

属性消去は現在の攻撃属性を無属性へ変更し、その後に反射しても無属性のまま維持する。元category、power、ability、amountは変えない。例えば有色武器を消去してから武器反射で返せるが、奇跡を武器へ変換するわけではない。消去だけで防御値を加算することもない。

反射では現在のsourceとtargetを交換し、新targetが次の応答actorになる。originと元の主手番ownerは変えず、元の威力・回復量・category・能力を保つ。原発奇跡のMPを再請求したり、新sourceへ奇跡を取得させたりしない。各応答で使用した物理品はその応答者から消費し、その人の補充予定へ記録する。連鎖中は配らず、最終解決後の自然終局判定が済むまで遅延する。

自己反射もresponseを継続する。これはv1の自己攻撃時と同じ**実験規約**である。現行clientは自己がtargetの場合に別の防御者paneを作らないが、それだけでサーバーの自己反射の解決・応答省略を確定することはできない。旧版の自己省略をこの実行器へ暗黙に移さない。

v2〜v4の継続反応は物理品を消費するため、その範囲では反射品が減る。v5の保持奇跡234は同tokenで跳ね返しを続けられるため、このphysical-only予算や旧32連鎖上限を流用しない。応答来歴の上限は後述する意思決定数・物理消費・MP会計から検証し、意思決定上限のtruncationも適用する。

## v3の遮断

`blockMiracle` は現在payloadのcategoryがmiraclesであれば属性・攻撃か回復かを問わずmatchingする。現行プールの回復奇跡はHP増加235（spring）とCP増加236（treasure）で、どちらも止める。MP増加の奇跡は現行台帳に無く、MP回復雑貨を奇跡とみなして止めることはない。matchingは効果の種類ではなくcategoryによる一般predicateである。`blockWeapon` の233は現在categoryがweaponsかつ無属性のときだけmatchingする。属性消去189を前置して有色武器をNE化すれば233を使えるが、元が奇跡なら消去しても武器へ変換されない。

選択は反射と同じく、空列または先頭から連続したfilterの直後だけ。普通DEF→遮断は禁止し、matching遮断を普通DEFへopt-outしない。blockMiracleの防具を非matchingの武器へ使う場合は、その属性に適合する普通DEFを保つ。武器42/50/55/56の通常ATK、56の保証命中武器への追加ATK15も保つ。

233は主動作の単独playや攻撃追加では使えず、防御応答として本人のinstance tokenを選ぶ。FREE無しの初使用は6MPを払い、そのtokenをusedとして保持する。再使用も6MPで、取得を重ねて別の保持品を作る処理ではない。総枠と予約済み補充に空きがあれば補充予定1、満杯なら0。v4では直後のFREEで費用を0にでき、MP不足の判定も完成列の最終費用を使う。

matching遮断は現在のpayload全体を取消し、ATK被害・闇即死・HP/MP/CP回復等の元効果を適用しない。反射連鎖の途中から遮断してもその時点で終了する。原発奇跡のMPを再課金せず、既に支払ったMPや消費・保持も巻き戻さない。遮断応答の品は通常どおり消費またはused保持し、MPを払い、補充を予約する。完了後は自然終局を優先し、未終局かつ打切りでない場合だけ全員分をまとめて配り、元ownerから次手番へ巡回する。

遮断は終端応答なので `RedirectFrame` ledgerへ保存しない。反射履歴にblock応答を含めたpending snapshotは拒否する。一方、元攻撃モデル列でblockMiracle武器を普通ATKとして使うこととは区別する。

この取消・補充・owner進行は明示した実験規約である。公式JSの `block` handler（11058–11067行）は `dE` を呼ぶだけで、`dE`（9765–9772行）も音・表示用 `aZ` の後に戻る。matching条件と演出は一次で確認できるが、権威的serverが元payloadをどの順で取り消し、消費・死亡・補充を発行するかを証明するコードではない。

## v4のFREE・属性付替・倍化

### 完成列のMPと無料化

`la` は選択列中の各MCについて、直後がcutCostならそのMCの費用だけを免除する。複数のMC→FREEペアがあれば全て免除し、旧Flashの「最初のFREEだけ」を採らない。MCとFREEの間に通常boostがある場合は免除されず、先頭FREEが後続MCを無料にすることもない。

公開clientの `ev/kM/ip`（9464–9520行）は構造上の選択を判断し、MP残高を参照しない。MP不足検査はsubmit先頭（9596行付近）で完成列の `la` と本人のMPを比較する。したがってMP0でも、壁233を先に選び、直後へ未使用のFREE物理品を足して完成費用0にすれば送信可能な列になる。実行器も各prefixで費用を拒否せず、構造を通した後に最終費用を検証する。

ローカルsamplerもMC+FREEを2品の初期proposalとし、追加時に単品では支払えないMCでも、未使用のFREEとatomic pairで追加できるようにする。これは方策側の候補生成であり、ゲーム用RNGやサーバーの抽選規則ではない。

### ATK・属性・通常role

FREEを追加品として使うと、`tw/l9` はその品のATKと攻撃属性を完全にskipする。FREE武器46をMC直後へ使ってもATK12を足さない。一方、先頭46は通常武器としてATK12を持つ。FREE防具143/163/173を普通防御へ使うとDEF6/9/12を保ち、防御属性 `tx` はFREEも通常どおり合成する。FREE雑貨204は単独主動作として使えない。

setter66/83は単独の火ATK2/水ATK5武器として使え、追加時もそのATKを加算して属性を上書きする。倍化231は単独主動作不可、保証命中の武器への追加専用MC。累積ATKをその位置で2倍にし、属性は無属性との通常合成になるので、それまで有色なら無属性へ変わる。後続setterで再び色を付けることはできる。順序は単なる選択集合へ圧縮しない。

たとえば無属性ATK1→火setter2→倍化はATK6/無属性、無属性ATK1→倍化→火setter2はATK4/火になる。どちらも倍化の基本費用6で、倍化直後へFREEを置けばその6だけを免除する。使用できるかどうかは本人の別token・手札・完成列MP等を別途検査する。

### 壁→FREEの消費

responseの `ev` は「直前がMCで次がcutCost」を通常の反応後suffix制約より先に許す。壁233→FREEは、MP0も含め最終費用0で使える。matching物理反射→FREEは直前がMCではないので不可。壁→FREE→2個目FREEも不可で、次のMCが別に構造上追加できるときの複数ペアと混同しない。

壁をused保持し、FREEは物理品として消費する。最終費用0、補充は通常の物理消費・MC保持の予約と総枠上限で計算する。FREEが防具でも、matchingblockの後へそのDEFを足して被害を減らす処理ではない。受信 `useDefenseItems` は全itemsをloop後に費用を処理し、submitの `ee(null)` も全選択を消費予告する。v4はこの全消費の表示構造を参考にし、旧版の「特殊後FREE未消費suffix」は採用しない。サーバーの最終消費・補充発行順を独立に検証したという意味ではない。

元攻撃のMPは最初の完成列で一度だけ支払う。反射してもその無料化・倍化・付替後payloadを保ち、元MCを再課金・新sourceへ取得させない。保持MCは同じtokenで再利用できるが、1コマンド内で同じtokenを重複選択することは認めない。

MCを追加した武器のcategoryはweaponsのままである。FREEやsetter、Auraがcategoryをmiraclesへ変えることはなく、魔法遮断と武器遮断の判定も元のcategoryを使う。MP連動武器58や全体化232等、この8種追加に含まれない能力は引き続き対象外。

## v5の跳ね返しと実験的な対象抽選

`bounceWeapon` の21は現在categoryがweaponsかつ無属性の場合、`bounceMiracle` の34/114/125/137/151/164/234は現在categoryがmiraclesの場合にmatchingする。奇跡なら攻撃/回復・属性を問わない。matching品の普通DEFへのopt-outは無く、先頭または連続filter直後に選ぶ。非matching時のarmorは通常DEFを保ち、武器21/34の通常ATKと34の追加ATKも保つ。

公開JSのbounce handler（11000–11030行）は反射と同じ `cZ` を使い、以前の防御者を新sourceにする。元aggregateの威力/hit表示値を維持し、新targetは受信済み `event.playerId` で設定する。公式教典の説明は「攻撃を弾くと、その攻撃は誰かに向かう。」であり、候補の生存条件・敵味方・自己・一様性・乱数消費は確定しない。

この実行器は**実験規約**として、入力seat順に全aliveを列挙し、自己・味方・旧sourceを含めて一様抽選する。環境のSplitMix64と偏りを除いた有界抽選を使い、候補1人でも1drawを消費する。反射には対象抽選が無く、0draw。自己へ跳ね返っても明示的にresponseを続ける。公式serverの抽選分布や自己応答規則を再現したという主張ではない。

originと元owner、元power/category/ability、既に消去した属性は維持し、原発MCを再課金・再取得しない。1対象の未解決redirect連鎖中には死亡確定や補充を挟まない。確率batchでは各対象の完了時に死亡を確定するが、補充はbatch全体の完了まで行わない。各応答者の消費/used保持/MP/補充予約を処理し、最後に既存の解決・死亡・終局優先・補充・次owner規約へ進む。

234は初使用/再使用とも5MP、初使用は同tokenをused保持する。直後FREEなら0MPで、そのFREEも物理消費する。MP不足prefixは完成列で検査し、同じ保持tokenを後のコマンドで繰り返し使える。1コマンド中の同token重複は不可。FREEの旧未消費suffixを採用しない。

この再使用を含むredirect履歴の件数は `decisions-1` 以下、かつ `n * (hand_limit + 99/5)` 以下であることを検査する（`99/5` は整数除算）。後者は全seatの物理/FREE消費予算と、有償234の最低5MP予算を合わせた上限である。件数上限だけでなくMP・補充・消費済み物理tokenを全seatで厳密に会計する。旧physical-only `n*18` や固定32を新しい連鎖上限として流用しない。

## v6の確率攻撃batch

### 一次で確認できた範囲

公開clientの `glk`（13033–13037行）はhitRateが正ならfalseを返し、`ip`（9523–9530行）の通常攻撃の対象自動選択へ入らない。標準選択ではtarget無しになる。一方、click用 `aT` とsubmitの `targetPlayerId` 送信一般経路にはこの禁止が無く、「全敵への展開」や「明示targetをserverがどう扱うか」は公開clientだけで確定しない。v6のtarget無し限定はscopeの入力規約として定める。

`hit`（10930–10955行）と `miss`（10956–10963行）は受信済みplayerIdへ表示を向ける。hitは応答演出へ進み、missは外れの演出だけ。`nextAttack`（10881–10888行）は受信attackをdecodeして表示する。対象列の生成・乱数抽選・確率端点・途中死亡・補充順はここに無い。reflect/bounceでは元aggregateのhitRate表示値を保つが、サーバーの再命中抽選の有無を独立に検証したものではない。

### 明示するローカル実験規約

確率playはtarget無しのみ受理する。launch時点でaliveかつ敵のプレイヤーを入力seat順に固定rosterへ記録する。自己と同じ非Soloチームは除外し、Solo同士は敵とする。以後は各対象の処理時にdeadならskipして乱数を引かない。生存対象ごとに環境RNGの有界100抽選を1回行い、`roll < hit_rate` ならhit、そうでなければmiss。missには応答を作らず、hitにだけ元hitRateを持つresponseを作る。

redirectはその命中済みpayloadを移し、反射・跳ね返し後に再命中抽選しない。属性消去後の無属性chance武器は現在のcategory/属性により21/38/233へmatchingし、旧版の「％武器は反射不可」を持ち込まない。攻撃の物理消費・MC費用/used保持・原発分の補充予約はbatchのlaunchで一度だけ処理し、各対象やredirectで再課金しない。

各受理コマンド末尾で意思決定数を1増やし、自然終局→意思決定cutoffを優先する。自動miss/skipは意思決定数を増やさず、独立したcutoff境界にもならない。launchコマンドは最初のhitまたは全miss/skipまで自動処理をdrainしてから、この判定を行う。responseのコマンドを完了した場合は、未終局・未cutoffのときだけ次の対象の抽選へ進む。元ownerが途中で死亡したら初期chance対象の残りを取消す。ただし既に予約したcounterは、それぞれのsource/targetが生存していれば処理を続ける。batchの途中では補充せず、全対象の処理/取消を完了して未終局・未cutoffの場合だけ生存者へまとめて補充し、元ownerから次手番へ進む。

たとえば `max_decisions=1` のlaunchでも、最初の対象がmissなら2人目以降のrollが発生し得る。最初のhitへ到達しても応答前にcutoffし、全missの場合も同じコマンド末尾でcutoffする。打切り時にはqueue・journal・補充予約を取消し、既消費/MP支払と実行済み乱数更新を残す。応答は実行していないので、そのlaunchによるHP被害は無い。

これらの全敵展開・固定順・命中分布/乱数消費・途中skip/取消・死亡/補充scheduleは**公式server未検証の実験規約**である。公開top-level `chance_rule` に `experimental:targetless-all-alive-enemies-in-input-order;darkcloud-forces-without-draw;roll-before-response;redirects-no-reroll;skip-dead;cancel-on-origin-death` を明示する。既存 `bounce_target_rule` の未検証表示も維持する。

### v6当時のbatchとv7への統一

v6の `ChanceBatch` は確率playのlaunch前state/RNG/counters/commandとresponse journalを保存していた。v7では全pending playを後述の `ActionBatch` (`.batch`) へ統一し、非再帰のlaunch前状態と受理response journalを特権snapshotへ保存する。`restore` は同じ内部executorでcommand/journalを再実行し、HP/dead・MP・補充予定・RNG・現在frameを含む全結果を照合する。各対象が終わるとalive集合が変わるため、単一frame用のalive不変のredirect再生だけでは十分ではない。

v6のresponse履歴は `n*(hand_limit+99/5)+targets.size()` 以下だったが、v7はcounter FIFOも含めて後述の上限へ拡張する。物理/FREEと有償234の予算に加え、対象ごとの空防御応答を許す。`chance_rolls` / `chance_hits` はnative snapshotの再現検査用カウンターで公開観測へ出さない。将来の対象roster/cursor・launch anchor・response journal・未来RNGも出さず、現在pendingの元 `hit_rate` のみを公開する。

## v7の攻撃反撃と全行動journal

### 原典の確定範囲

JA/EN台帳の181は「攻{ダメージ}」/75%、184は「攻{ダメージ×2}」。いずれもarmorで、公開 `fv`（12115–12143行）は受信attack.atkを動的ATKとして表示する。`gf3`（12293–12295行）はこの動的ATKが正ならincoming_is_attackと判定する。`counterAttack`（11603–11616行）は指定index品を点滅させるだけで、counterの生成条件・量・FIFOや死亡順はここに無い。「ダメージ」がraw残余か実HP損失か、0被害や死亡時の発動、chance抽選順は公式serverと独立照合していない。

counterのcategoryをweaponsへ変える処理も無い。通常属性防御/filterは正の動的ATKへ適用でき、特殊反応では190のreflectAnythingだけがmatchingする。武器/奇跡用の反射・bounce・遮断はarmorへmatchingしない。この区別は公開 `e1/ev` のcategory判定による。

### 明示する実験規約

通常の被害解決で実HP減少が正となり、かつ生存した応答者だけがcounterを作る。その防御列で選択した181/184を選択順に各1個、FIFO末尾へ予約する。181は実HP減少×1・火・75%、184は×2・土・必中。完全防御0被害・miss・遮断・反射で通常被害を受けない場合や、その被害で死亡した場合は生成しない。

HPは0〜99なので生成powerは保守的にも198以下であり、raw威力の反復×2による指数的拡大を採らない。整数演算はint64で十分な範囲に収まる。これは旧版のraw残余counterを公式の確定仕様として流用しない、明示したローカル規約である。

counterのoriginは生成者で固定し、現在source/targetはredirectで変わり得る。turn ownerは原発の主手番ownerのまま。新counterは生成者から、その被害を与えた現在source一人をtargetにする。181を初期全敵chanceとして展開しない。開始時にsourceまたはtargetがdeadならskipし、乱数を引かない。生存していれば181は有界100抽選の `roll < 75` を1回行い、hitだけresponse、missは応答無し。184は必中。redirect後の再命中抽選は無い。

counter FIFOは次の初期chance対象より先に処理し、counterへの防御から生じたchildcounterもFIFO末尾へ加える。command末尾の自然終局→cutoffを優先する。元owner死亡時は残りの初期chance対象を取消すが、既予約counterは各source/targetが生存していれば続ける。全actionの効果/予約を完了して未終局・未cutoffの場合だけaliveへまとめて補充し、元ownerから巡回する。全体の順序、発動条件、実HP基準、死者skip等は**公式server未検証の実験規約**である。

公開top-levelの `counter_rule` は `experimental:actual-hp-loss;survivors-only;selection-order-fifo-before-next-target;skip-dead-source-or-target;single-target;darkcloud-forces-without-draw;roll-before-response;redirects-no-reroll`。既存のchance_rule・bounce_target_ruleも維持する。

### ActionBatchとrestore

通常playも含む全pending actionを非再帰の `ActionBatch` (`MatchSnapshot.batch`) へ統一する。launch前state・RNG・counters・原発commandと受理response journalから同じ内部executorを完全再実行し、現在HP/dead・MP・補充・乱数・現在frame・counter FIFOを比較する。counterは `.counters` FIFOへ保存し、将来FIFO/journalは観測へ出さない。単一frameのalive不変再生や、chanceのみの旧ChanceBatchでは検証しきれない履歴を扱う。

`ActionBatch.counters_created` と予約中counter数はそれぞれ `n*hand_limit` 以下を維持する。v7のresponse上限 `n*(2*hand_limit+99/5)+targets.size()` はMP回復を含まないため、v8では後述の上限へ置き換える。物理品の消費予算と有償234のMP予算に加え、counterと初期対象ごとの空応答を認め、全seatの消費・MP・補充を厳密に会計する。native snapshotの `counter_created` / `counter_started` / `counter_rolls` / `counter_hits` は再現・集計用で、公開viewへ出さない。

## v8の資源反撃とMP予算

### 一次で確認できる算術と不明点

原典 `fv`（12115–12143行）は187にserverのattack.mp、188にattack.cpを表示する。`gf3/e1/ev` では両者はpower0のarmor nonattackで、190万能反射だけが適合する。普通DEF・属性filter・他counter品・武器/奇跡専用反応は選べない。guardian metadataがneptune/venusでも、この2品のabilityはsetGuardianではない。

`boostMP`（11303行付近）は現在targetへMPを加え、上限99にする。187を誰に向けるか、counterの量が実HP減少に基づくかはserver側から未確認である。`takeCP`（11341–11342行）は**現在targetへ名目額を先に請求し、現在sourceへ同じ名目額を加算する**。請求不足はCP→MP→HPへ波及、加算は99でclipし、実所持CPや実損失を上限にしない。この逐次算術は公開clientから確認できるが、counter188の対象設定と生成条件は別の未確定点である。

### 明示するローカル実験規約

v7同様、通常の被害による実HP減少が正で生存した防御者だけが、選択順に資源counterを予約する。187はamount=実HP減少×2（198以下）、power0・armor・NEで、生成者を自己targetにして明示responseを作る。passで現在targetのMPを99上限まで増やす。自己へ190で反射してもresponseを続け、最終解決で一度だけ増やす。自動自己解決や重複増加は採らない。

188はamount=実HP減少、power0・armor・NEで、被害を与えた直前のincoming.sourceをtargetにする。passでは現在targetへ名目CP全額を請求してから、現在counter.sourceへ同額をcreditする。190反射では現在source/targetが交換され、徴収先と受益者も交換される。counter originは生成者のまま。自己source==targetでも逐次処理し、creditで先に失ったHP/MPを回復しない。

例として自己targetのCP3/MP4/HP40へ名目10を適用すると、請求後CP0/MP0/HP37、credit後CP10/MP0/HP37となる。CP95へ名目10なら95→85→95であり、creditを先にcap99へしてから請求する結果89とは異なる。請求でHP0になっても、その後のCP加算で復活しない。

資源不足によるHP損失からは新counterを作らない（通常damage bodyだけが生成元）。通常のsurvivor条件・FIFO・source/target dead skip・command境界の自然終局→cutoff・全action後のalive補充はv7を維持する。trigger・量・対象・自己response・debtからcounterを作らない条件は**公式server未検証の実験規約**である。

公開 `counter_rule` のtarget説明を `single-source-target` から `single-target` へ更新し、別の `resource_counter_rule` に `experimental:mp-self-target-with-response;cp-target-incoming-source;debit-current-target-then-full-credit-current-source;resource-debt-no-new-counter` を明示する。CP primitiveの順序/不足連鎖と、このcounter生成・対象規約を同じ確定度で扱わない。

### 回復MPを含むresponse上限

MP187でMPが回復するため、旧 `floor(99/5)=19` 回/人の有償234上限は成立しない。初期MP総量は99*n以下、各187の実MP増分は最大99、counter生成数はn*hand_limit以下なので、全有償234の回数は保守的に `floor(99*n*(hand_limit+1)/5)` 以下となる。

response履歴の上限を `2*n*hand_limit + floor(99*n*(hand_limit+1)/5) + targets.size()` へ更新する。n=9/hand_limit=18・targets数を最大9とした上限は3,718。counter生成数/予約数は従来のn*hand_limit以下を維持し、固定32へ制限しない。ActionBatchの同一executorによる完全replayでMP増減・資源debt・credit・FIFO・乱数・giftまで検査し、将来journalやprivileged予算を公開viewへ出さない。

## v9のHP吸収と同被害

### 一次資料と参考Flashの境界

公式JA/ENの説明はabsorbHPが「HPを吸収する」/“Absorb HP”、dealSameDamageが「使用者に同じダメージ」/“Get the same damage”。公開 `dealDamage`（11247–11260行）はevent.playerId（0なら現在target）へevent.damageを減算し、`boostHP`（11275–11289行）も同じtarget指定規約でevent.hpを加算する。HP0〜99clipとdieが別イベントであることは確認できるが、吸収/同被害の算式・source先行・Dark除外・死者確定順を計算するclient分岐ではない。

参考の旧Flashエミュレーターでは `turn.inflictDamage` が正残余を条件に、Dark即死の別分岐以外でsource吸収回復→source同被害→target被害を適用する。[旧吸収調査](legacy-hp-absorption-rules.md)・[旧同被害調査](legacy-self-damage-rules.md)を参照する。v9はこの順序を参考に**実験規約**として採り、現行公式serverの確定ルールとは呼ばない。

### ローカルの通常damage body

普通DEF後の正のnonDark残余rawについて、absorbHPは現在sourceへraw全量を先に回復（HP99cap）し、その後現在targetへraw被害を適用する。dealSameDamageは現在sourceへraw被害を先に適用し、その後現在targetへraw被害を適用する。targetの残HP・実HP損失や、sourceの実回復/実損失を基準にrawを縮めない。

自己source==targetでも各段階を逐次clipする。吸収HP95/raw12なら99→87となり、自己を無効果へ相殺しない。同被害HP40/raw14なら40→26→12、HP10/raw14なら10→0→0。別人でsourceHP2/raw14でもsource0になった後にtargetへ14を適用する。source/targetのdead確定は**全bodyの後**で、段階間に死亡・補充・次counterを挟まない。

counterの基準はtargetdamage段階で減った実HPだけで、source側の吸収/同被害や自己全体のnet変化を使わない。生存条件も全body後に判定する。自己吸収でnet変化が小さくても、target段階の実損失は別に数える。派生counterは新しい反撃payloadで、元のabsorbHP/dealSameDamage能力を継承しない。

miss・block・redirect時点・残余0では副作用を適用しない。Dark bodyは対象HP0の別処理で副作用を通さない。ただし現scopeでは27/45/53はNE基底、98は木chanceで追加不可、216は光MCでFREEのみ、setterは火/水なので、合法なDark吸収/53は到達しない。防御filterは色を除去するだけで、Dark化する例を捏造しない。

反射/跳ね返しは元能力・量を維持し、最終解決時の現在source/targetへ上記順を適用する。通常の副作用でMPは増えず、新RNGも加えない。v8のMP込みresponse予算と全ActionBatch replayを維持する。

ベンチの `absorb_positive_resolutions` / `self_damage_positive_resolutions` は正nonDark残余で副作用を最終解決した件数、`absorb_nominal_hp` / `self_damage_nominal_hp` は普通DEF後のraw残余全量の合計である。miss/block/redirect時点/残余0は含めず、実HP回復量やsource/targetのclip実損失とは区別する。

公開 `damage_effect_rule` は `experimental:positive-nondark-remainder;current-source-side-effect-before-target-damage;counter-basis-target-damage-only`。HP primitive以外の量・順序・counter基準と死亡接合は現行server未検証である。v9では状態を含めず、閃光は次節のv10で明示して追加する。

## v10の閃光と解除

v10は70フラッシュダガー（光ATK2/価格15/重み1）、185天王の指輪（光armor/ATKDEF0/価格10/重み1）、224＜閃光＞（光ATK1/25%/MP3/重み1）、199スマイルの貝がら（mild解除/価格5/重み12）、200ハートの貝がら（全解除/価格15/重み6）、237＜音色＞（mild解除/MP2/重み1）、238＜歌声＞（全解除/MP5/重み1）を加える。合計201モデル/重み404、weapons94/armor73/sundries11/miracles23。

公開JAの閃光説明とclient `ev`（9478–9481行）は、防御時に2枚目を拒否する。FREE例外より先に検査するため、応答の奇跡+FREEも不可。主手番の複数使用・無料化にはこの制限を付けない。`dA`（13249行付近）は既存の非病気curse追加を重複させない。`removeMildCurses` の説明は閃光を解除対象に含め、`removeCurses`（11425行付近）はサーバーから受け取った解除リストを削除する。これらのclient事実と、以下のserver未確認の遷移規約を分ける。

70/224は対象へのdamage段階の実HP損失が正なら閃光を付ける。致死被害でも付与し、dead状態に残す。0被害・miss・block・redirect途中では付けない。再付与は1個のまま、通常被害・次手番・時間では自動解除しない。閃光の自然tickやRNGは導入しない。**実HP基準・致死時・持続条件は実験規約**で、現行serverの同等性を主張しない。

185は従来と同じ実被害>0かつ生存条件で、選択順FIFOへ1反撃を予約する。現在incoming.sourceへ向かう光/armor/power0/amount0の非攻撃payloadであり、通常DEF/filter/武器用反応/奇跡用反応は適合せず、190万能反射だけが適合する。反射後の現在targetへ閃光を付け、状態付与から新反撃は作らない。FIFOの先の185を解決した直後から、後続counterへの応答も1枚に制限する。source/target死のskip、自然終局優先、全action後補充は従来通り。

199/200/237/238は現在targetの閃光を解除する。v10当時の受理curseは閃光だけだったが、v11は後述のとおりmildとallで暗雲への効果を分ける。病気や霧/夢まで実行できるという意味にはしない。解除対象が無くても通常の消費・MP費用を適用する。自己解除は既存の自己非攻撃効果と同じ即時解決、他人への解除は明示responseを作り、適合する反射/奇跡遮断を待つ。237/238の保持再使用と主手番FREEを扱う。

v10のraw sceneは各人のcurseが空または閃光1個に限った。v11は次節の閃光/暗雲のcanonical集合へ広げ、他の病気・霧・夢、fake、guardian、confusion等は引き続き拒否する。公開boardのcurseと合法手・samplerが同じ閃光状態を使う。Snapshot10/全action replayが状態と選択制限まで照合し、旧1〜9を拒否する。C ABI1は維持、公開schemaはview-v8、`flash_rule` は `experimental:positive-target-hp-loss-including-lethal;idempotent-until-explicit-cure;counter-to-incoming-source;one-card-response;no-tick`。MP回復予算と物理counter数の上限はv8のままで、新しい特権情報を観測へ出さない。

benchの `flash_added` / `flash_removed` は各プレイヤーの有無が変化した回数で、再付与や空解除を数えない。致死付与はaddedへ含む。`flash_response_commands` は受理直前に閃光を持っていた応答者のコマンド数で、passも含む。いずれもベンチ内部集計で、方策のobserveへは加えない。

## v11の暗雲と無抽選必中

### 公開clientの確定範囲

JA説明は「全体攻撃を受けると必ず命中する」、ENは “Received %ATK attacks hit certainly”。WIDE能力だけに限定する説明ではない。`hit`（10930–10955行）は、受信済みhit対象の暗雲を参照して不可避演出へ分岐する（閲覧者の霧による秘匿条件も判定）。`nextAttack` は受信attackを表示するだけ。候補を処理する時点の判定、乱数を省くか、redirect後の再判定は公開clientから確定しない。

非病気curseの `dA`（13249行付近）は既存再付与をreturnして重複させず、HP更新やdieに暗雲を自動解除するclient処理は無い。mild解除のJA/EN説明は風邪/熱病/霧/閃光だけ、allは全curse。`removeCurses` はserverから受け取ったリストを削除するため、公開説明の範囲とserverの発行順を区別する。

### ローカルの付与・状態・命中規約

41は70/224と同じくtarget段階の正実HP損失時に暗雲を付け、致死時もdead状態へ残す。186は従来counterと同じ実被害>0かつ生存条件でFIFO生成し、旧incoming.sourceへ向かうarmor/闇/power0のnonattackとして現在targetへ暗雲を付ける。普通DEF/filter/武器・奇跡用反応は不適合で190だけが適合する。223は通常のaddCurse奇跡としてMP5/used保持・再使用/FREEを扱い、自己なら即時、他者なら明示responseを作る。反射/bounce/奇跡遮断のcategory条件は既存通りで、闇属性でもHP0効果にはしない。

FlashとDarkCloudは同居可能で、raw stateは閃光→暗雲のcanonical順・重複無しだけを受理する。再付与はidempotent、mild199/237は閃光だけを除去して暗雲を残し、all200/238は両方を除去する。他の災いは引き続き拒否する。暗雲は通常被害・次手番・時間で自動解除せず、all解除まで持続し、tickも追加しない。これらのtrigger・致死時・持続・解除順はserver未確認の実験規約である。

初期percent対象と181counterの**活性化時**、対象が暗雲なら環境RNGを引かずforced hitにする。dead skipは従来通り無draw。元のhit_rateは25/50/75等のまま保ち、100へ上書きせず、公開pending.forced_hit=trueとする。redirect後は暗雲の有無を再確認せず、命中済みpayloadをそのまま応答し続ける。開始後に別targetへ向かっても、新しい命中抽選やforced判定は行わない。

無drawのskip/forceも既存chance scheduleに従う。初期playは最初のhit（forcedを含む）または全miss/skipまでdrainしてからdecision/cutoffを判定し、後続counter/対象は完了responseの自然終局/cutoff後だけ活性化する。earlier FIFOで付与/解除された暗雲は、その後の対象活性化時に参照する。

公開 `darkcloud_rule` は `experimental:force-percent-hit-without-draw-at-target-activation;redirects-no-recheck;idempotent-until-all-cure;no-tick`。chance_ruleとcounter_ruleにも `darkcloud-forces-without-draw` を加える。ENの％攻撃必中説明とは整合するが、181を含む適用時点・無draw・持続等が公式serverと等価と確認されたわけではない。

### 再現・集計と観測

Snapshot11のActionBatch anchorとMatchSnapshotは `chance_forced_hits` / `counter_forced_hits` を保持し、同execute完全replayでforcedによるRNG非消費・状態・現在frameまで照合する。`EffectFrame.forced_hit` は現在の命中済みpayloadの来歴である。

`chance_hits` / `counter_hits` は実drawに成功した件数だけを数え、暗雲forcedを混ぜない。missは従来どおりrolls−hits、forced hitは別カウンターで記録する。 `darkcloud_added` / `darkcloud_removed` は有無の遷移だけで、再付与や空解除は数えない。`chance_forced_hits` / `counter_forced_hits` は無drawで活性化した命中保証で、被害を最終解決した件数ではない。launch直後cutoffなら記録だけ残り、応答未実行の被害は無い。公開viewへ累計forcedカウンターや将来の必中結果を出さず、現在pendingのforced_hitとboardの現在状態だけを返す。新しい乱数を加える効果ではなく、該当rollを省く実験規約であり、MP/gift/journal予算は維持する。

## v12の未使用手札・使用済み奇跡の除去

### 一次で確認できた対象と表示

公式JA/ENの説明は205が神器3個、206が起こした奇跡2個。`usedMiraclesNote` は使用済み奇跡を売買と夜空のホウキの対象外と明記する。したがって205はusedでない手札（未使用奇跡を含む）、206はusedな奇跡コピーを対象とする。ただし抽選分布・順序・empty/singletonのRNGはここに書かれていない。

JSのremoveItems（11495–11517行）とremoveUsedMiracles（11518–11544行）はserverが指定したevent.itemsを `ds/kr`（11801行付近）へ渡し、受信順で除去品を表示する。`kY` がdecodeするid/modelId/fakeModelId/usedのうち、instance idは本人手札の消去に使い、カードは他者の対象でも表示する。fake時は `cA/bZ` による真モデルのflip表示もある。206はusedモデル欄の同モデルコピーを1個ずつ除去する。候補母集団・抽選slotや選定乱数を計算するhandlerではない。

v12〜18ではこの除去表示をMatch.observeへ投影していなかった。v19は次節のlast_removalで直近の解決済み除去モデルだけを公開し、このgapを解消する。全イベント履歴や非除去手札の投影ではなく、token/slot・候補母数・RNGは引き続き非公開にする。

### 明示するローカル除去規約

205は現在targetのunused手札から最大3個、206は現在targetのused奇跡から最大2個を取り除く。同じmodelの別tokenコピーは別候補であり、1コピーずつ数える。毎回current inventory順にeligibleを再構築し、環境RNGの有界一様抽選で1個を選んでstable eraseする。これを上限まで繰り返すwithout replacementで、未選択コピーの相対順を保つ。

eligible0ならそこで終了して除去抽選を引かず、eligible1でもbounded drawを1回消費する。自己使用は205/206自身を物理消費してからeligibleを作る。自己効果は即時、他者へは明示responseを作り、sundry/NE/power0のnonattackなので190万能反射だけが適合する。反射後は現在targetから選び、原効果を再消費しない。

公開pending.amountの3/2は除去**上限**であり、実除去数ではない。実数は最終body時点のeligible数に依存して0〜上限となる。除去自体のgiftは0で、既に予約した使用品や190のgiftを取消したり増やしたりしない。empty-no-drawは除去本体の抽選についての規約であり、通常完了時の既存gift RNGまで0になる意味ではない。

206は起こせる奇跡のretained tokenを消すだけで、既に付与された暗雲等を解除しない。223を失ってもDarkCloudは残る。過去のMP費用・used使用・原発者の消費も巻き戻さない。状態解除は別のcure効果による。

公開 `removal_rule` は `experimental:unused-hand-max3;used-miracles-max2;uniform-without-replacement-in-inventory-order;bounded-draw-per-removal;empty-no-draw;no-removal-gifts`。除去数とused対象外の説明を除き、この抽選・自己順・gift scheduleは公式server未確認の実験規約である。counter/MP予算は変更しない。

### atomic bodyとsnapshot証明の範囲

除去は最終bodyの一つのstep内でatomicに完了し、1個目だけを除去した中間snapshotを公開しない。pending journalのrestoreはpre-removalの消費/反射/RNGと派生payload上限を再生して整合を検査する。完了後main/finishedの任意raw stateが公式サーバーの歴史から到達したことまで証明しない。privileged debug snapshotの既存境界を広げず、deterministic replayではbody抽選と以後のgiftの同一性を検査する。

ベンチの `hand_removal_resolved` / `used_removal_resolved` はeligible0も含む最終body解決数、`hand_items_removed` / `used_miracles_removed` は実token消失数である。205/206使用そのもののconsumeを実除去へ含めず、補充の新tokenで相殺しない。同model2コピーなら2個で、手札枚数のnet差とは異なる。これらの内部集計も公開observeへ追加しない。

## 死亡・次手番・終局・報酬

効果を解決してから、このsubsetが定義する順に脱落を確定する。v7の全action batchは前述の受理コマンド境界で自然終局/cutoffとcounter FIFO・残り対象を判定し、補充・次ownerは全action完了後へ遅延する。それ以外は1対象の完了後に補充・次ownerへ進む。反射/跳ね返しを経ても次ownerの巡回起点は元の主手番ownerのまま。deadの人は以後の主手番や生存対象から除く。v22のowner病気tickと既存208救済は明示した境界だけへ接続し、死に際攻撃・守護神等を推測で挿入しない。HP0→脱落→次手番の具体的scheduleはサーバー未検証の実験規約である。

生存者が0人ならdraw、1人ならその人が勝者になる。複数生存者が全員同じ非Soloチームならチーム終局とする。勝利チームのwinnerリストには、先にdeadになった同チームの人も含む。後者はクライアントのending表示 `h7.lV` の集合規則を参考にするが、それ自体をサーバーの勝敗判定器とみなさない。

報酬はactor相対ではなく、入力プレイヤー列に固定したベクトル。winnerへ+1、それ以外へ−1、drawなら全員0。意思決定上限によるtruncationは自然終局と別フラグで、報酬は全員0とする。512という既定上限は公式ゲームの終了条件ではない。

意思決定はplay/defend/discard/pray/exchangeの各受理コマンドで1ずつ数える。自然終局を先に判定し、未終局で上限へ達した場合にtruncationとする。応答待ちを作ったplayがその上限なら、待機effectと補充予定を取消して打ち切るため、未実行の被害や補充を後から適用しない。消費・MP支払いが既に行われた時点の状態は残す。完了済みの行動でも、打切りが先に成立した場合は補充・次ownerへ進めない。

## v13のMP武器と二回攻撃

### 一次資料で確認できる部分

現行58は旧Flashの57 MAGICALとID・計算を混同しない。JS preview `dt`（9544–9557行）は完成列費用 `la` を自己MPから引いた非負残額の2倍を `aR` の基底overrideへ渡す。`dA.iJ`→`tw`（12280行付近/3971行付近）が追加boost/Auraを順適用する。useAttackItems（10736–10749行）も完成列費用を減算してから残MP×2を `bF/kn` で再表示し、MP表示を0にする。FREE extraは威力/攻撃属性からskipするが、隣接MCの費用を免除する。

MP10で58→boost3→有料Aura6なら基底8から22、Aura→boost3なら19。58→Aura→FREEなら基底20から40。MP0でも `gf3` はweapon categoryをattackと判定し、0power responseを作れる。現行方式は旧Flashの各有料追加MC後の途中威力resetではない。setterは通常のATK加算/属性上書き、MCを追加してもweapon categoryを保つ。

14はJA/ENの「二回攻撃する」/“Attack twice” が根拠。公開nextAttackは受信attackを表示するhandlerであり、二発の消費・防御・死亡・補充scheduleを生成しない。固定旧Flashのturn/AttackData/全364行にも同対象2hitのrepeat処理は無く、DOUBLE_ATKは威力倍化なので二回攻撃の根拠にならない。

### 明示する実験規約

58は完成列MC費用差引後の残MP×2を基底overrideとしてから、順序付きboost/Auraを適用する。launchで元MP全額を一度だけ払い（MC費用と残MP burnの合計）、MP0も合法。原発MPはbatch.initial_stateのownerMPへ固定し、redirectや後続counter MP回復、現在sourceの変化によって再計算・再課金しない。元payloadのcategory/属性と既存反応規則は維持する。

14単体は明示指定targetへ `targets={target,target}` として二つのfresh payloadを作る。モデル列・元威力・属性は各hitとも原発から構築し、hit1のfilter/redirectはhit2へ引き継がない。消費/MC支払い/原発の補充予約は一度だけ。各hitは独立responseを持ち、6damageの一括処理へ置換しない。counter FIFOを次hitより先に進め、dead targetならskip、origin死亡なら未開始hitを取消す一方、既予約の生存counterは従来どおり続ける。自然終局→受理command末尾cutoffを優先し、全actionが未終局・未打切りで完了した後だけaliveへ補充して次ownerへ進む。

公開 `mana_weapon_rule` は `experimental:base-2x-mp-after-composed-miracle-cost;ordered-boosts;consume-all-mp-once-at-launch;redirects-no-repayment`、`repeat_attack_rule` は `experimental:two-fresh-payloads-to-same-target;single-consumption-and-payment;counter-fifo-before-next-hit;dead-target-skip;origin-death-cancels-rest;gifts-after-whole-action`。二回という説明とクライアント算術以外のscheduleは公式server未検証の実験規約である。新RNGとMP増加源は無く、既存response boundのtargets.size()に14の2件を含めて全journal replayを行う。2件は同じtargetでも別の初期frameであり、rosterを集合としてdedupしない。

ベンチの `mana_weapon_plays` は58の主動作数、`mana_mp_spent` はMC費用込みの全支払いMP、`mana_base_mp` は完成列MC費用後の残MP合計。`repeat_weapon_plays` は14の主動作数、`repeat_frames_completed` は各hitがredirect以外で終端応答を完了した数で、block/完全防御0も含む。未完了cutoffや次hit前のcounter死亡等では2未満になり、被害hit数とは異なる。`self_attacks` は自己宛攻撃の主動作数なので14でも1、ATK0の58もweaponとして含む。新しいpublic累計counterや特権schema fieldは追加しない。

## v14の全敵化と二巡攻撃

### 一次資料で確認できる部分

232＜蜃気楼＞/MirageはJA/ENで全敵への攻撃と説明される。JSONはmiracles/isPlusAtk/attackEveryEnemy/cost5/giftRate1、欠損するatk/hitRate/element/priceは0/0/NE/0。`gkN`（13024行付近）はATK0のisPlusAtk奇跡の単独開始を拒否し、`ev`（9478行付近）は基底がweaponかつhitRate0の追加を許す。他の奇跡基底/確率武器には追加しない。

`tw/l9/la`（3971–4007行）は232を通常ATK+0/NE合成として扱う。有色→232でNE、232→後続setterなら再着色する。各232の直後FREEだけがその費用を免除し、先頭FREEが後続232を自動無料化しない。別copyごとにMP/保持を数え、同commandで同tokenを重複選択する契約は追加しない。

`ip`（9520行付近）の `glk` による対象自動選択は先頭1品時だけで、232を追加してもtargetを消す分岐は無い。submit（9639行付近）はtargetがあればtargetPlayerIdを送信する。受信setTargetPlayer（10910行付近）の `A.kr` は232の存在を調べ、霧の演出分岐から除く。候補列やserver抽選を生成する関数ではない。alive敵の表示集合を作る `dR/ce` の存在も、serverでの順序・対象入力受理の証明にはしない。

### 明示する実験規約

232を含む完成weapon列はtarget無し、または既知aliveの任意target指定を受理する。指定は表示用anchorとして扱い、展開roster/順序を変えない。自己/味方を指定しても、その人を初期rosterへ追加しない。unknown/dead targetは拒否し、samplerはcanonicalなtarget無しを出す。この受理範囲をserver検証済みとは主張しない。

launch時alive敵を入力seat順に一度固定し、自己・同非Soloチームを除く。hitRate0の保証攻撃として各生存targetへresponseを作り、chance draw/暗雲forced累計を増やさない。旧Flash WIDEの100%化・毎対象の命中drawを流用しない。14が無ければrosterを一巡、14+232なら全roster二巡（A,B,A,B）とし、A,A,B,Bへ入れ替えない。複数232は対象拡大について冪等で、三巡目等を作らない。

原発消費・MC費用/used保持・補充予約は一度だけ。232各copyの費用/保持は別に処理し、直後FREEのみ免除する。58は完成列費用後のlaunch MP×2をimmutable基底にして全payloadへ固定し、後続MP回復や現在sourceから再計算しない。各frameは原発からfreshに作り、前frameのfilter/redirectを持ち越さない。

counter FIFOを次targetより先に処理し、dead targetは無draw skip、元owner死亡で未開始の初期rosterを取消す。既予約の生存counterは継続する。受理command末尾の自然終局→cutoffを優先し、全action完了後に未終局・未打切りならaliveへまとめてgiftして次ownerへ進む。展開の順・2pass・無draw・途中死亡/補充は**公式server未検証の実験規約**である。

公開 `wide_attack_rule` は `experimental:initial-alive-enemies-in-input-order;optional-alive-target-does-not-alter-roster;repeat-by-full-roster-pass;single-payment;counter-fifo-before-next-target;dead-target-skip;origin-death-cancels-rest;gifts-after-whole-action`。`repeat_attack_rule` は `experimental:two-fresh-passes-over-initial-targets;single-consumption-and-payment;counter-fifo-before-next-hit;dead-target-skip;origin-death-cancels-rest;gifts-after-whole-action` へ更新し、14単体の同target2hitを保つ。最大targetsは2*(n−1)=16、response boundの `+targets.size()` を含め最大3,725となる。3,718はv8〜13時点の上限であり、14+232へ流用しない。Snapshot14は旧1〜13拒否/view-v12/C ABI1で、全ActionBatch再生がroster二巡・payload・HP/MP/used/gift/RNG・counter FIFOを検査する。新たな未来roster/cursor/anchor/journalや特権counterは公開observeへ出さない。

## v15の使用時HP増減

### 一次資料と未確定の情報公開

202ドキドキ涙/Thump-thump Tearはsundries・abilityValue10・価格1/giftRate2、属性欠損はNE、ATK/DEF/MP/率0。JS `a4`（9376行付近）はboostHPとorDealDamageを組み合わせ、HP+10またはdamage10と表示する。50/50は説明に無く、同abilityを処理する抽選分岐も公開JSには見つからない。

`p3`（3653行付近）のattackはatk/hitRate/mp/cp等をdecodeするが202固有のsigned HP結果欄は無い。`boostHP/dealDamage`（11247行付近）handlerは受信イベントを現在targetまたは指定playerへ適用し、`eW` はHP0〜99にclipする。汎用attack.atk等が動的結果を運ぶ可能性も排除できないため、符号が応答前に公開される/されない、negativeが防御不可というserver規則の証拠とはしない。`eW` 自体はdeadにせず、`die` は別の受信eventである。

旧Flash237はlaunchの各50%抽選→COMMAND.hp公開→cloneで反射時維持→最終targetへ一度適用という参考実装を持つ。ただし旧bit対応は0damage/1healで、現行の一次根拠ではない。旧None属性/先頭196例外・244蘇生・病気tickの順を現行へ流用しない。

### 明示する実験規約

launchで環境RNGのbounded2を一度だけ引き、0なら+10、1なら−10を固定する。powerは0、amountは符号付き±10で、他者へのresponseを待つ時点から公開pendingへ出す。selfは即時解決、他者は明示response、190反射だけが適合する。反射はcurrent source/targetを交換するが符号は変えず再抽選しない。負でもnonattackのままで普通DEF/filter/counter・型別反応は使わず、追加boost/FREE/232等も併用しない。

最終targetへ一度だけHPを加減し、0〜99へclipする。HP95の+10は実増4、HP99の+10は実増0でもheal解決、HP5の−10は実損失5。通常attack被害としてFlash等の付与やcounter予約を生成しない。死flagはbody後に確定し、自然終局→command cutoffを優先、全action完了後に未終局・未打切りの場合だけaliveへ既予約giftを配る。物理202の消費/gift予約は原発一度、190の消費/予約は各応答どおりで、効果自身は追加giftを作らない。v15時点では現行蘇生を追加せず、v16で後述の自動救済を加える。HP0なのにdead=falseの不安定raw sceneを受理する契約は追加しない。

公開 `random_hp_rule` は `experimental:launch-once;uniform-plus10-or-minus10;zero-heals-one-damages;fixed-through-reflections;nonattack-no-defense-or-counters;hp-clips-0-99;gifts-after-action`。50/50・符号draw時点/前公開・固定・negative防御不可・死亡/補充順は公式server未検証の**実験規約**である。chance命中抽選ではなくDarkCloudで必中化したりdrawを省いたりせず、chance/forced累計にも含めない。

Snapshot15は旧1〜14を拒否し、`ActionBatch.random_hp_delta` とlaunch draw前のRNG anchorから同executorで再生する。effect factoryは固定deltaを受け取る純粋計算で、反射時や現在HPから再抽選しない。signed現在値を公開しても、未来RNG・anchor・特権journalは公開しない。view-v13/C ABI1を使い、MP/反撃/response予算はv14を維持する。

ベンチの `random_hp_plays` は使用数、`random_hp_resolved` は最終body解決数、`random_hp_heals` / `random_hp_damages` はその完了した符号分類。HP99で実増0でもhealsへ数える。`random_hp_gained` / `random_hp_lost` は0/99clip後の実HP増減合計で、名目±10や使用数×10と分ける。redirect中や未解決cutoffはresolved/heals/damagesへ含めない。

## v16の自動復活と満杯時の祈り

### 一次資料と旧版との差

208太陽のお守り/Sun Amuletはsundries・revive・abilityValue10・価格10/giftRate1。公式JA/ENはHP0になるとHP+10、教典は手動discard禁止と説明する。`gkN/ev`（13024/9499行付近）もmain開始とdiscardを拒否し、ATK/DEF/追加役割が無いためresponseにも選べない。

revive handler（11617–11649行）はserver指定event.itemのinstance idを本人手札で選択し、`d_`→`ee(null)`（12743/12694行付近）へ通して物理消費と補充用空枠を作る。どのcopyかは受信時点で指定済みで、clientがinventory最初を決めるわけではない。HP加算やdead取消をこのhandlerは計算せず、boostHP/dieは別eventである。従って実gift数・HP0発火/走査順・not.dead人数gate・複数瀕死の決着は未確定とする。

[旧244の調査](legacy-revival-rules.md)にある直接discard/gift0・手動discard可・固定seat走査の非対称は参考資料であり、現行へ自動流用しない。現行死亡時攻撃は112昇天弓で、旧117と別。revive/die/attackDyinglyのdispatch分岐順をserver発行順とはみなさず、112をこのscopeへ追加しない。

### 明示する実験規約

全bodyのsource/targetへのHP・資源・状態処理を済ませた後、入力seat順にHP0かつnot.deadの人物を走査する。各人物の処理時点でnot.dead人数が2以上、かつ208を持つなら、inventory最初のcopyを一枚消費してHP+10、補充1を予約し、deadにしない。成立しなければdeadを確定する。既deadは救済しない。1境界で1人1copyだけであり、別のbody終了で再びHP0になれば次copyを使える。主手番owner順やHP>0人数へ置換しない。

例えば両者HP0でseat1だけ208なら、seat0を先にdead化してnot.deadが1となるためseat1も救われない。両者が208を持てばseat0→seat1とも救われる。これは選んだ実験規約の非対称で、現行server確認済みの同時死亡ルールではない。

消費は自動で、手動play/response/discard/FREEのActionは追加しない。HP以外のMP/CP/usedMC/curseは変えず、救済自体ではRNGを引かない（後のgift抽選とは別）。1giftは現行clientの補充用空枠を参考に選ぶ規約であり、server実giftの証明ではない。全action後だけaliveへまとめて配り、自然終局/cutoffなら予約を取消す。反射中は未解決bodyなので発火せず、現在targetへ最終効果が入ったbody後に処理する。

通常damageのcounter基準は復活前の被害段階の実HP損失を保つ。復活後にsurvivorなら、その損失額で選択counterを予約できる。復活healをnet差分に混ぜて被害を消したり、source側副作用や資源debtから新counterを作ったりしない。次のinitial target/counterや自然終局判定はこの救済後のdead集合を見る。

武器が無い場合の祈りは満杯でも受理する。公開clientの空選択submitと既存 `selection_ready` は手札枠の空きを必須にせず、武器の有無を見ている。満杯ではrequest_giftが0予約となり、配布RNGを引かず、意思決定と通常の手番/cutoffを進めるのはローカル規約である。祈り自身に復活や他の効果は付けない。

公開 `revival_rule` は `experimental:post-body-input-order;hp0-and-not-dead;two-not-dead-required;first-copy-once-per-boundary;hp-plus10;one-deferred-gift;counter-basis-pre-revival-loss;passive-and-undiscardable;full-hand-prayer-no-draw`。gate/先頭copy/走査順/1gift/死亡・counter・祈りのscheduleは**公式server未検証の実験規約**である。Snapshot16は旧1〜15拒否/view-v14/C ABI1、nativeの特権 `revivals` 累計とBatch anchorをreplayで照合するが、累計・token・anchorを公開observeへ出さない。MP増加源や途中giftは加えずresponse bound最大3,725を維持する。

ベンチの `revivals` は実行された自動救済の回数で、所持枚数・不成立のHP0判定回数とは異なる。`physical_used` は従来どおりplay/defendで選択使用した非MC数だけで、208の自動消費は含めず `revivals` に別計上する。`random_hp_resolved/heals/damages/gained/lost` は202自身のbody処理だけを集計し、復活HP+10を除く。negative202でHP0→復活10となりnet HPが正でもdamage分類/被害段階実損失のままにする。

## v17の杵と臼

### 一次資料と旧版との差

現行94はあぶないキネ/Dangerous Pestle、Light weapon/ATK30/danger/価格5/giftRate1。209はあぶないウス/Dangerous Mortar、sundries/attractDanger/abilityValue99/価格1/giftRate1。JA/ENは「誰かへの攻撃」と「キネから99damage」を説明する。旧99/245の999・価格0と混同しない。

`attractDanger`（10970–10999行）はserver指定event.itemをtarget側paneへ置き、本人手札ならinstanceをselect→`d_`→`ee(null)`で臼を消費して補充用空枠を作る。`fG`（12407行付近）のresponse選択はdefender paneが空の時だけなので、臼が置かれた表示はその分岐を避ける。ただしserverが防御commandを拒否すること、実gift数/順をこれだけで証明しない。

`ev`（9478行付近）は94にも普通boost/setter・210火の玉・217流星・231Aura・232Wide・MC直後FREEを許し、旧版の列長1を要求しない。`tw/l9/la` は通常合成で、94→火の玉はFire/32、94→AuraはNE/60、94→WideはNE/30となる。94自身はisPlusAtkが無いので他の武器のextraにはならない。`glk/ip` は先頭94を通常weaponとしてenemyを自動選択し、submitは明示targetを送り得る。dangerを霧演出から除く分岐は、対象候補/抽選分布を生成しない。

[旧杵・臼調査](legacy-pestle-rules.md)の999・臼非消費・単独限定・self応答省略を現行へ移さない。臼99を追加後も固定99とするか、基底overrideへ使うか、Wideとの優先関係、臼複数時の選択順はserver未確定である。

### 明示する実験規約

94の完成列はtarget無し/既知aliveの任意ヒントを受理し、ヒントは無視する。unknown/dead指定は拒否する。launch一度で、全aliveをinput seat順に走査し最初の209所持者/そのinventory最初copyを選ぶ。自己/味方も対象となる。holder無しなら自己/味方を含む全aliveから有界一様抽選を一度引く。臼があれば対象抽選を引かず、redirect後に再選択・再抽選しない。

臼ありは正の基底override99から通常のordered boost/Aura/属性/FREEを合成する。例えばAuraならNE/198、火の玉ならFire/101であり、最終値を99へ固定しない。選択した臼一copyを自動消費してgift1を予約し、0DEF・応答無しでbodyを即解決する。原発の使用/MC費用/used保持は通常どおり一度で、反応による臼再消費はしない。自動消費のgiftは全action後まで遅延し、terminal/cutoffなら取消す。

臼無しは基底30から同じ合成をして通常responseを作る。自己に向いてもresponseを待ち、普通DEF/filter/型別反応は現在属性・categoryに従う。反射/跳ね返しで元威力/属性/abilityを保ち、臼を新たに探さない。209は手動play/response/discardに使わず、205のunused強制除去には含める。臼ありbody後の復活208・死亡・自然終局/cutoffは既存規約へ接続する。

94+232だけは公開client predicateが許しても優先規則が未確定なので、runtimeで明示unsupported（-5）とする。他の対応boost/MC/FREEまで旧版同様の単独限定にはしない。read-only append/composeがtrueでも、このMatchの全組合せ対応を意味しない。

公開 `danger_rule` は `experimental:launch-once;first-alive-mortar-in-input-order;otherwise-uniform-all-alive;optional-alive-target-ignored;base99-before-ordered-boosts;consume-one-mortar-with-deferred-gift;attracted-body-no-response;redirects-no-reselection;wide-combination-unsupported`。対象分布/first holder・copy/99基底合成/自動消費とgift/応答省略は**公式server未検証の実験規約**である。Snapshot17は旧1〜16拒否/view-v15/C ABI1、MP源や途中giftは追加しない。

公開 `last_danger` がある場合、実行済launchのorigin/target/models/mortar/power/elementだけを返す。mortarは臼による誘引の有無、target/power/elementは初期選択結果を指す。response中のredirectで書き換えず、次のmain commandを受理したとき消す。不正commandで消去せず、terminalまで次mainが無ければ保持する。token/slot/RNG/未選択holderや将来の選択は返さない。これは限定した過去launchの要約であり、全イベント履歴の投影ではない。pending journalではlaunch/redirectとの整合を再生して照合するが、安定main/finishedの任意raw sceneの全過去履歴を公式到達可能と証明するものではない。

ベンチの `danger_plays` / `danger_mortar_plays` は94使用/臼誘引、`danger_self_targets` / `danger_ally_targets` はlaunch時の自己/味方選択の件数（味方は本人を除く同じ非Soloチーム、解決/被害件数ではない）。209の自動消費はplay/defend選択使用の `physical_used` へ含めず、臼誘引欄へ別計上する。

## v18の両替と型付き配分

### 一次資料で確認できる部分

現行3両替/Exchangeはtrade・価格5/giftRate20、説明はHP1=MP1=CP1の交換。旧2の価格10とは区別し、価格5を操作費用にしない。`tj`（3582行付近）と `fo/fp`（11864–11921行）はHP↔MP・HP↔CPを±1/±10の単位で動かし、各0〜99を守り総和を保存する。HP1から0まで移す操作も可能である。

submit（9635行付近）は新mp/cpだけをcommandへ送りhpを送らない。exchange event（11080–11100行）は受信hp/mp/cpをsource欄へ直接代入する。canonicalな3値と配分前総和を持てばwireのmp/cpへ一意に射影できるが、server側の導出/受理検査を完全取得したという意味ではない。単品tradeに `ev` は追加品を認めず、MP費用0、`ee` は物理消費と補充用空枠1を作る。targetを送る一般submit経路があっても、source再配分の表示から他者への配分を導かない。

### 明示する実験規約

新CommandKind4=`exchange` とし、本人所有のmodel3一tokenだけを使う。targetは無し、allocation=(hp,mp,cp)は各0〜99の整数、配分前HP+MP+CPと同じ総和にする。同値再配分も合法で、MP/CP feeは無く、MC保持・FREE・応答・反射/counterは発生しない。通常playや他kindへallocationを混ぜず、資源負値/99超/不一致sum/重複tokenは受理しない。

物理3を一枚消費してgift1予約→3資源を同時のdirect assignment→既存finalize_deathsによるHP0自殺/208救済→自然終局/cutoff優先→全action後gift/次ownerの順。総和保存は配分直後までで、208がHP+10を加えれば最終sumは10増える。208の自動消費/gift予約も既存規約どおりで、terminal/cutoffならgiftを破棄する。不安定HP0 sceneを外部から直接loadする契約は増やさない。

配分の選択はpolicy RNGで可行な0〜99/sum保存の組から行い、環境RNGは後のgift抽選だけに使う。環境RNGを使って配分を勝手に変えず、legal照会も抽選しない。main単独なのでMP99への配分があっても各batchの初期MP<=99を保ち、response予算最大3,725は維持する。

公開 `exchange_rule` は `experimental:single-owned-exchange;targetless-self;three-resources-0-99;sum-conserved-before-revival;zero-mp-fee;no-response;one-deferred-gift;death-and-revival-before-gift`。UI算術/自己表示と、target無しというcanonical入力・実gift数・死亡/復活・手番scheduleの**公式server未検証の実験規約**を分ける。sacrificeは終末依存なので今回は導入せず、94+232のunsupportedも維持する。Snapshot18は旧1〜17拒否/view-v16で、追加した現在資源をboardへ反映するが、配分要求の独自イベント履歴を新設しない。

### Match ABI2への移行

型付き配分追加により `gfo_match_*` はABI2へ進む。`gfo_match_command` は168→184byteで、従来prefixの後へint32 hp/mp/cp、has_allocationとreserved_tail[3]を追加する。config24byte/status248byteは据置。構造体を渡す前に `gfo_match_abi_version()` とsize照会を検査する。旧168byteの構造体でABI2へ呼び出してはならず、ABI1 Match clientはversion不一致で停止して移行が必要。ライブラリが不一致な旧構造体の長さを自動判別できると考えない。

catalogの `gfo_*` ABI1、`gfo_state_*` ABI1、legacy37の全ABIは変更しない。Pythonはv18の5KINDSからv20でsellを加え、v21でbuy/accept/declineを加えた9KINDSと `Command` 末尾の任意allocationを使い、入力は3整数のtuple/listだけを許す（generator等のiterableは拒否し、無限列を読み込まない）。例えば `Command(actor, kind="exchange", items=(token,), allocation=(40, 20, 10))` を現在sum70の本人に与える。exchange以外はallocation=Noneとする。C側もexchangeならhas_allocation=1、他kindは0かつhp/mp/cp=0とし、reserved_tail[3]は常に0を要求する。raw snapshotの移行や公式オンラインへのwire送信を実装するAPIではない。

ベンチの `exchange_plays` は使用数、`zero_hp_exchanges` は希望HP0の配分数（復活後HPとは別）、`exchanged_resource_total` は各配分前総和の累計。選択tradeの物理消費は `physical_used` に含め、価格5や配分量を消費数として数えない。

## v19の解決済み除去結果の公開

v19はモデル/重みを増やさず214/437（trade1/98武器/74防具/16雑貨/25奇跡）を維持し、除去結果の公開gapを解消する。rulesetは `official-execution-subset-v19`、Snapshot19（旧1〜18拒否）、公開schemaは `official-execution-view-v17`。Match ABI2/command184/config24/status248、catalog/state ABI1・legacy37は維持する。

`last_removal` はnullまたは `{origin,source,target,model,removed_models}`。originは原発者、source/targetは反射後を含む解決時の現在座標、modelは205または206の単一整数ID、removed_modelsは実際の抽選順の整数ID配列で同モデルのコピー重複を残す。最終bodyが解決したときだけ作り、候補0の解決も空配列[]のeventを残す。使用品自体の消費や後のgiftをremoved_modelsへ混ぜない。

次のmain commandの受理で消去し、不正/失敗commandでは保持する。response処理自体は消去契機ではない。v19〜24の205/206は単一bodyのaction完了時にeventを作り、response phaseとeventは同居しなかった。v25以降の後続death workは別に扱う。reset/load_sceneで消去する。解決後のcutoffではeventを残し、未解決でcutoffした場合はnullのままとする。token/slot・RNG・元eligible数・非除去の他者手札を公開しない。これは全履歴の蓄積ではない。

公開 `removal_event_rule` は `experimental:resolved-body-only;public-removed-models-in-draw-order;duplicates-and-empty-preserved;clear-on-next-accepted-main;no-tokens-or-rng`。他者が対象でもeventを受信したviewerに除去品を表示する経路には公開clientの根拠がある。ただし全参加者/観戦者へのserver配信や空結果eventの必ずの発行は確認していない。ローカル抽選順をevent順とすること・履歴寿命・cutoffの保持は**公式server未検証の実験規約**である。配布プール、除去抽選、資源/死亡/gift/手番の遷移はv18から変更しない。

復元時はdecisions>0、全endpointが既知、model205/206、countが各3/2以下、対応モデルIDだけ、206なら奇跡のみを検査する。v19〜24ではresponse phaseのeventも拒否していた。完了eventとlast_danger/last_sale/last_purchaseの同居は拒否する。完了raw snapshotは構造整合を確認するだけで、その公開結果が過去の公式またはローカル対局で実際に起きたことまで証明しない。pendingはevent無しを要求し、既存の全action replayを維持する。

## v20の売却と満杯配送の未対応境界

v20は4売る/SELL（trade/sell/価格5/giftRate20）を加え、215モデル/重み457（trade2/weapons98/armor74/sundries16/miracles25）とする。rulesetは `official-execution-subset-v20`、Snapshot20（旧1〜19拒否）、公開schemaは `official-execution-view-v18`。Match ABI2のcommand184/config24/status248は変更せず、新kind sell=5を追加する。catalog/state ABI1とlegacy37を保持し、学習器を追加しない。

`Command(actor, kind="sell", items=(sell_token, commodity_token), target=target)` は主手番で、本人所有の物理4と未使用商品一個の順を要求する。二tokenは別物、生存targetを明示し、自己/味方も許可、allocationは無し。used奇跡は商品不可だが、武器・208/209・別コピーの4・未使用奇跡は売れる。play4や追加FREE列へ変換しない。MP費用0で、価格5は操作料ではない。

原発時に4と商品を物理消費し、操作4だけにgift1を予約、商品の補充は無し。奇跡の商品もused保持や取得効果を起こさない。他者宛はtrade/power0/無属性/amount=商品priceのnonattackとしてresponseを作り、空応答または190万能反射だけを許す。自己宛は即時解決。反射で商品とpriceを固定したまま現在source/targetを交換し、再消費しない。

最終bodyでは現在targetのCPから名目priceを引く（不足はMP→HPへ波及）→現在sourceのCPへ名目price全額を加算（上限99、実所持CP額で制限しない）→現在targetへ商品の新token/unused instanceを直接渡す→208復活/死亡→自然終局/cutoff優先→既予約gift/次ownerとする。自己aliasもこの逐次順で、CP不足のHP損失を後のCP加算が戻すことはない。直接配送は環境RNGを使わず、通常の後続gift抽選と区別する。受け取った208による救済もこの順のローカル規約である。

v20〜25では配送先が実物で満杯なら明示unsupported（-5）でatomicに拒否し、samplerも除外する。自己売却は二品消費後の空きを考慮する。現行clientにはserver指定overflowItemの処理があるが、実物のeviction選択は未確定である。旧Flashの満杯配送失敗を無視して料金だけ処理する挙動を流用せず、満杯配送まで含む完全SELL対応とは呼ばない。

`last_sale` はnullまたは解決時の `{origin,source,target,model,price}`。modelは商品IDで、originは原発者、source/targetは現在の解決座標。解決時のみ作り、次main受理/reset/loadでclear、不正/失敗時は保持、解決済みcutoffでは残し未解決cutoffではnullにする。last_danger/last_removal/last_purchaseと排他、v20〜24ではpending/response phaseにも完了eventを置かなかった。現在もpurchase quoteとの同居は禁止する。token・配送先の他手札・RNGを出さず、完了raw snapshotの構造検査を全過去履歴の到達証明とみなさない。

v20〜25時点の公開 `sell_rule` は `experimental:one-unused-commodity;explicit-alive-target;zero-mp-fee;one-operation-gift;foreign-response-and-reflection;buyer-debit-before-current-source-credit;fresh-token-delivery-before-death;full-recipient-unsupported`。商品の232/Aura231/MP武器58/杵94/復活208等のabilityを売却payloadの構成時に実行・継承せず、単一対象・power0・属性無し・商品priceだけを用いる。配送後208の独立した救済hookは別に扱う。商品fakeも既存unsupportedのまま。構造体は同じABI2でも、clientは新opcode5の認識とruleset20のloader照合が必要である。

直接配送後の予約giftは `min(old_gift, hand_limit−new_items_count)` へ防御的に正規化する。実物を追い出す処理ではない。現scopeの正常経路では原発二品消費/gift1、190消費/gift1が作る空きにより通常は超過しない。実物満杯のunsupportedをこのclampで回避しない。

ベンチの `sell_plays` は操作回数、`sell_resolved` は自己即時/他者空応答で最終bodyが完了した数（190反射途中は除く）、`sell_nominal_cp` は名目price総額で実CP支払/HP損失/加算clip量ではない。`sold_miracles` は配送商品が奇跡だった件数でcast/学習ではない。`sold_miracles<=sell_resolved<=sell_plays` を確認する。v20の `physical_used` はSELL一回につき操作4だけ+1、商品は移送として除外し、通常play/defend/exchangeの選択使用や奇跡first/recastへ混ぜない。

## v21の購入確認と商品escrow

v21は5買う/BUY（trade/buy/価格5/giftRate20）を加え、216モデル/重み477（trade3/weapons98/armor74/sundries16/miracles25）とする。rulesetは `official-execution-subset-v21`、Snapshot21（旧1〜20拒否）、公開schemaは `official-execution-view-v19`。Match ABI2/command184/config24/status248は据置で、kind buy=6/accept=7/decline=8とphase purchase=3を追加する。finished=2は維持する。clientは新opcode/phaseとruleset21のloaderを認識する必要がある。catalog/state ABI1・legacy37を保持し、学習器は追加しない。

主手番 `Command(actor, kind="buy", items=(buy_token,), target=seller)` は本人の物理5一個と本人以外の明示alive targetを要求する。自己BUYはserver条件未確定のため明示unsupported（-5）とし、play5・追加MC/FREE・allocation混入を許さない。MP費用0、価格5は操作料ではなく、BUY操作にgift1を予約する。他者へのnonattackは空応答または190万能反射だけを許し、反射後の現在sourceがbuyer/現在targetがsellerになる。原発操作を再消費しない。

最終bodyで現在sellerの未使用手札一個を一様bounded抽選し（used奇跡を除く）、原slot/tokenを特権escrowとして一時的に取り除く。候補0ならdraw無し・見積無しで終了、候補1でもdraw1回。現在buyerのCP<商品priceならその場で元slot/tokenへ戻し、金銭移動無しで終了する。このCP条件に借金支払を使わない。候補生成/抽選・空結果・CP不足終了・slot同一性はserver未確認の実験規約である。

CP>=priceならpurchase phaseへ入りactorは現在buyer、公開 `purchase={origin,buyer,seller,model,price}` だけを示す。現在effect queueは空になるがbatchは残し、主動作/response/抽選/escrowを全journal replayで照合する。元slot/token・候補母数・乱数・他の未使用手札は公開しない。見積中はlast_purchaseを作らない。

buyerの `Command(actor, kind="accept")` / `Command(actor, kind="decline")` はitems空・target無し・allocation無し。YESはbuyerのCPからpriceを減算→sellerへ名目全額CP加算（上限99）→buyerへ新tokenのunused商品を直接配送→既存の復活/死亡・自然終局/cutoff・gift/次手番へ進む。NOはsellerの元slot/tokenへ戻して金銭移動無し。商品分giftは無く、未使用奇跡もcast/保持化せず、配送/返却は環境RNGを使わない。後の通常gift抽選は別に残る。

v21〜25では実物満杯buyerへのYESはunsupported（-5）でatomic拒否し、NOは可能。通常の閉じた経路では操作5または190の消費で空きがあるが、実物eviction規約は導入しない。未完了escrowを残したcutoffでは元slot/tokenへ返却し、予約giftを取消す。既に配送まで解決した結果のgift取消とは区別する。自己BUY/満杯配送を含む完全BUY対応とは呼ばない。

`last_purchase` はnullまたは `{origin,buyer,seller,model,price,bought,reason}`。reasonは `bought` / `declined` / `empty` / `insufficient-cp` / `cutoff`。emptyはmodel0/price0、launch後でも商品未選定のcutoffはnullとする。次main受理/reset/loadでclear、失敗時は保持。見積と完了eventは同居させず、last_danger/last_removal/last_saleとも排他。完了raw snapshotの構造検査は全過去履歴の到達証明ではない。

v21〜25時点の公開 `buy_rule` は `experimental:foreign-only;one-operation-gift;uniform-unused-commodity;empty-no-draw;cp-only-affordability;buyer-accept-or-decline;escrow-return-on-decline-or-cutoff;fresh-token-delivery;full-accept-unsupported`。`bought` はreasonがboughtかどうかから派生するbooleanで、独立した履歴flagではない。特権 `PurchaseOffer` はoriginal item（token/model/used=false）と原slotを保持するが、公開purchaseにはこれらを追加しない。承認/拒否も新しい意思決定として既存cutoff規約に従う。

購入決定をjournalへ一件加えるため、BUY actionだけ既存の保守的response/journal boundへ+1する。非BUYの最大3,725を変えず、全体の保守的な上界へさらに+1を見込めば3,726となる。実際のBUYは単一target・190の物理反射だけなので、これより小さい。購入承認でも商品配送後のgift予約を `min(gift, hand_limit−new_items_count)` に正規化し、実物evictionを行わない。

ベンチは `main_commands+responses+purchase_commands=decisions` へ三分離する。`buy_plays` は操作数、`buy_quotes` は公開見積phase開始数、`purchase_commands` はaccept/decline数、`bought`/`declined` は各決定結果、`buy_empty`/`buy_insufficient_cp`/`buy_cutoff` は各完了reason、`buy_nominal_cp` はboughtの名目price総和（sellerのclip後実増分ではない）。`bought+declined=purchase_commands<=buy_quotes<=buy_plays`。`physical_used` はBUY操作5だけ+1で、商品移動・購入決定・奇跡cast/学習を加えない。v20以前のmain+responses=decisionsは過去版の定義として残す。

## v22の病気付与・再付与・owner tick

v22はonDamage武器30/33/48/62と状態奇跡219/220の六モデルを加え、222モデル/重み483（trade3/weapons102/armor74/sundries16/miracles27）とする。病気はCold/Fever/Hell/Heavenの4状態。v22時点では168のselfCurseと198のboostMPAndAddCurseは未対応で、関連八モデルを一括実装していなかった。rulesetは `official-execution-subset-v22`、Snapshot22（旧1〜21拒否）、公開schemaは `official-execution-view-v20`。Match ABI2の幅・opcode0〜8・purchase phase3/finished2、catalog/state ABI1・legacy37は維持し、学習器は追加しない。

一人の病気は一種類とし、Flash/DarkCloudとは独立に持つ。病気が無ければincomingを付け、既存病気があれば `max(incoming_rank,current_rank+1)` へ進める。既にHeavenへ病気を再付与するとHP0に落とし、Heaven状態を保つ。Flash/DarkCloudの付与はこの病気再付与と見なさず、Heavenfallを起こさない。これらの厳密なrank/落下処理はローカル実験規約である。

30はNE/ATK8/Hell、33はNE/ATK9/Cold、48はNE/ATK13/Cold、62はStone/ATK1/Coldで、正の通常target HP実被害に応じて既存onDamage経路で付与する。219はCold/MP6、220はHeaven/MP15の状態奇跡でused保持・FREE・適合する反射/遮断/跳ね返しを扱う。再付与でのHeavenfall分をcounter基準へ足さず、既存のregular target damage段階の実損失だけを使い、208救済後に生存していればsurvivor counterの対象となる。

mild解除199/237はCold/Fever/Flashを取り、Hell/Heaven/DarkCloudを残す。all解除200/238は対応する病気4種とFlash/DarkCloudの全六種類を対象とする。霧/夢まで新たに実行可能にする意味ではない。

元ownerの全actionが完了してadvanceへ入る冒頭でだけ病気tickを行う。ownerが既deadまたは病気無しならdraw無し。対象ならbounded100を一度引き、roll<5で一段悪化→変化後のCold HP−1/Fever−2/Hell−5/Heaven HP+5を適用する。Heavenからの悪化はHP0へのfallで、そのtickのhealはしない。次に208救済/死亡→自然終局→既予約gift→次ownerへ進む。固定5%を採用し、旧FlashのworseChance増加式を使わない。

body側の自然終局/decision cutoffが先に成立した場合はadvance/tickへ入らない。完成bodyでのcutoffでもtick無し。反射・連撃・全体攻撃の各frame間、counter FIFO途中、BUY見積中にはtickしない。BUYのaccept/decline等で全actionを完了した場合の対象は現在buyerではなく元ownerである。ownerが生きたまま同action中に208救済された場合も、この全action完了の境界で判定する。これらの対象・抽選順・境界・gift scheduleは公式server未確認の実験規約である。

nativeの特権 `last_tick` はHP/病気before-after・worsened/fallen・revival_tokenを記録し、body被害/202統計・除去・tickによる208消費を分離する。内部tick record・token・RNGを公開JSONへ加えず、公開するのはdisease_ruleとboardの病気/資源状態だけ。完了raw snapshotの構造検査は全公式履歴の到達証明ではない。

v22/23時点の公開 `disease_rule` は `experimental:one-disease-plus-flash-and-darkcloud;reapply-max-incoming-or-next;heaven-reapplication-hp0;owner-only-after-whole-action;natural-and-cutoff-before-tick;fixed5-percent-before-hp;post-upgrade-tick;post-tick-revival-before-gifts`。v22/23のraw stateは病気最大一種→Flash→DarkCloudのenum順・重複無し、最大三curseのcanonical集合だった。v24のFog拡張は後述する。tick recordは次main受理でclearし、現在の公開boardから過去のroll/tokenを復元して返す機能は作らない。 現行clientの病気表示は新たな受信curseで旧病気一種を置き換え、非病気Flash/DarkCloudは独立に残す。旧FlashのHeaven中に非病気を付けても転落させる挙動は流用しない。

ベンチの `disease_ticks` / `disease_worsened` / `disease_falls` はtick/自然悪化/Heavenfall件数、`disease_tick_revivals` はtick由来208救済数。`disease_hp_gained/lost` は復活前のtick HP増減を集計し、救済時はrawAfter=0として208の+10を除き、Heavenfallの実HP損失も含める。累計 `revivals` はbody由来とtick由来を含む。HP増減202のbody集計からtick HPdeltaとtick救済を外し、205/206のtoken消失数から同targetのtick208消費を除いて公開除去eventと照合する。これらの内部会計のためにlast_tick/tokenを利用しても、公開JSONには出さない。

## v23の自己病気防具とMP/Heaven雑貨

v23は168熱狂仮面（Fire armor/普通DEF10/selfCurse Fever/価格10/giftRate1）と198天国草（NE sundry/MP+20とHeaven/価格20/giftRate1）を加え、224モデル/重み485（trade3/weapons102/armor75/sundries17/miracles27）とする。MP費用/guardianはどちらも無い。rulesetは `official-execution-subset-v23`、Snapshot23（旧1〜22拒否）、公開schemaは `official-execution-view-v21`。Match ABI2の幅・commands0〜8・purchase phase3/finished2、catalog/state ABI1・legacy37を保持し、学習器は追加しない。

168はmain使用不可の普通防具で、属性適合を満たせば同copy複数を順序付きで使える。Flashなら一枚だけ、leading filter→168→普通防具は可能、168と190/壁/跳ね返し等の特殊反応は同じ列に併用できない。通常の全response消費後、選択順に各168の自己Feverを一回ずつ付ける。同modelだからと副作用をdedupせず、完全防御でincoming被害0でも自己付与する。

その後incoming bodyを適用し、全body後にだけ208救済/死亡を確定する。prefixでHeavenfallによりHP0になっても本体前に蘇生させず、HP0&&!deadは同じtransactional step内だけに留める。応答待ちsnapshotへHP0を許すように制約を緩めない。counter基準はincoming bodyのtarget HP実損失だけで、prefix落下分を足さない。ただし自己HP吸収aliasならprefix HP0→source回復→target被害という順で正のbody損失が発生し得るため、prefix落下後のcounterが常に0とはしない。最終survivorは既存208救済後に判定する。

198はmainのplay専用でresponse品には選ばず、MP+20を先に0〜99clipしてからHeavenを付与し、全body後に救済/死亡を処理する。既にHeavenならMPは増加した後に再付与fallが起こり得る。自己は即時、他人はnonattack utilityとして空応答または190万能反射だけ、反射後の現在targetへ適用する。FREE/追加品・保持化・MP費用は無く、能力自体のRNGも0。全action後の病気tick/gift RNGは別である。

168/198をdiscardや売買商品として消費/移動しても、この新abilityを発動しない。商品として届いたことから自己FeverやMP20/Heavenを付けず、guardianを召喚しない。既存の一病気+Flash/DarkCloud、mild/all、固定5%/元ownerの全action完了tick、自然終局/cutoff優先、208/gift順はv22を維持する。

公開 `disease_item_rule` は `experimental:response-self-curse-in-selection-order-after-consumption;prefix-before-body;death-and-revival-after-whole-body;counter-basis-body-target-loss-only;mp-boost-before-disease;no-trigger-on-trade-or-discard`。clientに根拠がある普通防具の選択/属性・selfCurse選択indexの演出・198のMP20/Heaven表示と、消費後の順序/暫定HP0/208/body/counter/死亡を接合する**公式server未確認の実験規約**を区別する。失敗時は状態/RNG/出力を含む全stepをrollbackし、新たな副作用history/token/RNGを公開しない。 198は全response後のbodyでMPを増やし、nonattackへの反応は190だけなので同batchの有償MP反応予算を増やさない。v23/24時点のjournal上界は非BUY3,725/BUY3,726だった。v25のdeath追加分は後述する。新ベンチ `fever_mask_uses` は応答で使った168コピー数、`heaven_herb_plays` は198のmain開始数で、捨てる/商品移動を含めない。`heaven_herb_resolved` は自己即時または他者空応答の最終body解決数（190反射途中は除く）、`heaven_herb_mp_gained` は当該targetのMP after-before実増分で、99cap後の値を数える。同body後のtick/giftはMPを変えないため分離でき、名目20倍値に置き換えない。resolved<=plays/実増<=20*resolvedを検査する。未解決cutoffなら開始だけ、解決済みcutoffならbody実増も計上する。

## v24のFog：対象選択・表示・queryの境界

v24は72（Water weapon/ATK3/onDamage Fog/p10w1）、97（Water weapon/ATK3/50%/onDamage Fog/p8w1）、182（Water armor/counter Fog/power0/DEF0/p10w1）、221（Water miracle/Fog/MP3/w1）を加え、228モデル/重み489（trade3/weapons104/armor76/sundries17/miracles28）とする。rulesetは `official-execution-subset-v24`、Snapshot24（旧1〜23拒否）、公開schemaは `official-execution-view-v22`。Match ABI2の幅・kinds0〜8・purchase phase3/finished2、catalog/state ABI1・legacy37を維持し、学習器は追加しない。

raw stateは病気最大一種→Fog→Flash→DarkCloudのenum順/重複無し、最大四curseへ広げる。mildはCold/Fever/Fog/Flash、allは対応する病気4種/Fog/Flash/DarkCloudの全七種類を解除する。Fogは非病気でHeaven再付与fallを起こさず、Fog単独には自然tickを追加しない。72/97の付与は既存の正target HP実損失、182はsurvivor/FIFOの純curse反撃、221は保持/FREEと適合する魔法反応に従う。

Fogの本人がmain playまたはbuyで明示的に敵を指定した場合、launchでalive敵をinput順に列挙して一様bounded drawを一度引き、候補一人でもdrawする。202を使う場合はこの対象draw→符号drawの順。自己/味方指定は変えず（自己BUYの既存unsupportedは維持）、%攻撃・Wide232・Danger94はFog drawを除外する。14は選択先に二回、redirect/bounce/counter/購入YESNOで選び直さない。祈り/discard/exchangeもこの敵指定launchに含めない。

教典の「敵に使った神器は敵内でrandom」に対し、一様性/一人時draw/launch時点/例外/順序は**公式server未確認の実験規約**である。特権 `last_fog={owner,intended,target,models}` は次main受理でclearし、公開JSONへ出さない。pendingの全journal replayは元command/状態/RNGから対象選択とpayloadを検査するが、完了raw sceneの全歴史の到達証明ではない。

観測は既存project_viewで、生存Fog viewerかつ未終局のときだけ生存敵statusをnullにする。自己/味方/dead敵、観戦/dead viewer/終局後には覆いを掛けない。通常status欄と別の使用済み奇跡paneに表示される名前・team・usedモデルは公開のまま。未使用の他者手札枚数/中身は出さない。

`pending.forced_hit` は現在targetではなく、命中判定時の `initial_target` のboard paneがFogで隠れるviewerに対し、内部true/falseの両方をnullにする。元のhidden敵のDarkCloud判定が190反射で可視selfへ戻ってもnullを維持し、可視の元targetならboolを返す。内部の必中判定/乱数消費は変えない。pending payloadやpurchase/removal/sale等の公開eventは別表示経路として保持するが、全viewerへのserver fanoutは未照合である。

v24/25ではFog本人のforeign SELLは敵/味方を問わず、配送容量を読む前にunsupported（-5）として拒否し、自己SELLは許す。samplerはFogの有無にかかわらずSELLを自己targetに固定し、hiddenな他者未使用手札数を配送先候補から読まない。非Fogで手動foreign SELLをしたときの旧overflow-capacity依存gapは残す。全query/全profileの情報非干渉を達成したとは主張せず、この限定拒否と方策変更を明示する。

ベンチの `fog_main_commands` は受理main前のownerがFogだった全kind、`fog_launches` はそのmainでlast_fogを生成した数、`fog_target_changed` はintended!=selectedの数。response中のrecord残存を重複計上せず、changed<=launches<=fog_main_commands<=main_commandsを検査する。2人戦の候補一人で対象不変でも、launch drawは一回消費する。view-v22のforced_hitはbool/nullの三状態として読み、nullをfalseと同一視しない。

v24/25時点の公開 `fog_rule` は `experimental:launch-once-for-explicit-enemy-target;uniform-alive-enemies-in-input-order;one-draw-including-singleton;self-and-allies-unchanged;chance-wide-danger-excluded;redirects-no-reselection;fog-foreign-sell-unsupported;sampler-sell-self-only`。canonical集合を広げた現行 `disease_rule` は `experimental:one-disease-plus-fog-flash-and-darkcloud;reapply-max-incoming-or-next;heaven-reapplication-hp0;owner-only-after-whole-action;natural-and-cutoff-before-tick;fixed5-percent-before-hp;post-upgrade-tick;post-tick-revival-before-gifts`。病気のrank/5%/owner境界やFlash/DarkCloudの既存規約を変えず、Fogの導入を旧版のnot-forced条件へ置き換えない。

## v25の昇天弓：通常使用と死後FIFO

v25は112昇天弓（Light weapon/通常ATK1・25%/死亡時ATK30・75%/価格10/giftRate1）を加え、229モデル/重み490（trade3/weapons105/armor76/sundries17/miracles28）とする。rulesetは `official-execution-subset-v25`、Snapshot25（旧1〜24拒否）、公開schemaは `official-execution-view-v23`。Match ABI2の幅・kinds0〜8・purchase phase3/finished2、catalog/state ABI1・legacy37を維持し、学習器は追加しない。

主動作の112は通常の25%攻撃としてtarget無しのalive敵rosterを使い、消費/支払/命中/RNG/応答は既存規約に従う。死亡時は別の自動payloadで、Light30/75%・モデル112・death来歴を持つ。通常の1/25%から動的値を識別し、旧117の通常75%を現行112へ流用しない。

208救済を先に処理し、実際にdeadになった人物だけを対象とする。全bodyのactual death flags確定後に自然終局なら112を消費/予約しない。非終局なら新死者の在庫112を全copy在庫順に直接消し、copyごとにその境界のalive敵input順へframeを一括予約する。死亡者へgift予約は増やさず、通常consume helperとは分ける。同じcopyが複数enemy payloadの来歴になっても、物理消費は一回だけである。

decision cutoffはbody/死後の112消費・予約より後に判定する。従ってこの場合のcopyは消費済みで、残workを取消し、まだ開始していない命中roll/responseは行わない。自然終局優先では生存者一人・全員同じ非Solo team等の既存条件も使い、duelの最後の敵死亡から道連れを発生させない。このgate・全copy・消費順・cutoffの差は**公式server未確認の実験規約**である。

新しいdeath FIFOを普通counter FIFO・初期rosterより先にdrainし、途中で新たに死亡した人物のframeは末尾へ追加する。死亡者はsource/originのまま自動攻撃できるが、dead actorの手動commandを許す意味ではない。ownerは元mainのまま。対象deadはskip、alive対象への活性化時に75%を抽選し、DarkCloudならdraw無し必中。Fogの追加再選択はせず、反射/跳ね返しで再命中rollしない。返された先がdeadならframeを終了、bounceは既存alive候補へ向ける。普通防御・属性filter・適合反応は現在payloadに従う。

全main kindのprestate/原command/受理response journalから同executorを再生し、死者・消費copy・death FIFO・RNG・普通counter・現在frameを照合する。病気tickによる死亡も同じ仕組みへ接続し、特権 `tick_processed` stageで元ownerのtickを一回に限る。tick後に死亡時応答を待っても、完了時に同ownerへ再tickしない。giftと次ownerは全work後まで遅延し、自然終局/cutoffで残work/giftを取消す。安定raw sceneの全過去履歴まで証明する契約ではない。

death frame数の保守的上界は `player_capacity*(player_capacity−1)*hand_limit`（最大9×8×18=1,296）。copy数とframe数を混同せず、journal上界へこのdeath_boundを加える。旧v23/24の最大3,725/BUY3,726だけを死亡queueへ流用しない。死亡FIFO・自動消費token・未来RNG・stage・内部累計は公開せず、現在の `pending.death` booleanとdeath_ruleを公開する。死亡時payloadでもforced_hitのinitial_targetに対するFog null化は維持する。

ベンチの `death_bows` は自動消費した112コピー数で、通常の `physical_used` へ混ぜない。`death_created` は全予約frame数、`death_started` は75%命中または暗雲forcedでalive actorのresponseを開始したframe数、`death_rolls/hits` は実抽選/成功、`death_forced_hits` は無draw命中、`death_responses` は反射等を含む実defend回数。bows<=created<=8*bows、hits<=rolls<=created、started=hits+forced、forced<=created−rolls、responses>=startedを検査する。新しいtickだけを集計し、死後応答中に残るlast_tickを重複計上しない。自己205/206の後に同tickで消えた208/自動112を除去eventのtoken消失数へ混ぜない。公開JSONへ累計/未来FIFO/来歴stageを追加しない。通常配布で未対応のgiftモデルは残り8種で、Dreamは調査のみとする。

公開 `death_rule` は `experimental:revival-before-actual-death;nonterminal-boundaries-only;consume-all-bows-in-inventory-order;copy-then-alive-enemies-in-input-order;light30-75-percent;death-fifo-before-counters-and-initial-roster;dead-source-allowed;darkcloud-forces-without-draw;redirects-no-reroll;dead-redirect-target-ends-frame;no-fog-reselection-or-gifts;owner-tick-once;natural-and-cutoff-cancel-pending`。教典の死亡時75%攻30とclientの受信演出を、ここで選んだqueue/終局/再帰/救済/giftのserver規約と取り違えない。

## v26の取引overflow・Fog SELL・capacity query

v26はモデルを増やさず229モデル/重み490（trade3/weapons105/armor76/sundries17/miracles28）を維持し、SELLの満杯配送・BUYにも共用する配送helper、Fogのforeign SELL、先行capacity queryのgapを扱う。rulesetは `official-execution-subset-v26`、Snapshot26（旧1〜25拒否）、公開schemaは `official-execution-view-v24`。Match ABI2の幅・kinds0〜8・purchase phase3/finished2、catalog/state ABI1・legacy37を維持し、学習器を追加しない。Dreamは調査のみで未対応のままである。

取引配送先の実物itemsが上限なら、配送前からある全実物をinventory順に列挙し、uniform bounded draw一回で一copyをevictしてから新token/unused商品を渡す。使用済み奇跡、208/209/112等のpassiveも候補で、同モデル別copyを別候補として扱う。一候補でもdraw一回。新商品自身や補充placeholderから選ばず、空きがあればoverflow draw無しで直接配送する。使用済み奇跡の商品選択除外と、overflow候補としての可否は別契約である。

v26当時の229モデルprofileではBUYのbuyerは操作5または反射190を消費するため常にcap−1以下で、quote中の流入/giftも無い。したがって満杯BUYはこの閉じたprofileでは到達不能で、共通配送helperの備えに留まる。検証はfullSELLで共用helperの全候補順を確認し、通常BUYでoverflow_model=nullを確認するもので、偽raw quote importでBUY fullを到達させたcoverageを主張しない。

CP debit/current sourceへのfull nominal credit→既存itemのeviction→fresh delivery→既存208救済/死亡/112予約の順。eviction自身に補充・MP返金・curse解除・passive等のabilityを起こさない。残った/届いた208や112は後の実死亡境界で通常のhookに従う。使用済み奇跡を失った場合はused paneから消えるが、付与済み状態を取消さない。配送後の予約gift min-clampは維持する。BUYのdecline/未決cutoffは原slot/tokenへの返却なので、このoverflow抽選を行わない。

`last_sale` / `last_purchase` に任意の `overflow_model` を加え、eviction無しならnull、有りなら失ったモデルIDだけを公開する。token/元slot/候補母数/used flag/RNGは追加しない。受信clientのoverflowItem→reveal/本人手札消去/used pane更新に表示根拠があるが、全viewerへのserver fanoutは未確認。uniform分布・全実物候補・一候補draw・補充/死亡順は**公式server未確認の実験規約**であり、これで公式サーバーの完全な取引仕様を取得したとは呼ばない。

Fogのforeign SELLをunsupportedにせず、play/buyと同じく明示enemy hintならlaunchでalive敵input順から一様draw一回（一人でも一回）を行う。自己/味方は維持する。商品が232/%武器/Danger等であっても、商品を対象modifierへ混ぜず、操作SELLだけでtarget/power0/element無し/priceを決める。反射後の再選択は無い。samplerのSELLは全alive input順のtargetを候補にし、他者容量をpeekして除外しない。legal/samplerが満杯か否かで事前拒否する依存を解消し、overflow結果は行動解決後の公開eventとする。

v19〜24では単一body完了がaction完了で、完了eventはresponseと同居しなかった。v25以降のdeath FIFOでは元の除去/売却/購入bodyが完了しても後続death応答が残り得るため、last_removal/last_sale/last_purchaseはこの応答中も保持する。購入quoteとの同居は禁止し、次main受理でclear・失敗保持・相互排他・解決済みcutoff保持は維持する。pending journalの全replayで取引/抽選/overflow/死後workを照合するが、完了raw stateの全過去到達履歴は証明しない。

ベンチ `sell_overflows` / `buy_overflows` は各解決stepで一回だけ数え、`overflow_events=sell_overflows+buy_overflows`、sell_overflows<=sell_resolved、buy_overflows<=boughtを検査する。`overflow_event_checksum` はSELL/BUYのkind5/6を分けた公開event tupleから作りsum集計し、slot/token/元候補数は含めない。これは通常observeへ内部累計を追加するものではない。

自己BUY、通常giftが満杯時のclamp/抽選scheduleの公式照合、94+Wide等の別gapは残す。ここでのfull配送対応はSELL/BUY直接配送の範囲であり、一般の全配布/全モデル/全効果への対応を主張しない。

公開 `overflow_rule` は `experimental:trade-delivery-only;full-recipient-uniform-existing-item-in-inventory-order;one-bounded-draw;all-existing-items-including-used-miracles-and-passives;cp-debit-and-credit-before-eviction;eviction-before-fresh-delivery-and-death;no-eviction-gift-or-ability;public-model-only;no-overflow-on-decline-or-cutoff-return`。

公開 `sell_rule` は `experimental:one-unused-commodity;explicit-alive-target;zero-mp-fee;one-operation-gift;foreign-response-and-reflection;buyer-debit-before-current-source-credit;fresh-token-delivery-before-death;full-recipient-uniform-existing-item-overflow`。

公開 `buy_rule` は `experimental:foreign-only;one-operation-gift;uniform-unused-commodity;empty-no-draw;cp-only-affordability;buyer-accept-or-decline;escrow-return-on-decline-or-cutoff;fresh-token-delivery;full-recipient-uniform-existing-item-overflow`。

公開 `fog_rule` は `experimental:launch-once-for-explicit-enemy-play-buy-sell;uniform-alive-enemies-in-input-order;one-draw-including-singleton;self-and-allies-unchanged;chance-wide-danger-excluded-except-sale-commodity;redirects-no-reselection;sampler-sell-all-alive-without-capacity-peek`。

## APIと再現性の境界

`Match::legal` はこのsubsetの現在局面に対する検査。`step` の不正入力・未対応入力では状態、乱数、意思決定数を変更しない。reset/load/restoreも検査とコピーを済ませてから置き換え、例外時に既存のmatchを一部更新しない。現行v47の `MatchSnapshot` はversion47を持ち、config、raw state、owner/actor/phase、待機effect、補充予定、乱数・token・意思決定数、終局結果を保持する。旧version1〜46を自動移行せず拒否する。`restore` は対応範囲と構造・待機effectの整合を検査後に一括で置き換える。任意のraw sceneの履歴が公式サーバーで到達可能であることまで証明しない。

応答待ちsnapshotには主動作command、全response journal、各現在frameの `RedirectFrame{defender,models,tokens,target}` を特権来歴として保存する。v7はActionBatchのlaunch前状態から全executeを再生し、主動作/防御の選択token重複・普通品消費・奇跡used保持・MP・各人の正確な補充予定に加え、派生counterのモデル/威力/originとFIFOを検証する。counterのモデルを主動作で直接使用したtoken列と取り違えない。`EffectFrame.redirect_rng` やActionBatchのRNG anchor、counter FIFO・journalは再現検査用であり、公開pendingへ出さない。v1の「owner以外の補充予定は必ず0」という条件は、反応者も品を消費する現行には適用しない。

raw snapshotには他人の手札や将来のローカル乱数状態があるため、ポリシーの観測として渡さない。表示は `observe(viewer)` などの投影経路から取得する。snapshotはデバッグ・再現用の特権状態であり、公式の通信スナップショットと同じ形式でも、バージョンをまたぐ永続checkpointでもない。

[official_match_c_api.h](../include/godfield/official_match_c_api.h) の `gfo_match_*` ABI2と [Python `official_match.py`](../python/official_match.py) はこの実行器のsubset・実験scopeを明示し、read-onlyの `OfficialData` / `OfficialState` と区別する。`Match` / `Config` / `PlayerSetup` / `Command` を用い、reset、legal、step、status、observe、外部policy RNGによるsample_actionやrandom_rolloutを提供する。Pythonの `load_scene(OfficialState, owner, seed)` と対応C APIはboard-onlyの安定したsceneから新しい実行を始める。Pythonは非空attacksを拒否し、event履歴を待機queueへ補完しない。RNG/journalを含む実行checkpointのimport/restoreはこの公開APIに提供しない。

従来の `random_rollout` は `sample_action` / `step` の反復であり、正の混乱mainとgrant phase4で停止する。通常samplerは207の起動も生成しない。外部driverが `sample_confusion` / `confusion_step`、`sample_grant` / `grant_step` を区別し、playerが選ぶ207 tokenは `sample_fate` / `fate_step` へ渡す必要がある。choiceとstepは別transactionなので、chance RNGはstep成功後だけcommitし、Match snapshotとは別に保存する。

現行v47の `observe` はschema `official-execution-view-v45` を使い、Match C ABIは2とする。rulesetはv47、通常配布は既定236/499または `include_fate=True` の237/500とし、201/239召喚・canonical42自動行動/離脱と外部指定行動/typed Earth・Moon payload/global現象5種/sourceなし現象3種/typed磁気嵐295/混乱289と外部指定主動作/外部結果付き207/外部指定悪魔5種/終末閾値とささげる/opt-in補充chance phase4を扱い、`local_guardian_payloads=True` ではEarth120/Moon26の限定payloadを自動機会へ接続する。top-levelに `overflow_rule` / `death_rule` / `fog_rule` / `disease_item_rule` / `disease_rule` / `buy_rule` / `sell_rule` / `removal_event_rule` / `exchange_rule` / `danger_rule` / `revival_rule` / `random_hp_rule` / `wide_attack_rule` / `mana_weapon_rule` / `repeat_attack_rule` / `removal_rule` / `darkcloud_rule` / `flash_rule` / `damage_effect_rule` / `resource_counter_rule` / `counter_rule` / `chance_rule`（前述の実験規約）と `bounce_target_rule="experimental:uniform-all-alive-in-input-order;includes-self-and-teammates"` を明示する。実験matchのphase/owner/actor・終局フラグ・意思決定数、fresh board view、前述の実行済launch要約 `last_danger`、解決済み除去要約 `last_removal`、解決済み売却要約 `last_sale`、公開購入見積 `purchase` と完了要約 `last_purchase`（両取引eventのoptional overflow_modelは失ったモデルIDだけ）、元の待機effectモデル/威力、`pending.reflections` に反射件数、`pending.bounces` に跳ね返し件数、`pending.hit_rate` に命中率（通常攻撃は0）、`pending.environmental` にsourceなしの現象か（その場合origin/sourceはnull）、`pending.counter` に現在payloadがcounterか、`pending.death` に112の死亡時payloadか、`pending.forced_hit` に暗雲による無抽選必中だったかを返す。counter181の75は開始後の表示で、初期全敵chance攻撃とは区別する。属性消去の回数は返さず、現在属性 `element=null` に反映する。次の配布・乱数・他者未使用手札・補充予定の内訳・来歴token・action batchの将来roster/cursor/anchor/journal・counter FIFOは観測へ出さない。対応モデルの照会やローカルランダム実行をできるようにしても、公式オンライン対局へ接続したりコマンドを送ったりする機能ではない。

実行例は `python3 python/official_match.py`。ライブラリパスを別に指定する場合は同スクリプトの引数へ渡す。これは初期化してローカルに進める例であり、公式対局traceの取得ではない。

旧37profileとその台帳・Env・C ABI/Pythonは変更しない。この段階で学習器や学習済みモデルは追加しない。公式完全対応に向けては、一般effect queue、実配布・抽選・死亡・ターン順の一次照合、互換クラス外Dream、取引server挙動、9人/チームでの多段効果、Earth/Moonの一般神器/奇跡選出、207の自動現象抽選/混乱の自動選択分布・悪魔の自動出現/補充連鎖等が残る。

## 検証と計測

次節にv26の最終検証を記録する。後続のv1〜v25記録は各版の歴史的証拠として保持し、今回のhash一致や速度の証拠へ読み替えない。

### v26の検証と取引overflow集計

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

### v25の過去の検証と死亡時攻撃集計

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

### v24の過去の検証とFog集計

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

### v23の過去の検証と仮面/天国草集計

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

### v22の過去の検証と病気tick集計

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

### v21の過去の検証と購入集計

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

### v20の過去の検証と売却集計

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

### v19の過去の検証と公開除去結果

v19は全source/test最終freeze後のfull Release CTest129件/21.01秒・ASan/UBSan89件/51.62秒が通過した（各 `-j4` 同時、rolloutとの競合あり）。最終Release/SAN full buildはwarning/error0。focused Release3件/1.27秒（native0.46/C API0.02/Python0.79秒）・SAN2件/1.18秒（native1.03/C API0.15秒）が通過した。native零引数55関数のmain明示登録/unused guard、Python AST38ケース、Release/SAN双方の `fixed_query_probes==384` 完走を確認した。公式配信assetは上記の差分確認であり、fresh hash一致とは記録しない。

| v19人数 | 試合数 | 意思決定数 | 遷移checksum | 解決済み除去event | 空結果event | 除去event checksum |
| --- | --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,839,935 | `4941009284767353250` | 10,194 | 4,120 | `16555731515668663194` |
| 9 | 10,000 | 2,428,289 | `11062027547654796276` | 7,817 | 2,655 | `13048935337781861835` |

seed42の2人100,000試合/9人10,000試合を各1 worker/4 workersで実行し、worker数・時間以外の全fieldが一致した。v18 baselineともruleset・時間等の計測fieldを除く既存fieldがchecksumまで全一致し、除去結果の公開追加が遷移/抽選/既存集計を変えないことを確認した。全試合の終了snapshotを復元できた。`removal_events=hand_removal_resolved+used_removal_resolved` と公開removed_modelsの累計数が実除去コピー数に一致するguardも通過した。空結果は0候補でも解決したbodyのeventで、未解決cutoffを含めない。Release/SAN seed42各1,000試合も時間以外の全fieldが一致し、v18の既存非時間fieldとも一致した。遷移checksumは2人 `1463939626686470704` / 9人 `375287159278559213`、新除去event checksumは2人 `17881167250890207477` / 9人 `11841601995346163861`。今回はquiet速度を再計測せず、v19の速度値や改善倍率を主張しない。

### v18の過去の検証と集計

v18（214モデル/重み437）。全Release CTest129件/17.79秒・ASan/UBSan89件/46.30秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、release/sanitize/bench/SAN bench/native/bridgeの6ログはwarning/error0。focused Release3件/1.08秒（native0.29/C API0.02/Python0.77秒）・SAN2件/1.36秒（native1.00/C API0.35秒）が通過した。Python MatchはAST確認36ケース、native零引数52関数は全てmainへ明示登録を確認した。既存C API384照会guardも完走し、公式assetsのfresh取得hash/296モデルfactsも一致した。

新native3局面でsum/fee/curse保持/gift/死亡/noop、総和1と297の端点、不正10例・response禁止・late failure・待機journalへのallocation混入拒否を検証した。C APIはABI2/sizeof184とloader、tailのflag/range/padding/sum/target/role/重複8例のatomic出力、samplerのkind4配分、HP0の死亡/208救済を検証した。Python3追加ケースはdirect/noop/cutoff/HP0/復活、tuple/listの3整数制限とgenerator拒否、policy allocationが環境状態を変えないことを確認した。

以下はseed42の大規模集計。

| 人数 | 試合数 | 意思決定数 | 勝数 | draw | 打切り | checksum |
| --- | --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,839,935 | 49,689 / 50,169 | 142 | 0 | `4941009284767353250` |
| 9 | 10,000 | 2,428,289 | 1,111 / 1,116 / 1,077 / 1,145 / 1,154 / 1,099 / 1,155 / 1,028 / 1,091 | 23 | 1 | `11062027547654796276` |

| 人数 | 両替使用 | 希望HP0 | 配分前sum合計 | 選択physical使用 |
| --- | --- | --- | --- | --- |
| 2 | 69,512 | 1,229 | 4,247,506 | 2,454,696 |
| 9 | 31,266 | 601 | 1,782,132 | 1,694,909 |

希望HP0は配分要求の分類で、208救済後の最終HPを数えない。sum合計は配分前の総和で、価格5や復活+10を引き算/足し算した額ではない。v18のphysical_usedはplay/defend/exchangeで選択使用した非MC数となり、208/209の自動消費は別計上を維持する。zero_hp<=exchange_plays、exchange_plays<=sum合計<=297*exchange_playsを確認した。

| 人数 | 杵launch | 臼誘引 / 自己選択 / 味方選択 |
| --- | --- | --- |
| 2 | 6,914 | 502 / 3,508 / 0 |
| 9 | 4,330 | 945 / 822 / 0 |

| 人数 | 復活発動数 | 202使用 / 最終解決 | heal / damage解決 | 202実HP増 / 減 |
| --- | --- | --- | --- | --- |
| 2 | 5,204 | 10,150 / 10,150 | 5,082 / 5,068 | 50,817 / 48,558 |
| 9 | 5,168 | 7,685 / 7,685 | 3,837 / 3,848 | 38,370 / 36,343 |

| 人数 | 232使用 / 完了frame | 14使用 / 完了frame | 58使用 / 全支払いMP / 基底MP |
| --- | --- | --- | --- |
| 2 | 8,287 / 8,349 | 6,979 / 13,824 | 6,942 / 83,965 / 83,060 |
| 9 | 6,721 / 37,115 | 4,270 / 8,870 | 4,345 / 51,501 / 50,676 |

杵/臼/自己/味方はlaunch件数、全員Soloのため味方0。202実HP増減は復活を除くbody値、wide/repeat完了frameはblock/全防御0も含み14+232では両集計へ入る。既存danger/randomHP/revivals/mana/removal/wide/repeatの上限も確認した。

| 人数 | chance攻撃 | chance実draw / hit / miss / forced | counter生成 / 開始 | counter実draw / hit / miss / forced | 反射 / bounce / 消去 / 遮断 | 最大redirect深度 |
| --- | --- | --- | --- | --- | --- | --- |
| 2 | 109,230 | 104,577 / 60,382 / 44,195 / 4,653 | 19,283 / 18,459 | 3,052 / 2,272 / 780 / 156 | 16,589 / 24,253 / 15,341 / 8,371 | 5 |
| 9 | 46,176 | 170,075 / 97,526 / 72,549 / 11,908 | 14,292 / 13,675 | 2,277 / 1,686 / 591 / 132 | 13,295 / 18,938 / 13,040 / 8,954 | 5 |

| 人数 | 205解決 / token除去 | 206解決 / token除去 | 閃光追加 / 解除 / 応答 | 暗雲追加 / 解除 |
| --- | --- | --- | --- | --- |
| 2 | 5,174 / 15,031 | 5,020 / 1,028 | 11,232 / 2,753 / 38,356 | 13,785 / 1,281 |
| 9 | 3,960 / 11,708 | 3,857 / 1,450 | 8,766 / 3,287 / 43,941 | 10,677 / 1,670 |

| 人数 | MP反撃解決 / 実増分 | CP反撃解決 / 名目額 | 吸収正解決 / 名目HP | 同被害正解決 / 名目HP |
| --- | --- | --- | --- | --- |
| 2 | 3,092 / 45,330 | 2,990 / 22,459 | 19,864 / 157,570 | 6,156 / 73,994 |
| 9 | 2,233 / 32,463 | 2,103 / 15,126 | 17,771 / 121,760 | 3,902 / 45,957 |

| 人数 | 乱気流 初 / 再 | 壁 初 / 再 | 自己bounce応答 | FREE品使用 main / response | 免除MP | Aura | setter |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2 | 736 / 236 | 3,291 / 784 | 12,141 | 17,425 / 13,715 | 71,205 | 6,309 | 17,117 |
| 9 | 1,181 / 340 | 2,672 / 1,191 | 3,227 | 13,658 / 10,414 | 60,554 | 5,959 | 10,428 |

各人数の1 worker/4 workersでworker数・時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。Release/SAN seed42各1,000試合でも時間以外の全fieldが一致し、checksumは2人 `1463939626686470704`、9人 `375287159278559213`。最大redirect深度は各表に示す観測値で、機能上の連鎖上限ではない。

競合を避けた独立Release・1 workerは、2人100,000試合が2.31177秒（1,661,030意思決定/秒）、9人10,000試合が2.58239秒（940,327意思決定/秒）。baselineと時間以外の全fieldが一致した。合法手sampler・実行・reset・各試合の終了snapshot検証込みで、合成単体・Python学習・モデル推論/最適化・公式全モデルの速度ではない。プールと方策が異なるため版間の速度改善倍率を主張しない。

### v17の過去の検証と集計

v17（213モデル/重み417）。全Release CTest129件/18.30秒・ASan/UBSan89件/48.50秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、release/sanitize/bench/SAN bench/native/bridgeの6ログはwarning/error0。focused Release3件/0.76秒（native0.32/C API0.01/Python0.43秒）・SAN2件/1.25秒（native0.93/C API0.31秒）が通過した。Python MatchはAST確認33ケース、native零引数49関数は全てmainへ明示登録を確認した。公式assetsのfresh取得hash/296モデルfactsも一致した。修正C APIの6本×64照会はRelease/SANで `fixed_query_probes==384` の完走を確認した。

94/209用の追加native3局面で、9aliveの対象golden・自己/味方・ヒント非干渉、反射時のRNG/初期launch要約保持と次main消去、最初臼seat/copyと既dead skipを検証した。基底99のboost/Aura/FREE合成・自己被害から復活/cutoff・臼gift1/未選択holder温存、9種の待機改ざん/初期decision0のlaunch要約捏造、手動209/94+Wide unsupported/sampler/late failureも確認した。C/Pythonはfilter→反射の要約保持、自己/他者への臼即時body、合成Fire62、Wideの-5 atomic等を検証した。

以下はseed42の大規模集計。

| 人数 | 試合数 | 意思決定数 | 勝数 | draw | 打切り | checksum |
| --- | --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,851,617 | 49,691 / 50,199 | 110 | 0 | `11374515972360380290` |
| 9 | 10,000 | 2,378,375 | 1,068 / 1,086 / 1,141 / 1,105 / 1,049 / 1,152 / 1,130 / 1,130 / 1,120 | 18 | 1 | `17909990660097319005` |

| 人数 | 杵launch | 臼誘引 / 自己選択 / 味方選択 |
| --- | --- | --- |
| 2 | 7,400 | 559 / 3,757 / 0 |
| 9 | 4,471 | 1,076 / 795 / 0 |

新4集計はlaunch選択の件数で、実被害/終端解決の件数ではない。臼誘引と自己/味方選択は重なり得るが、自己と味方は別分類。本ベンチは全員Soloなので味方選択は0であり、味方を候補から除く規約という意味ではない。臼自動消費はphysical_usedへ混ぜない。

| 人数 | 復活発動数 | 202使用 / 最終解決 | heal / damage解決 | 202実HP増 / 減 |
| --- | --- | --- | --- | --- |
| 2 | 5,547 | 10,820 / 10,820 | 5,581 / 5,239 | 55,810 / 50,143 |
| 9 | 5,491 | 7,765 / 7,765 | 3,852 / 3,913 | 38,514 / 37,031 |

| 人数 | 232使用 / 完了frame | 14使用 / 完了frame | 58使用 / 全支払いMP / 基底MP |
| --- | --- | --- | --- |
| 2 | 9,498 / 9,562 | 7,230 / 14,300 | 7,259 / 84,561 / 83,393 |
| 9 | 7,785 / 42,910 | 4,407 / 9,214 | 4,377 / 53,192 / 52,382 |

復活はnative累計差分、202の実HP増減は復活healを除くbody値。wide/repeat完了frameはblock/完全防御0を含み14+232では両集計へ入る。58の支払いはMC費用込み、基底MPは費用後残額。新dangerと既存randomHP/revivals/mana/removal/wide/repeatの上限を確認した。

| 人数 | chance攻撃 | chance実draw / hit / miss / forced | counter生成 / 開始 | counter実draw / hit / miss / forced | 反射 / bounce / 消去 / 遮断 | 最大redirect深度 |
| --- | --- | --- | --- | --- | --- | --- |
| 2 | 112,643 | 107,634 / 61,761 / 45,873 / 5,009 | 21,455 / 20,568 | 3,440 / 2,581 / 859 / 136 | 18,563 / 27,598 / 16,819 / 9,494 | 5 |
| 9 | 43,805 | 164,747 / 94,695 / 70,052 / 11,163 | 15,150 / 14,534 | 2,374 / 1,786 / 588 / 162 | 14,688 / 21,703 / 14,143 / 9,988 | 6 |

| 人数 | 205解決 / token除去 | 206解決 / token除去 | 閃光追加 / 解除 / 応答 | 暗雲追加 / 解除 |
| --- | --- | --- | --- | --- |
| 2 | 5,449 / 15,926 | 5,376 / 1,145 | 12,078 / 3,021 / 42,267 | 14,787 / 1,470 |
| 9 | 3,942 / 11,731 | 3,888 / 1,531 | 9,010 / 3,362 / 45,085 | 10,818 / 1,672 |

| 人数 | MP反撃解決 / 実増分 | CP反撃解決 / 名目額 | 吸収正解決 / 名目HP | 同被害正解決 / 名目HP |
| --- | --- | --- | --- | --- |
| 2 | 3,226 / 46,895 | 3,369 / 24,674 | 20,383 / 162,666 | 6,362 / 75,736 |
| 9 | 2,321 / 32,372 | 2,314 / 16,725 | 16,962 / 118,044 | 3,948 / 45,866 |

| 人数 | 乱気流 初 / 再 | 壁 初 / 再 | 自己bounce応答 | FREE品使用 main / response | 免除MP | Aura | setter |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2 | 871 / 269 | 3,815 / 878 | 13,921 | 19,509 / 15,085 | 80,503 | 7,160 | 18,783 |
| 9 | 1,393 / 417 | 2,928 / 1,332 | 3,756 | 15,078 / 11,593 | 68,814 | 6,447 | 11,208 |

各人数の1 worker/4 workersでworker数・時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。Release/SAN seed42各1,000試合でも時間以外の全fieldが一致し、checksumは2人 `18068382678944874926`、9人 `8193855328192109680`。観測最大redirect深度5/6は機能上の連鎖上限ではない。

競合を避けた独立Release・1 workerは、2人100,000試合が2.43255秒（1,583,370意思決定/秒）、9人10,000試合が2.5677秒（926,266意思決定/秒）。baselineと時間以外の全fieldが一致した。合法手sampler・実行・reset・各試合の終了snapshot検証込みで、合成単体・Python学習・モデル推論/最適化・公式全モデルの速度ではない。プールと方策が異なるため版間の速度改善倍率を主張しない。

### v16の過去の検証と集計

v16（211モデル/重み415）。全Release CTest129件/17.77秒・ASan/UBSan89件/47.15秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、release/sanitize/bench/SAN bench/native/bridgeの6ログはwarning/error0。focused Release3件/0.62秒（native0.08/C API0.01/Python0.53秒）・SAN2件/1.21秒（native0.90/C API0.31秒）が通過した。Python MatchはAST確認31ケース、native零引数46関数は全てmainへ明示登録を確認した。公式assetsのfresh取得hash/296モデルfactsも一致した。

v16検証範囲の訂正：C APIの固定seed照会6本の開始値を誤って275に変更し、条件seed<64へ入らなかったため、閃光解除・暗雲forced・ホウキ・石けん・MP武器・HP増減の各64回、計384照会がv16では未実行だった。この384件をv16で実行済みとした範囲は撤回する。全Release129件/SAN89件の通過・他assert・native/Pythonの相当照会・大規模計測値は事実として保持する。v17では6loopを `seed{}`（0）へ戻し、`fixed_query_probes==384` の完走必須assertを追加した。Wide+FREE探索も開始275から0へ戻すが、こちらはv16でも275以降で探索を実行していた。v17の修正後Release/SANで384照会完走を再検証して通過した。過去のv16検証範囲を遡って増やさない。

208用の追加native3局面で、通常/Dark/202negativeのHP0救済・curse保持・遅延補充、手動play/response/discard拒否、18枚すべて208の満杯祈り・環境RNG不変・手番進行、既dead不発を検証した。被害実損失5→復活HP10→Saturn反撃10、二回攻撃で二copyの順次消費、53による両HP0の4所持パターン×owner2座席、9種の改ざん拒否、cutoff時gift取消とlate update/gift token枯渇時rollbackも確認した。C/Pythonも救済後の補充・二copy/cutoff・1枠満杯の祈り・不正な手動使用を検証した。CP債務/Wideの208専用native fixtureは無く、共通resolve/finalize_deathsと次対象判定のsource auditとして分離する。

以下はseed42の大規模集計。

| 人数 | 試合数 | 意思決定数 | 勝数 | draw | 打切り | checksum |
| --- | --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,974,476 | 49,387 / 50,468 | 145 | 0 | `6692312305902983439` |
| 9 | 10,000 | 2,456,213 | 1,148 / 1,084 / 1,126 / 1,113 / 1,122 / 1,086 / 1,097 / 1,082 / 1,096 | 44 | 2 | `18168476733015740621` |

| 人数 | 復活発動数 | 202使用 / 最終解決 | heal / damage解決 | 202実HP増 / 減 |
| --- | --- | --- | --- | --- |
| 2 | 5,620 | 11,134 / 11,134 | 5,486 / 5,648 | 54,858 / 54,253 |
| 9 | 5,620 | 8,010 / 8,010 | 4,034 / 3,976 | 40,340 / 37,650 |

復活発動はnative累計のstep差分による実消費数で、所持数やHP0判定回数ではない。208の自動消費は選択使用の `physical_used` へ混ぜず、この復活欄へ別計上する。202分類/実HP増減は復活+10を除くbody値で、negative→復活してnet正でもdamage分類を保つ。resolved=heals+damages、resolved<=plays、gained<=10*heals/lost<=10*damagesと復活数の上限を確認した。

| 人数 | 232使用 / 完了frame | 14使用 / 完了frame | 58使用 / 全支払いMP / 基底MP |
| --- | --- | --- | --- |
| 2 | 9,860 / 9,924 | 7,491 / 14,805 | 7,488 / 87,581 / 86,571 |
| 9 | 8,045 / 44,841 | 4,563 / 9,574 | 4,669 / 58,826 / 57,855 |

wide/repeatの完了frameはblock/完全防御0を含み、14+232では両集計へ入る。58の支払いはMC費用込み、基底MPは費用後残額。既存mana/removal/wide/repeatの上限も確認した。

| 人数 | chance攻撃 | chance実draw / hit / miss / forced | counter生成 / 開始 | counter実draw / hit / miss / forced | 反射 / bounce / 消去 / 遮断 | 最大redirect深度 |
| --- | --- | --- | --- | --- | --- | --- |
| 2 | 117,572 | 112,158 / 64,404 / 47,754 / 5,414 | 21,912 / 20,978 | 3,482 / 2,578 / 904 / 167 | 19,242 / 28,376 / 17,477 / 9,781 | 5 |
| 9 | 45,453 | 170,767 / 98,368 / 72,399 / 11,684 | 15,727 / 15,054 | 2,505 / 1,855 / 650 / 153 | 15,435 / 21,773 / 14,363 / 10,457 | 5 |

| 人数 | 205解決 / token除去 | 206解決 / token除去 | 閃光追加 / 解除 / 応答 | 暗雲追加 / 解除 |
| --- | --- | --- | --- | --- |
| 2 | 5,796 / 16,822 | 5,618 / 1,268 | 12,530 / 3,326 / 44,117 | 15,317 / 1,540 |
| 9 | 4,114 / 12,254 | 4,067 / 1,624 | 9,401 / 3,533 / 47,655 | 11,069 / 1,635 |

| 人数 | MP反撃解決 / 実増分 | CP反撃解決 / 名目額 | 吸収正解決 / 名目HP | 同被害正解決 / 名目HP |
| --- | --- | --- | --- | --- |
| 2 | 3,460 / 50,650 | 3,473 / 25,737 | 21,374 / 169,620 | 6,657 / 79,374 |
| 9 | 2,466 / 34,885 | 2,431 / 17,557 | 17,697 / 122,711 | 4,006 / 46,633 |

| 人数 | 乱気流 初 / 再 | 壁 初 / 再 | 自己bounce応答 | FREE品使用 main / response | 免除MP | Aura | setter |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2 | 898 / 269 | 3,821 / 975 | 14,103 | 20,330 / 15,702 | 86,129 | 7,396 | 19,281 |
| 9 | 1,357 / 374 | 3,025 / 1,349 | 3,709 | 15,595 / 11,940 | 70,933 | 6,734 | 11,551 |

各人数の1 worker/4 workersでworker数・時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。Release/SAN seed42各1,000試合でも時間以外の全fieldが一致し、checksumは2人 `13347754452093966556`、9人 `1870282473665283638`。観測最大redirect深度5は機能上の連鎖上限ではない。

競合を避けた独立Release・1 workerは、2人100,000試合が2.4242秒（1,639,500意思決定/秒）、9人10,000試合が2.75017秒（893,114意思決定/秒）。baselineと時間以外の全fieldが一致した。合法手sampler・実行・reset・各試合の終了snapshot検証込みで、合成単体・Python学習・モデル推論/最適化・公式全モデルの速度ではない。プールと方策が異なるため版間の速度改善倍率を主張しない。

### v15の過去の検証と集計

v15（210モデル/重み414）。全Release CTest129件/18.87秒・ASan/UBSan89件/49.59秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、release/sanitize/bench/SAN bench/native/bridge-fixの6ログはwarning/error0。focused Release3件/0.63秒（native0.08/C API0.01/Python0.53秒）・SAN2件/1.00秒（native0.68/C API0.32秒）が通過した。Python MatchはAST確認29ケース、native零引数43関数は全てmainへ明示登録を確認した。公式assetsのfresh取得hash/296モデルfactsも一致した。

202用の追加native3局面で、両符号・一度のlaunch draw・self即時/foreign応答・HPclip/死亡/自然終局優先を検証した。万能反射往復で符号とRNGが不変、signed現在値が公開される一方でanchorは非公開、9種のjournal改ざんをatomicに拒否することを確認した。legal照会が未来RNGを消費しないこと、cutoffで未解決body/giftを取消すこと、launch/body/後段giftでの失敗時rollbackも検証した。DarkCloud非干渉は202固有fixtureではなく、非chanceのlaunch/factory/body経路をsource auditして確認した。

以下はseed42の大規模集計。

| 人数 | 試合数 | 意思決定数 | 勝数 | draw | 打切り | checksum |
| --- | --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,889,410 | 49,760 / 50,115 | 125 | 0 | `16376680674247028005` |
| 9 | 10,000 | 2,403,179 | 1,151 / 1,129 / 1,119 / 1,067 / 1,111 / 1,159 / 1,079 / 1,081 / 1,084 | 20 | 0 | `8098081484010360106` |

| 人数 | 202使用 / 最終解決 | heal / damage解決 | 実HP増 / 減 |
| --- | --- | --- | --- |
| 2 | 10,938 / 10,938 | 5,513 / 5,425 | 55,130 / 52,236 |
| 9 | 8,180 / 8,180 | 4,073 / 4,107 | 40,728 / 38,842 |

heal/damageは完了した符号の分類で、実HP増減とは異なる。99で実増0でもheal解決に数え、実増減はclip後の合計。resolved=heals+damages、resolved<=plays、gained<=10*heals/lost<=10*damagesを確認した。これは50/50という実験規約の実行結果で、公式serverの確率測定ではない。

| 人数 | 232使用 / 完了frame | 14使用 / 完了frame | 58使用 / 全支払いMP / 基底MP |
| --- | --- | --- | --- |
| 2 | 9,666 / 9,729 | 7,448 / 14,745 | 7,337 / 85,485 / 84,486 |
| 9 | 8,113 / 44,638 | 4,519 / 9,266 | 4,478 / 55,510 / 54,646 |

wide/repeatの完了frameはblock/完全防御0を含み、14+232では両集計へ入る。58の支払いはMC費用込み、基底MPは費用後残額。既存mana/removal/wide/repeatの上限も確認した。

| 人数 | chance攻撃 | chance実draw / hit / miss / forced | counter生成 / 開始 | counter実draw / hit / miss / forced | 反射 / bounce / 消去 / 遮断 | 最大redirect深度 |
| --- | --- | --- | --- | --- | --- | --- |
| 2 | 113,987 | 108,747 / 62,776 / 45,971 / 5,240 | 21,546 / 20,724 | 3,463 / 2,667 / 796 / 137 | 19,150 / 27,845 / 17,147 / 9,683 | 4 |
| 9 | 43,858 | 163,737 / 93,339 / 70,398 / 11,060 | 15,340 / 14,734 | 2,371 / 1,790 / 581 / 139 | 15,274 / 21,707 / 14,481 / 10,168 | 4 |

| 人数 | 205解決 / token除去 | 206解決 / token除去 | 閃光追加 / 解除 / 応答 | 暗雲追加 / 解除 |
| --- | --- | --- | --- | --- |
| 2 | 5,674 / 16,561 | 5,452 / 1,190 | 12,063 / 3,204 / 42,266 | 15,007 / 1,478 |
| 9 | 4,141 / 12,277 | 4,031 / 1,604 | 9,020 / 3,393 / 45,041 | 10,958 / 1,686 |

| 人数 | MP反撃解決 / 実増分 | CP反撃解決 / 名目額 | 吸収正解決 / 名目HP | 同被害正解決 / 名目HP |
| --- | --- | --- | --- | --- |
| 2 | 3,352 / 49,538 | 3,431 / 25,050 | 20,918 / 165,711 | 6,566 / 78,411 |
| 9 | 2,293 / 32,895 | 2,487 / 17,633 | 17,142 / 120,498 | 3,885 / 45,206 |

| 人数 | 乱気流 初 / 再 | 壁 初 / 再 | 自己bounce応答 | FREE品使用 main / response | 免除MP | Aura | setter |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2 | 803 / 224 | 3,854 / 956 | 13,930 | 19,933 / 15,601 | 84,336 | 7,424 | 19,012 |
| 9 | 1,327 / 429 | 3,028 / 1,376 | 3,775 | 15,074 / 11,821 | 68,316 | 6,511 | 11,502 |

各人数の1 worker/4 workersでworker数・時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。Release/SAN seed42各1,000試合でも時間以外の全fieldが一致し、checksumは2人 `3640146110508428199`、9人 `12830718056704997050`。観測最大redirect深度4は機能上の連鎖上限ではない。

競合を避けた独立Release・1 workerは、2人100,000試合が2.49299秒（1,560,140意思決定/秒）、9人10,000試合が2.65853秒（903,949意思決定/秒）。baselineと時間以外の全fieldが一致した。合法手sampler・実行・reset・各試合の終了snapshot検証込みで、合成単体・Python学習・モデル推論/最適化・公式全モデルの速度ではない。プールと方策が異なるため版間の速度改善倍率を主張しない。

### v14の過去の検証と集計

v14（209モデル/重み412）。全Release CTest129件/19.09秒・ASan/UBSan89件/50.23秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、Release/SAN/bench/SAN bench/bridgeの5ログはwarning/error0。focused Release3件/0.81秒（native0.32/C API0.01/Python0.47秒）・SAN2件/1.17秒（native0.83/C API0.34秒）が通過した。Python MatchはAST確認27ケース、native零引数40関数は全てmainへ明示登録を確認した。公式assetsのfresh取得hash/296モデルfactsも一致した。

追加native局面で、任意alive target指定の非干渉・unknown/dead拒否、複数232の保持/gift、0MPのFREE組合せsampler、58の有料/無料追加後の不変威力を検証した。9人の14+232では16frameの全step restore・各回fresh属性・暗雲でも無chance drawを確認し、counter FIFO・owner死亡取消・二巡目dead skip・cutoff・改ざん6例・後段更新失敗の完全rollbackも検証した。

以下はseed42の大規模集計。

| 人数 | 試合数 | 意思決定数 | 勝数 | draw | 打切り | checksum |
| --- | --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,868,837 | 49,849 / 50,032 | 119 | 0 | `7280126839917179986` |
| 9 | 10,000 | 2,392,134 | 1,105 / 1,102 / 1,109 / 1,125 / 1,107 / 1,109 / 1,135 / 1,093 / 1,092 | 23 | 0 | `11171425924432160273` |

| 人数 | 232使用 / 完了frame | 14使用 / 完了frame | 58使用 / 全支払いMP / 基底MP |
| --- | --- | --- | --- |
| 2 | 9,727 / 9,793 | 7,389 / 14,631 | 7,411 / 87,390 / 86,255 |
| 9 | 8,167 / 45,210 | 4,587 / 9,563 | 4,506 / 55,197 / 54,345 |

`wide_plays` は232を含む主動作数で、複数232でも1。完了frameは非counterの初期payloadが終端応答で完了した数で、block/完全防御0を含み、未完了cutoffを除く。14+232のframeはwide/repeatの両集計へ入り、互いに排他的な件数ではない。14完了の上限は2*(人数−1)*主動作数へ広がり、単体の2*主動作数を上限として使わない。58の支払いはMC費用込み、基底MPは完成列費用後の残額。

| 人数 | chance攻撃 | chance実draw / hit / miss / forced | counter生成 / 開始 | counter実draw / hit / miss / forced | 反射 / bounce / 消去 / 遮断 | 最大redirect深度 |
| --- | --- | --- | --- | --- | --- | --- |
| 2 | 114,312 | 109,170 / 62,768 / 46,402 / 5,142 | 21,879 / 20,976 | 3,538 / 2,661 / 877 / 154 | 19,048 / 28,294 / 17,311 / 9,906 | 4 |
| 9 | 43,901 | 164,811 / 94,751 / 70,060 / 11,754 | 15,630 / 14,988 | 2,527 / 1,913 / 614 / 122 | 15,304 / 22,379 / 14,508 / 10,405 | 4 |

| 人数 | 205解決 / token除去 | 206解決 / token除去 | 閃光追加 / 解除 / 応答 | 暗雲追加 / 解除 |
| --- | --- | --- | --- | --- |
| 2 | 5,525 / 16,142 | 5,529 / 1,172 | 12,094 / 3,071 / 41,593 | 15,185 / 1,519 |
| 9 | 4,179 / 12,430 | 3,904 / 1,581 | 9,030 / 3,382 / 44,579 | 11,249 / 1,807 |

| 人数 | MP反撃解決 / 実増分 | CP反撃解決 / 名目額 | 吸収正解決 / 名目HP | 同被害正解決 / 名目HP |
| --- | --- | --- | --- | --- |
| 2 | 3,510 / 51,126 | 3,372 / 24,185 | 20,788 / 163,888 | 6,391 / 76,321 |
| 9 | 2,418 / 33,791 | 2,363 / 16,400 | 17,603 / 121,765 | 3,968 / 46,287 |

| 人数 | 乱気流 初 / 再 | 壁 初 / 再 | 自己bounce応答 | FREE品使用 main / response | 免除MP | Aura | setter |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2 | 876 / 277 | 3,894 / 968 | 14,074 | 20,082 / 15,699 | 84,537 | 7,325 | 19,230 |
| 9 | 1,372 / 461 | 3,072 / 1,402 | 3,702 | 15,417 / 11,930 | 70,178 | 6,654 | 11,554 |

各人数の1 worker/4 workersでworker数・時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。観測最大redirect深度4は機能上の連鎖上限ではない。

Release/SAN seed42各1,000試合でも時間以外の全fieldが一致し、checksumは2人 `9992836881741631580`、9人 `4056226161425745428`。

競合を避けた独立Release・1 workerは、2人100,000試合が2.45876秒（1,573,490意思決定/秒）、9人10,000試合が2.69532秒（887,513意思決定/秒）。baselineと時間以外の全fieldが一致した。合法手sampler・実行・reset・各試合の終了snapshot検証込みで、合成単体・Python学習・モデル推論/最適化・公式全モデルの速度ではない。プールと方策が異なるため版間の速度改善倍率を主張しない。

### v13の過去の検証と集計

v13（208モデル/重み411）。全Release CTest129件/20.39秒・ASan/UBSan89件/51.58秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、focused Release3件/0.94秒（native0.38/C API0.01/Python0.55秒）・SAN2件/1.52秒（native1.17/C API0.35秒）が通過した。Python MatchはAST確認25ケース、native零引数37関数は全てmainへ明示登録を確認した。最終2buildと前段bridge/bench3ログのwarning/errorは0。公式assetsのfresh取得hash/296モデルfactsも一致した。

`./build/godfield_official_bench GAMES PLAYERS 42 WORKERS` のseed42集計:

| 人数 | 試合数 | 意思決定数 | 勝数 | draw | 打切り | checksum |
| --- | --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,862,744 | 49,657 / 50,222 | 121 | 0 | `2365312122692827753` |
| 9 | 10,000 | 2,474,625 | 1,108 / 1,104 / 1,126 / 1,066 / 1,113 / 1,139 / 1,097 / 1,141 / 1,083 | 22 | 1 | `9384306151148000397` |

| 人数 | 58主動作 / 全支払いMP / 基底MP | 14主動作 / 完了frame | 自己宛攻撃の主動作 |
| --- | --- | --- | --- |
| 2 | 7,394 / 86,103 / 85,307 | 7,322 / 14,489 | 549,790 |
| 9 | 4,797 / 61,321 / 60,719 | 4,761 / 9,382 | 129,634 |

58の全支払いMPはMC費用を含み、基底MPは完成列費用後の残MP。14完了frameはblock/全防御0も含み、被害hit数ではない。自己宛攻撃の主動作は14でも1、ATK0の58も含む。

| 人数 | chance攻撃 | chance実draw / hit / miss / forced | counter生成 / 開始 | counter実draw / hit / miss / forced | 反射 / bounce / 消去 / 遮断 | 最大redirect深度 |
| --- | --- | --- | --- | --- | --- | --- |
| 2 | 114,699 | 109,497 / 62,894 / 46,603 / 5,202 | 21,939 / 20,990 | 3,460 / 2,538 / 922 / 129 | 19,348 / 28,558 / 17,234 / 9,807 | 5 |
| 9 | 47,287 | 178,922 / 103,074 / 75,848 / 12,359 | 15,444 / 14,750 | 2,516 / 1,855 / 661 / 159 | 15,367 / 21,743 / 14,714 / 10,666 | 4 |

| 人数 | 205解決 / token除去 | 206解決 / token除去 | 閃光追加 / 解除 / 応答 | 暗雲追加 / 解除 |
| --- | --- | --- | --- | --- |
| 2 | 5,352 / 15,634 | 5,388 / 1,173 | 12,386 / 3,213 / 42,604 | 15,092 / 1,602 |
| 9 | 4,241 / 12,600 | 4,103 / 1,569 | 9,482 / 3,629 / 46,700 | 11,198 / 1,693 |

| 人数 | MP反撃解決 / 実増分 | CP反撃解決 / 名目額 | 吸収正解決 / 名目HP | 同被害正解決 / 名目HP |
| --- | --- | --- | --- | --- |
| 2 | 3,466 / 50,562 | 3,378 / 24,497 | 20,704 / 164,832 | 6,598 / 77,966 |
| 9 | 2,435 / 34,647 | 2,267 / 15,811 | 18,310 / 124,249 | 4,103 / 47,615 |

| 人数 | 乱気流 初 / 再 | 壁 初 / 再 | 自己bounce応答 | FREE品使用 main / response | 免除MP | Aura | setter |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2 | 842 / 264 | 3,862 / 908 | 14,167 | 19,708 / 15,722 | 84,421 | 7,495 | 19,045 |
| 9 | 1,455 / 452 | 3,073 / 1,355 | 3,757 | 15,663 / 11,794 | 70,446 | 6,743 | 11,724 |

1 worker/4 workersでworker数・時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。Release/SAN seed42各1,000試合も時間以外の全fieldが一致し、checksumは2人 `14212402720306530348`、9人 `15651691549885442780`。観測最大redirect深度は機能上の連鎖上限ではない。

競合を避けた独立Release・1 workerは、2人100,000試合が2.36604秒（1,632,580意思決定/秒）、9人10,000試合が2.75418秒（898,499意思決定/秒）。baselineと時間以外の全fieldが一致した。合法手sampler・実行・reset・各試合の終了snapshot検証込みで、合成単体・Python学習・モデル推論/最適化・公式全モデルの速度ではない。プールと方策が異なるため版間の速度改善倍率を主張しない。

### v12の過去の検証と集計

v12（206モデル/重み409）は全Release CTest129件/18.63秒、ASan/UBSan89件/49.95秒が通過した（各suiteの `-j4` 同時実行）。focusedはRelease3件/0.58秒（native0.30/C API0.01/Python0.27秒）・SAN2件/1.28秒（native1.01/C API0.27秒）が通過した。全source/test最終freeze後の最終2full build/全検証で、最終2buildと前段bridge/bench3ログのwarning/errorは0。Python Match suiteはAST確認23ケース、nativeの零引数テスト全34関数をmainへ明示登録したことも確認した。公式assetsのfresh取得hashと296モデルfactsも一致した。

`./build/godfield_official_bench GAMES PLAYERS 42 WORKERS` のv12集計:

| 人数 | 試合数 | 意思決定数 | 勝数 | draw | 打切り | checksum |
| ---: | ---: | ---: | --- | ---: | ---: | --- |
| 2 | 100,000 | 3,942,252 | 49,913 / 49,954 | 133 | 0 | `6915174992826563900` |
| 9 | 10,000 | 2,511,295 | 1,105 / 1,094 / 1,142 / 1,025 / 1,096 / 1,096 / 1,107 / 1,175 / 1,138 | 20 | 2 | `2152079972708342754` |

| 人数 | 205解決 / 実token除去 | 206解決 / 実token除去 | 閃光追加 / 解除 / 応答 | 暗雲追加 / 解除 |
| ---: | --- | --- | --- | --- |
| 2 | 5,402 / 15,724 | 5,526 / 1,131 | 12,727 / 3,380 / 45,039 | 15,642 / 1,589 |
| 9 | 4,327 / 12,845 | 4,324 / 1,745 | 9,473 / 3,630 / 47,749 | 11,661 / 1,866 |

除去解決数はeligible0を含み、実除去は上限205で3×解決数、206で2×解決数以下を確認した。実token消失は使用品consumeを除き、同modelの別コピーを個別に数え、後続giftで相殺しない。curseの追加/解除は有無の遷移、閃光中応答はpassを含む。これらはベンチ内部集計で公開observeへ追加しない。

| 人数 | chance攻撃 | chance実draw / hit / miss / forced | counter生成 / 開始 | counter実draw / hit / miss / forced | 反射 / bounce / 消去 / 遮断 | 最大redirect深度 |
| ---: | ---: | --- | --- | --- | --- | ---: |
| 2 | 119,063 | 113,588 / 65,605 / 47,983 / 5,475 | 21,882 / 20,994 | 3,517 / 2,661 / 856 / 159 | 19,449 / 28,542 / 17,591 / 10,026 | 5 |
| 9 | 48,617 | 185,056 / 106,400 / 78,656 / 13,364 | 15,620 / 14,999 | 2,480 / 1,880 / 600 / 149 | 15,485 / 22,241 / 15,028 / 11,043 | 5 |

| 人数 | MP反撃解決 / 実増分 | CP反撃解決 / 名目額 | 吸収正解決 / 名目HP | 同被害正解決 / 名目HP |
| ---: | --- | --- | --- | --- |
| 2 | 3,460 / 50,252 | 3,397 / 24,774 | 21,364 / 169,681 | 6,567 / 77,260 |
| 9 | 2,365 / 33,056 | 2,349 / 16,303 | 18,675 / 128,429 | 4,039 / 45,969 |

| 人数 | 乱気流 初 / 再 | 壁 初 / 再 | 自己bounce応答 | FREE品使用 main / response | 免除MP | Aura | setter |
| ---: | --- | --- | ---: | --- | ---: | ---: | ---: |
| 2 | 898 / 293 | 3,913 / 961 | 14,255 | 20,309 / 15,900 | 87,010 | 7,621 | 19,193 |
| 9 | 1,633 / 564 | 3,002 / 1,407 | 3,882 | 16,133 / 11,868 | 74,219 | 7,171 | 11,944 |

1 worker/4 workersでworker数・時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。観測最大redirect深度5は機能上の連鎖上限ではない。Release/SAN seed42各1,000試合も時間以外の全fieldが一致し、checksumは2人 `11958536962789411924`、9人 `6514456865413471030`。競合ログの秒数を独立速度として使わず、版間の速度比を改善率として主張しない。

競合を避けたRelease・1 workerの再計測は、2人100,000試合が2.37831秒（1,657,590意思決定/秒）、9人10,000試合が2.67775秒（937,838意思決定/秒）。時間以外の全fieldは上表と一致した。合法手sampler、実行、reset、各試合の終了snapshot検証を含み、合成単体・Python学習・モデル推論/最適化・公式全モデルの速度ではない。scope・policy・プールが異なるため、版間の速度比を改善率として主張しない。

### v11の過去の検証と集計

v11（204モデル/重み407）は全Release CTest129件/20.55秒、ASan/UBSan89件/52.21秒が通過した。focusedはRelease3件/0.65秒（native0.09/C API0.23/Python0.33秒）・SAN2件/0.88秒（native0.56/C API0.32秒）が通過した。全source/test最終freeze後のbuild/full検証を行い、4buildログでwarning/errorは0、Python Match suiteはAST確認21ケース。公式assetsのfresh取得hashと296モデルfactsも一致した。

native mainに全31関数の呼出しがあることを別途確認し、v10で未登録だったFlash2関数を今回初めて実行した。fixture修正無しでRelease/SAN双方を通過した。新しい暗雲混合/forced4関数も通過し、GNU/Clangのunused-function guardは実compile flagsとnegative probeで有効性を確認した。v10の過去の検証範囲を遡って増やす扱いにはしない。

`./build/godfield_official_bench GAMES PLAYERS 42 WORKERS` のv11集計:

| 人数 | 試合数 | 意思決定数 | 勝数 | draw | 打切り | checksum |
| ---: | ---: | ---: | --- | ---: | ---: | --- |
| 2 | 100,000 | 3,940,299 | 49,555 / 50,307 | 138 | 0 | `11534437455040832945` |
| 9 | 10,000 | 2,519,780 | 1,089 / 1,056 / 1,159 / 1,112 / 1,118 / 1,068 / 1,102 / 1,149 / 1,118 | 29 | 0 | `5814487950518364984` |

| 人数 | 閃光追加 / 解除 | 閃光中応答 | 暗雲追加 / 解除 | 初期chance forced | counter forced |
| ---: | --- | ---: | --- | ---: | ---: |
| 2 | 12,679 / 3,364 | 43,975 | 15,714 / 1,683 | 5,489 | 176 |
| 9 | 9,925 / 3,830 | 49,959 | 11,595 / 1,927 | 13,207 | 156 |

| 人数 | chance攻撃 | chance実draw / hit / miss | counter生成 / 開始 | counter実draw / hit / miss | 反射 / bounce / 消去 / 遮断 | 最大redirect深度 |
| ---: | ---: | --- | --- | --- | --- | ---: |
| 2 | 119,404 | 113,915 / 65,400 / 48,515 | 22,558 / 21,616 | 3,603 / 2,687 / 916 | 19,818 / 29,228 / 17,932 / 10,315 | 5 |
| 9 | 49,320 | 186,845 / 107,246 / 79,599 | 16,010 / 15,345 | 2,562 / 1,927 / 635 | 15,801 / 22,849 / 15,382 / 11,344 | 6 |

実draw hitとforcedは別であり、上表のmissはdraw−hit。forcedは無drawの命中活性化数であって被害解決数ではなく、cutoffで被害未実行の記録を含み得る。curse追加/解除は有無の遷移だけ、再付与や空解除は除く。閃光中応答はpassも含む。これらの累計は公開observeへ出さない。

| 人数 | MP反撃解決 / 実増分 | CP反撃解決 / 名目額 | 吸収正解決 / 名目HP | 同被害正解決 / 名目HP |
| ---: | --- | --- | --- | --- |
| 2 | 3,502 / 50,292 | 3,522 / 25,478 | 21,581 / 171,023 | 6,754 / 79,379 |
| 9 | 2,435 / 34,050 | 2,407 / 17,103 | 19,126 / 132,275 | 4,124 / 47,068 |

| 人数 | 乱気流 初 / 再 | 壁 初 / 再 | 自己bounce応答 | FREE品使用 main / response | 免除MP | Aura | setter |
| ---: | --- | --- | ---: | --- | ---: | ---: | ---: |
| 2 | 898 / 268 | 4,054 / 980 | 14,665 | 20,730 / 16,038 | 88,271 | 7,969 | 20,034 |
| 9 | 1,627 / 609 | 3,067 / 1,498 | 3,986 | 16,561 / 12,164 | 75,911 | 7,471 | 12,124 |

1 worker/4 workersでworker数・時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。観測最大redirect深度5/6は機能上の連鎖上限ではない。Release/SAN seed42各1,000試合も時間以外の全fieldが一致し、checksumは2人 `17832090841414215461`、9人 `15509475549777632243`。競合ログの秒数を独立速度として使わず、版間の速度比を改善率として主張しない。

競合を避けたRelease・1 workerの再計測は、2人100,000試合が2.46388秒（1,599,230意思決定/秒）、9人10,000試合が2.75792秒（913,652意思決定/秒）。時間以外の全fieldは上表と一致した。合法手sampler、実行、reset、各試合の終了snapshot検証を含み、合成単体・Python学習・モデル推論/最適化・公式全モデルの速度ではない。scope・policy・プールが異なるため、版間の速度比を改善率として主張しない。

### v10の過去の検証と集計

v10は全Release CTest129件/20.43秒、ASan/UBSan89件/52.20秒が通過した（各suiteを `-j4` で同時実行）。focusedはRelease3件/0.70秒・SAN2件/1.37秒、Python Matchは18ケース。最終source/test freeze後のbuild/full検証でwarning/errorは0、公式assetsのfresh取得hashと296モデルfactsも一致した。

検証範囲の訂正：v11監査で、v10専用native関数 `flash_damage_selection_and_cures` / `flash_counter_chance_and_journal` が定義のみでmainへ未登録だったことが判明した。したがって、この2関数が意図したcounter FIFO・Flash固有のcanonical偽造・late rollback等をv10で実行済みとした記述は撤回する。上記CTest件数・C API/Python18ケースの公開Flash経路・大規模benchの記録値は実行済みであり維持する。v11で登録を修正して初実行し、全source/test最終freeze後の検証でもfixture修正無しでRelease/SANとも通過した。 GNU/Clangのofficial_match native test targetへ `-Wunused-function -Werror=unused-function` を追加し、同様の未使用テスト関数をcompile errorにする。

`./build/godfield_official_bench GAMES PLAYERS 42 WORKERS` のv10集計:

| 人数 | 試合数 | 意思決定数 | 勝数 | draw | 打切り | checksum |
| ---: | ---: | ---: | --- | ---: | ---: | --- |
| 2 | 100,000 | 3,942,821 | 49,625 / 50,233 | 142 | 0 | `8101395066724713421` |
| 9 | 10,000 | 2,506,639 | 1,087 / 1,154 / 1,115 / 1,144 / 1,047 / 1,068 / 1,114 / 1,123 / 1,126 | 21 | 1 | `17246129150498610966` |

| 人数 | 閃光追加 | 閃光解除 | 閃光中の応答 | counter生成 / 開始 | chance抽選 / hit / miss |
| ---: | ---: | ---: | ---: | --- | --- |
| 2 | 12,556 | 3,349 | 43,311 | 18,782 / 17,816 | 121,191 / 69,527 / 51,664 |
| 9 | 9,761 | 3,846 | 47,748 | 13,552 / 12,841 | 202,829 / 116,362 / 86,467 |

閃光追加/解除は有無の遷移を数え、再付与や空解除を除く。閃光中の応答はpassも含む。これらの内部集計を公開observeへ出さない。1 worker/4 workersはworker数・時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。Release/SAN seed42各1,000試合も時間以外の全fieldが一致し、checksumは2人 `18308359105672864432`、9人 `5134795071147902293`。

競合を避けたRelease・1 workerでは、2人100,000試合が2.44296秒（1,613,950意思決定/秒）、9人10,000試合が2.64504秒（947,677意思決定/秒）で上表と時間以外の全fieldが一致した。合法手sampler、reset、step、終了snapshot検証を含み、学習/推論や公式全モデルの速度ではない。版間の速度改善比は主張しない。

### v9の過去の検証と集計

v9（194モデル/重み381）は全Release CTest129件/20.20秒、ASan/UBSan89件/51.81秒が通過した。focusedはRelease3件/1.13秒（native0.52/C API0.20/Python0.40秒）・SAN2件/1.33秒（native1.02/C API0.31秒）が通過した。全source/bridge/test最終freeze後のbuild/full検証でwarning/errorは0、Python Match suiteはAST確認16ケース。公式assetsのfresh取得SHA-256と296モデルfactsも一致した。

`./build/godfield_official_bench GAMES PLAYERS 42 WORKERS` のv9集計:

| 人数 | 試合数 | 意思決定数 | 勝数 | draw | 打切り | checksum |
| ---: | ---: | ---: | --- | ---: | ---: | --- |
| 2 | 100,000 | 3,734,360 | 49,634 / 50,224 | 142 | 0 | `8957456254725946378` |
| 9 | 10,000 | 2,361,580 | 1,123 / 1,143 / 1,101 / 1,132 / 1,128 / 1,068 / 1,065 / 1,082 / 1,143 | 15 | 0 | `12174732045177881378` |

| 人数 | 正の吸収解決 | 吸収raw残余合計 | 正の同被害解決 | 同被害raw残余合計 | MP反撃解決 / 実MP増分 | CP反撃解決 / 名目CP |
| ---: | ---: | ---: | ---: | ---: | --- | --- |
| 2 | 22,464 | 180,643 | 6,922 | 81,885 | 3,756 / 54,052 | 3,690 / 26,463 |
| 9 | 19,382 | 136,285 | 4,180 | 47,877 | 2,581 / 35,820 | 2,601 / 17,958 |

吸収/同被害の名目HPは普通DEF後raw残余で、clipした実回復・source/targetの実損失とは異なる。正nonDark解決だけを数え、miss/block/redirect時点/残余0は除外する。MP反撃は99cap後の実増分、CP反撃は不足連鎖とsource cap前の名目請求額を記録する。これらは特権ベンチ集計であり、公開observeへ追加しない。

| 人数 | chance攻撃 | chance抽選 / hit / miss | counter生成 / 開始 | counter抽選 / hit / miss | 反射 | bounce | 消去 | 遮断 | 最大redirect深度 |
| ---: | ---: | --- | --- | --- | ---: | ---: | ---: | ---: | ---: |
| 2 | 118,484 | 118,484 / 70,065 / 48,419 | 15,364 / 14,369 | 3,955 / 2,996 / 959 | 21,236 | 31,674 | 19,308 | 10,869 | 5 |
| 9 | 48,815 | 196,348 / 115,869 / 80,479 | 10,934 / 10,164 | 2,905 / 2,158 / 747 | 16,554 | 24,248 | 16,444 | 11,724 | 5 |

| 人数 | 乱気流 初 / 再 | 壁 初 / 再 | 自己bounce応答 | FREE品使用 main / response | 免除MP | Aura | setter |
| ---: | --- | --- | ---: | --- | ---: | ---: | ---: |
| 2 | 943 / 294 | 4,420 / 1,160 | 15,700 | 20,254 / 18,166 | 91,694 | 8,984 | 20,990 |
| 9 | 1,628 / 531 | 3,535 / 1,800 | 4,250 | 16,140 / 13,940 | 77,831 | 8,662 | 12,770 |

1 worker/4 workersでworker数・時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。観測最大redirect深度5は機能上の連鎖上限ではない。Release/SAN seed42各1,000試合も時間以外の全fieldが一致し、checksumは2人 `2715841436232270123`、9人 `13089925563403552514`。競合ログの秒数を独立速度として使わず、版間の速度比を改善率として主張しない。

競合を避けたRelease・1 workerの再計測は、2人100,000試合が2.34078秒（1,595,350意思決定/秒）、9人10,000試合が2.509秒（941,242意思決定/秒）。時間以外の全fieldは上表と一致した。合法手sampler、実行、reset、各試合の終了snapshot検証を含み、合成単体・Python学習・モデル推論/最適化・公式全モデルの速度ではない。scope・policy・プールが異なるため、版間の速度比を改善率として主張しない。

### v8の過去の検証と集計

v8（189モデル/重み376）は全Release CTest129件/19.78秒、ASan/UBSan89件/51.38秒が通過した。focusedもRelease3件/1.32秒・SAN2件/1.32秒が通過した。全ソース最終freeze後のbuild/full検証でcompile warning/errorは0、Python Match suiteはAST確認15ケース。公式assets再取得のSHA-256と296モデルfactsも一致した。

`./build/godfield_official_bench GAMES PLAYERS 42 WORKERS` のv8集計:

| 人数 | 試合数 | 意思決定数 | 勝数 | draw | 打切り | checksum |
| ---: | ---: | ---: | --- | ---: | ---: | --- |
| 2 | 100,000 | 3,776,364 | 49,937 / 50,063 | 0 | 0 | `2033392899257200488` |
| 9 | 10,000 | 2,380,991 | 1,146 / 1,179 / 1,077 / 1,078 / 1,114 / 1,080 / 1,113 / 1,068 / 1,144 | 0 | 1 | `11415801088495861110` |

| 人数 | MP反撃解決 | MP実増分 | CP反撃解決 | CP名目請求額 | counter生成 / 開始 | counter抽選 / hit / miss |
| ---: | ---: | ---: | ---: | ---: | --- | --- |
| 2 | 3,682 | 53,711 | 3,821 | 26,935 | 15,424 / 14,402 | 3,930 / 2,925 / 1,005 |
| 9 | 2,617 | 36,262 | 2,575 | 17,886 | 10,877 / 10,129 | 2,845 / 2,115 / 730 |

`mp_counter_gained` はMP99cap後の実増分であり、2倍の名目量ではない。`cp_counter_nominal` は請求の名目額で、対象のCP実損失やsourceのCP実増分とは異なる。CP不足はMP/HPへ波及し、source側creditは99でclipする。counter抽選/hit/missは181だけの命中判定、生成/開始は資源counterを含む全4種を数える。これらは特権ベンチ集計で、公開observeへ出さない。

| 人数 | chance攻撃 | chance抽選 / hit / miss | 反射 | bounce | 属性消去 | 遮断 | 最大redirect深度 |
| ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| 2 | 116,636 | 116,636 / 67,752 / 48,884 | 21,379 | 31,619 | 19,482 | 10,761 | 5 |
| 9 | 47,853 | 194,851 / 113,573 / 81,278 | 17,031 | 24,066 | 16,922 | 11,778 | 5 |

| 人数 | 乱気流 初 / 再 | 壁 初 / 再 | 自己bounce応答 | FREE品使用 main / response | 免除MP | Aura | setter |
| ---: | --- | --- | ---: | --- | ---: | ---: | ---: |
| 2 | 883 / 256 | 4,477 / 1,293 | 15,788 | 20,387 / 18,701 | 86,345 | 9,289 | 21,433 |
| 9 | 1,581 / 497 | 3,735 / 1,911 | 4,197 | 16,331 / 14,373 | 74,797 | 9,141 | 13,406 |

1 worker/4 workersで時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。観測最大redirect深度5は機能上の連鎖上限ではない。Release/SAN seed42各1,000試合も時間以外の全fieldが一致し、checksumは2人 `13623686729798944892`、9人 `4100116851726543187`。

競合を避けたRelease・1 workerの再計測は、2人100,000試合が2.34208秒（1,612,400意思決定/秒）、9人10,000試合が2.46961秒（964,117意思決定/秒）。時間以外の全fieldは上表と一致した。合法手sampler、実行、reset、各試合の終了snapshot検証を含み、合成単体・Python学習・モデル推論/最適化・公式全モデルの速度ではない。scope・policy・プールが異なるため、版間の速度比を改善率として主張しない。

### v7の過去の検証と集計

v7（187モデル/重み374）は全Release CTest129件/18.41秒、ASan/UBSan89件/49.31秒が通過した（各suiteの `-j4` 同時実行）。focusedもRelease3件/0.67秒・SAN2件/1.31秒が通過した。最終全ソースfreeze後のbuild/full検証でcompile warningは無く、Python Match suiteはAST確認14ケース。公式assets再取得のSHA-256と296モデルfactsも一致した。

nativeの9人Solo・HP99・seed0局面では、1行動でcounterを51本生成し50本開始する経路を検証した。50本目で元ownerが死亡し、残りcounterはdead targetによりskip、初期chance rosterの残りは取消となる。全stepのsnapshot restoreと、生存したcreatorだけへのgiftを確認した。固定32のcounter上限を置かず、実消費/MP/decision/journal予算で扱う。

`./build/godfield_official_bench GAMES PLAYERS 42 WORKERS` のv7集計:

| 人数 | 試合数 | 意思決定数 | 勝数 | draw | 打切り | checksum |
| ---: | ---: | ---: | --- | ---: | ---: | --- |
| 2 | 100,000 | 3,771,526 | 49,773 / 50,227 | 0 | 0 | `4348491756840048703` |
| 9 | 10,000 | 2,381,724 | 1,075 / 1,107 / 1,069 / 1,156 / 1,148 / 1,120 / 1,093 / 1,121 / 1,111 | 0 | 0 | `5379530731485268463` |

| 人数 | counter生成 | counter開始 | counter抽選 | counter hit | counter miss | chance攻撃 | chance抽選 / hit / miss |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 2 | 7,881 | 6,885 | 3,998 | 3,007 | 991 | 115,888 | 115,888 / 67,268 / 48,620 |
| 9 | 5,593 | 4,829 | 2,898 | 2,139 | 759 | 48,239 | 194,836 / 113,063 / 81,773 |

counter抽選/hit/missは181の開始時抽選を数え、184の必中counterと初期全敵chanceを混ぜない。counterの生成・開始は別集計で、開始前のdead skip等を同じ件数にしない。いずれも特権ベンチ集計であり、方策の公開observeへ出す情報ではない。

| 人数 | 反射 | bounce | 属性消去 | 遮断 | 乱気流 初 / 再 | 壁 初 / 再 | 自己bounce応答 | 最大redirect深度 |
| ---: | ---: | ---: | ---: | ---: | --- | --- | ---: | ---: |
| 2 | 21,556 | 31,947 | 19,845 | 11,096 | 987 / 296 | 4,589 / 1,282 | 15,946 | 4 |
| 9 | 17,004 | 24,136 | 16,973 | 11,888 | 1,552 / 545 | 3,687 / 1,939 | 4,156 | 5 |

| 人数 | FREE品使用 main / response | 免除MP合計 | Aura | setter |
| ---: | --- | ---: | ---: | ---: |
| 2 | 20,363 / 19,079 | 87,072 | 9,261 | 21,590 |
| 9 | 16,372 / 14,547 | 75,396 | 9,359 | 13,337 |

1 worker/4 workersで時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。観測最大redirect深度4/5は機能上の連鎖上限ではない。Release/SAN seed42各1,000試合も時間以外の全fieldが一致し、checksumは2人 `3714956624162246545`、9人 `13673169011916776123`。

競合を避けたRelease・1 workerの再計測は、2人100,000試合が2.32449秒（1,622,520意思決定/秒）、9人10,000試合が2.49136秒（955,992意思決定/秒）。時間以外の全fieldは上表と一致した。合法手sampler、実行、reset、各試合の終了snapshot検証を含み、合成単体・Python学習・モデル推論/最適化・公式全モデルの速度ではない。scope・policy・プールが異なるため、版間の速度比を改善率として主張しない。

### v6の過去の検証と集計

v6（185モデル/重み372）は全Release CTest129件/18.98秒、ASan/UBSan89件/50.42秒が通過した（各suiteの `-j4` 同時実行）。focusedはRelease3件/0.51秒・SAN2件/0.64秒が通過した。全ソース最終freeze後のbuild/full検証でcompile warningは無く、Python Match suiteは13ケース。公式assets再取得のSHA-256と296モデルfactsも一致した。

`./build/godfield_official_bench GAMES PLAYERS 42 WORKERS` のv6集計:

| 人数 | 試合数 | 意思決定数 | 勝数 | draw | 打切り | checksum |
| ---: | ---: | ---: | --- | ---: | ---: | --- |
| 2 | 100,000 | 3,795,626 | 49,552 / 50,448 | 0 | 0 | `12990827435025744992` |
| 9 | 10,000 | 2,396,687 | 1,144 / 1,136 / 1,058 / 1,101 / 1,168 / 1,114 / 1,089 / 1,061 / 1,129 | 0 | 0 | `12373722458217459627` |

| 人数 | chance攻撃 | 命中抽選 | hit | miss | 反射 | bounce | 属性消去 | 遮断 | 最大redirect深度 |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2 | 117,318 | 117,318 | 68,036 | 49,282 | 22,050 | 32,572 | 20,333 | 11,271 | 5 |
| 9 | 47,957 | 195,003 | 113,248 | 81,755 | 17,302 | 25,129 | 17,252 | 12,353 | 5 |

| 人数 | 乱気流 初 / 再 | 壁 初 / 再 | 自己bounce応答 | FREE品使用 main / response | 免除MP合計 | Aura | setter |
| ---: | --- | --- | ---: | --- | ---: | ---: | ---: |
| 2 | 990 / 279 | 4,760 / 1,337 | 16,347 | 21,022 / 19,452 | 90,195 | 9,606 | 21,796 |
| 9 | 1,623 / 533 | 3,805 / 2,053 | 4,361 | 16,417 / 14,688 | 75,420 | 9,260 | 13,510 |

1 worker/4 workersで時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。chance攻撃数は原発コマンド数、roll/hit/missは各対象への自動判定を数える。redirect後の再命中抽選は数えない。表は特権ベンチ集計であり、これらのnative chanceカウンターを方策の公開observeへ出す契約ではない。

Release/SAN seed42各1,000試合の時間以外の全fieldも一致した。checksumは2人 `14650413054504497333`、9人 `15816808055195738927`。最大redirect深度5は観測値で連鎖上限ではなく、競合ログの秒数を独立速度として使わない。

競合を避けたRelease・1 workerの再計測は、2人100,000試合が1.96549秒（1,931,130意思決定/秒）、9人10,000試合が1.98373秒（1,208,170意思決定/秒）。時間以外の全fieldは上表と一致した。合法手sampler、実行、reset、各試合の終了snapshot検証を含み、合成単体・Python学習・モデル推論/最適化・公式全モデルの速度ではない。scope・policy・プールがv5と異なるため、版間の速度比を改善率として主張しない。

### v5の過去の検証と集計

v5（164モデル/重み351）は全Release CTest129件/20.03秒、ASan/UBSan89件/51.49秒が通過した（各suiteの `-j4` 同時実行）。focusedもRelease3件/1.09秒・SAN2件/1.20秒が通過し、Python Match suiteはASTで12ケースを確認した。全ソース最終freeze後にbuildと全体検証を行い、全sourceのcompile warningは無かった。公式assets再取得のSHA-256と296モデルfactsも一致した。

`./build/godfield_official_bench GAMES PLAYERS 42 WORKERS` のv5集計:

| 人数 | 試合数 | 意思決定数 | 勝数 | draw | 打切り | checksum |
| ---: | ---: | ---: | --- | ---: | ---: | --- |
| 2 | 100,000 | 3,900,109 | 49,546 / 50,454 | 0 | 0 | `8091597239127085284` |
| 9 | 10,000 | 2,787,125 | 1,123 / 1,111 / 1,121 / 1,072 / 1,212 / 1,090 / 1,124 / 1,048 / 1,087 | 0 | 12 | `15009826975196651729` |

| 人数 | 跳ね返し | 乱気流 初 / 再使用 | 自己bounce応答 | 反射 | 属性消去 | 遮断 | 壁 初 / 再使用 | 最大redirect深度 |
| ---: | ---: | --- | ---: | ---: | ---: | ---: | --- | ---: |
| 2 | 36,602 | 893 / 288 | 18,203 | 24,448 | 22,067 | 12,476 | 5,446 / 1,844 | 4 |
| 9 | 31,700 | 1,880 / 639 | 5,801 | 21,983 | 21,187 | 15,703 | 5,126 / 3,692 | 5 |

| 人数 | FREE品使用 main / response | 免除MP合計 | Aura使用 | setter使用 |
| ---: | --- | ---: | ---: | ---: |
| 2 | 20,682 / 22,354 | 71,467 | 11,257 | 24,260 |
| 9 | 21,162 / 19,140 | 90,602 | 14,164 | 17,184 |

1 worker/4 workersで時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。`bounces` は受理された全bounce、`self_bounce_responses` は自己が次のactorとして応答を続けた件数であり、打切りでactorが無くなったbounceは後者に含まない。

v5は旧 `max_reflection_depth` を廃止し、反射とbounceを合わせた履歴深度 `max_redirect_depth` を記録する。表の4/5は観測最大値であり、機能上の連鎖上限ではない。FREE使用数には通常武器/防具としての使用も含み、無料化発動回数とは異なる。競合中ログの秒数を独立速度に使わず、v1〜v4との速度比を改善率として主張しない。

競合を避けたRelease・1 workerの再計測は、2人100,000試合が1.9292秒（2,021,620意思決定/秒）、9人10,000試合が2.24261秒（1,242,800意思決定/秒）。時間以外の全fieldは上表と一致した。合法手sampler、実行、reset、各試合の終了snapshot検証を含み、合成単体・Python学習・モデル推論/最適化・公式全モデルの速度ではない。scope・policy・プールがv4と異なるため、版間の速度比を改善率として主張しない。

Release/SAN seed42各1,000試合も時間以外の全fieldが一致した。checksumは2人 `13433316251301511666`、9人 `10911795126884805082`。

### v4の過去の検証と集計

v4（156モデル/重み337）は全Release CTest129件/19.67秒、ASan/UBSan89件/51.30秒が通過した（各suiteの `-j4` 同時実行）。Python Match suiteは11ケース。公式assetsの再取得SHA-256と296モデルfactsは固定済み資料と一致。

全体検証後、513個のpolicy seed（0〜512）で、0MPの追加Auraと既走査の未使用FREEをatomic pairとして2組まで選べる専用native局面を追加した。対象をRelease1件/0.26秒・SAN1件/0.57秒で再検証済み。coreの追加変更は無い。

`./build/godfield_official_bench GAMES PLAYERS 42 WORKERS` のv4集計:

| 人数 | 試合数 | 意思決定数 | 勝数 | draw | 打切り | checksum |
| ---: | ---: | ---: | --- | ---: | ---: | --- |
| 2 | 100,000 | 3,858,925 | 49,579 / 50,421 | 0 | 0 | `626852514698712269` |
| 9 | 10,000 | 2,776,530 | 1,095 / 1,147 / 1,121 / 1,102 / 1,089 / 1,128 / 1,113 / 1,085 / 1,107 | 0 | 13 | `11822269879695662487` |

| 人数 | FREE品使用 main / response | 免除MP合計 | Aura使用 | setter使用 | 遮断 | 反射 | 属性消去 | 観測最大反射深度 |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2 | 21,576 / 22,484 | 73,916 | 11,505 | 25,211 | 12,681 | 25,231 | 22,678 | 3 |
| 9 | 22,596 / 19,373 | 96,103 | 14,610 | 17,719 | 16,370 | 22,460 | 21,568 | 3 |

FREE品使用数はcutCost能力のある品を実際に使った数であり、46を通常ATKで使う場合や143/163/173を普通防具で使う場合も含む。無料化が発動した回数とは異なり、直後MCの実費用免除量は `waived_mp`（免除MP合計）が表す。

1 worker/4 workersで時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。観測最大深度3は機能上の連鎖上限ではない。表は決定的集計だけを示し、他のsuiteと競合していた計測の秒数をv4の独立速度として使わない。

競合を避けたRelease・1 workerの再計測は、2人100,000試合が1.91505秒（2,015,060意思決定/秒）、9人10,000試合が2.23955秒（1,239,770意思決定/秒）。時間以外の全fieldは上表と一致した。合法手sampler、実行、reset、各試合の終了snapshot検証を含む。合成単体・Python学習・モデル推論/最適化・公式全モデルの速度ではない。プールとsamplerがv3と異なるため、版間の速度比を改善率として主張しない。

Release/SANのseed42・2人/9人各1,000試合でも、時間以外の全fieldが一致した。checksumは2人 `1841161691979893053`、9人 `14516418235990999296`。

### v3の過去の検証と集計

v3（148モデル/重み328）の全Release CTest129件/19.71秒、ASan/UBSan89件/51.45秒が通過した。全ソースの最終freeze後の全体検証で、Release/SAN各suiteを `-j4` で同時実行した値である。性能計測には使わない。Python Match suiteは10ケース。

再取得した公式assetsのSHA-256と296モデルのfactsは固定済みデータと一致した。これは元のpayload取消・補充・次手番等のサーバー規約を確定するものではない。

`./build/godfield_official_bench GAMES PLAYERS 42 WORKERS` のv3集計:

| 人数 | 試合数 | 意思決定数 | 勝数 | draw | 打切り | checksum |
| ---: | ---: | ---: | --- | ---: | ---: | --- |
| 2 | 100,000 | 3,905,076 | 49,897 / 50,103 | 0 | 0 | `11131397330614631561` |
| 9 | 10,000 | 2,849,191 | 1,194 / 1,072 / 1,130 / 1,116 / 1,068 / 1,073 / 1,097 / 1,112 / 1,106 | 0 | 32 | `15567992248220262038` |

| 人数 | 遮断 | 壁の初使用 | 壁の再使用 | 反射 | 属性消去 | 観測最大反射深度 |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2 | 12,674 | 5,816 | 1,668 | 25,960 | 23,458 | 3 |
| 9 | 15,884 | 5,349 | 3,554 | 23,384 | 22,398 | 4 |

各条件の1 worker/4 workersで時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。表の最大深度3/4はこの実行で観測した値であり、実行器の連鎖上限ではない。

競合を避けたRelease・1 workerの再計測は、2人100,000試合が2.41763秒（1,615,250意思決定/秒）、9人10,000試合が2.46832秒（1,154,300意思決定/秒）。時間以外の全fieldは上表と一致した。合法手sampler、実行、reset、各試合の終了snapshot検証を含み、合成単体・Python学習・モデル推論/最適化・公式全モデルの速度ではない。

Release/SANのseed42・2人/9人各1,000試合も時間以外の全fieldが一致した。checksumは2人 `12257384685277614778`、9人 `9792092663242239069`。v1/v2の異なるプール・実装のchecksumと混同しない。

### v2の過去の検証と集計

v2（139モデル/重み319）の全Release CTest129件/60.39秒、ASan/UBSan89件/91.53秒が通過した。両suiteを同時実行した値であり、独立した速度測定に流用しない。その後、nativeの35反射連鎖・Snapshot v1拒否ケースを追加し、対象をRelease/SAN各1件再検証した（0.33/0.65秒）。coreの追加変更はない。

PythonのMatch suiteは9ケース。既存の計算/選択50,270件、state helper/抽出分岐22,140件、実client資源計算7,898件のgolden一致を維持し、再取得した公式assetのhash・296モデルfactsも一致した。これらは権威的serverとの対局trace適合を意味しない。

`./build/godfield_official_bench GAMES PLAYERS 42 WORKERS` のv2集計:

| 人数 | 試合数 | 意思決定数 | 勝数 | draw | 打切り | 反射 | 消去 | checksum |
| ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: | --- |
| 2 | 100,000 | 3,949,983 | 49,709 / 50,291 | 0 | 0 | 27,346 | 24,118 | `17327277230250071888` |
| 9 | 10,000 | 2,862,579 | 1,120 / 1,135 / 1,128 / 1,093 / 1,107 / 1,098 / 1,090 / 1,151 / 1,056 | 0 | 22 | 24,214 | 23,344 | `13034266850202749327` |

各条件で1 worker/4 workersの時間以外の全fieldが一致し、全試合の終了snapshot（自然終局または打切り）をrestoreできた。観測された最大反射深度は両方3だが、これはベンチマーク内の出現結果であり、実行器が3で打ち切るという意味ではない。別のnative局面で35反射を検証している。

競合を避けたRelease・1 workerの再計測は、2人100,000試合が2.51205秒（1,572,410意思決定/秒）、9人10,000試合が2.52802秒（1,132,340意思決定/秒）で、上表の時間以外の全fieldを維持した。合法手sampler、実行、reset、各試合の終了snapshot検証を含み、合成関数単体、Python経由の学習、モデル推論・最適化、公式全モデルの速度ではない。

SAN/Releaseの2人・9人各1,000試合も時間以外の全集計が一致し、checksumは2人 `9045870663897908326`、9人 `15249773347321244157`。v1のプールやchecksumと混同しない。

### v1の過去の検証と計測

以下は **v1・132モデル/重み308時点** の検証と計測記録である。v2・139モデルの結果ではなく、現在のv2プールを同じコマンドで再実行して同じchecksumを得られるという意味でもない。

全Release CTestは129件/21.83秒、ASan/UBSanは89件/50.47秒が通過した。その後、資源primitive単独ではHP0になってもdeadを立てないことを確認するassertを追加し、資源ターゲットをRelease/SAN各1件再検証した。coreの追加変更はない。

根拠ごとの検証範囲は次のとおり。

- 既存の公式client計算・選択50,270件と、fresh状態helper/抽出分岐22,140件のgolden一致を維持。
- [`c5.eW/dH/bc` の実client実行結果7,898件](../data/official-v1/resource-vectors.manifest.json)と資源計算が一致。対象はローカル加減・clip・不足連鎖であり、サーバーの死亡・補充・受理・手番順ではない。
- 新MatchのPython8ケース、native C ABI境界、未対応入力、固定席報酬、phase、表示、保持・補充、チーム終局を検証。
- 48seedで各受理stepのsnapshotを別Matchへrestoreして一致。改ざんした待機effect、token、保持、補充・MP会計等は拒否。
- 更新カウンター上限や後段の補充token枯渇でstepが失敗したとき、既に計算した被害・消費・phase・queue・補充予約・意思決定数・乱数を含めて状態が変わらないことを確認。

以下は `./build/godfield_official_bench GAMES PLAYERS 42 WORKERS` の集計。勝数は入力プレイヤー列の順。

| 人数 | 試合数 | 意思決定数 | 勝数 | draw | 打切り | checksum |
| ---: | ---: | ---: | --- | ---: | ---: | --- |
| 2 | 100,000 | 3,918,160 | 49,436 / 50,564 | 0 | 0 | `92948246955144686` |
| 9 | 10,000 | 2,838,305 | 1,131 / 1,111 / 1,120 / 1,103 / 1,101 / 1,076 / 1,128 / 1,112 / 1,089 | 0 | 29 | `997021765811772519` |

同じ条件の1 workerと4 workersで、時間以外の全決定的な集計とchecksumが一致した。全試合の終了snapshot（自然終局または打切り）をrestoreできた。samplerの最適化前後も、2人/9人各1,000試合の全集計を維持した。ここでの方策はローカル合法手samplerであり、学習済み方策ではない。

SANでもseed42の2人/9人各1,000試合を実行し、Releaseと全集計が一致した。checksumは2人 `259328886224040849`、9人 `17538591307536653932`。

他の計測処理と競合させずに再実行したRelease・1 workerの速度は、2人100,000試合が2.48432秒（1,577,160意思決定/秒）、9人10,000試合が2.48806秒（1,140,770意思決定/秒）だった。合法手のサンプリング、実行、reset、各試合の終了snapshot検証を含む。合成関数単体の速度、Python経由の学習速度、モデル推論・最適化速度、公式全モデルの速度を表す値ではない。

この一致は明記した実験規約の内部整合・再現性と公開clientローカル関数の適合を確認するもので、公式対局traceや権威的サーバーの全状態遷移と一致したという証拠ではない。

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

[official-v2の取得・再生成](../data/official-v2/README.md) / [v1→v2比較](../data/official-v2/upgrade-review.json) / [v1 archive](../data/official-v1/README.md)。

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

[再現script](../scripts/official_dream_oracle.cjs) と [互換クラス報告](../data/official-v2/dream-compatibility-v28.json) を追加した。v28当時の233モデルに対し、別faceを持つのは146モデル（gift重み321）、singletonは87モデル（重み173）。全miracle・tradeと今回のDream付与4種はsingletonで、現在のfake生成対象は非miracle物理品だけである。保持MCをunmaskする共通実装があっても、現クラスで偽装MCの通常到達を検証したという意味ではない。

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

9keyの互換制限は維持し、178はsingletonとして追加する。146 eligibleモデル（重み321）・88 singleton（重み174）・118クラス・992順序付きpairで、全234モデルが50%変装する規約ではない。v28の1,821,312比較・native94/Python67とその対局値は前版の証拠として保持し、今回の一新を検証した値へ読み替えない。新版の[redraw再現script](../scripts/official_redraw_oracle.cjs) / [client報告](../data/official-v2/redraw-client-v29.json) は、実 `fH.lI`・実item decoder・実 `j9.jF` をオフライン抽出し、DOM/widgetだけstubに置き換えた。通常置換223例・不足配列1例・used pane19例を記録する。非empty slotにはused奇跡も含み、空placeholderを維持すること、used paneを全clearすることを照合した。不足配列でthrowし、余剰配列は未使用になるというrendererの挙動も含む。受信itemsのtoken/model/fake/usedを使う表示処理であり、serverの置換枚数・new token発行・used=false再配布・帽子/curse/body/復活/giftの順を証明しない。各helperを個別に呼ぶため、実際の全viewerへのevent fanoutや全UI操作の検証とも区別する。

新版の[Dream再現script](../scripts/official_dream_v29_oracle.cjs) / [互換報告](../data/official-v2/dream-compatibility-v29.json) は234 IDs/118クラス/146 eligible/88 singleton/992 pairを記録する。開始/既定target1,984件・MP14,880件・main追加696,384件・response追加1,111,040件、計1,824,288比較で、検査したclient選択・MP・targetの互換性を照合する。威力/価格/回復値の同値や公式偽装分布は保証しない。旧v28 script/reportは当時の233モデルの証拠として保存する。

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

[自己BUY再現script](../scripts/official_self_buy_oracle.cjs) / [client報告](../data/official-v2/self-buy-client-v30.json) は108ケースで実target選択 `jO.aT`・実alive click roster `h8.aG`・実team判定 `c5.ce`・実MP計算 `A.ld` と、submit `jS.$0` の最初のawaitまでを抽出実行した。自分/敵/味方のtarget、同じtargetの再選択、ID0/最大safe integer、MP0/CP0/Fog/Dreamを含む。108のRPC payloadをローカルstubで捕捉し、自己targetPlayerIdと物理itemIdsが送信され、このpreflightでCPを参照しないことを記録した。DOM/音/Future/backendはstubで、アプリ起動・ネット接続・server応答経路を実行しない。直接methodのdead/unknown-object検査不足は、通常alive rosterからのUIクリックやserverがそれらを受理する証拠ではない。

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

[守護神再現script](../scripts/official_guardian_oracle.cjs) / [client報告](../data/official-v2/guardian-client-v31.json) は、実 `A.p6` attack decoder8ケース、実 `fs.fu` 受信frame・`gf2/iI` attack表示・`tz/lc/ld` 算術・`eu` 応答追加などの664投影を記録する。内訳は42原record×4条件の168、通常239モデルの単品239、仮Earth/Moon compound239、動的表示18。10種のguardian表示のreplaceとclearも個別に照合し、DOM/widget/CSS/text表示先はstubに置く。application startup・通信・serverの行動選出/phaseは実行しない。

通常239モデル単品や仮compoundの入力は、公式selectorがそれらを生成するという証拠ではない。decoderが任意DTOのatk/hitRate/mp/cpを読み、表示関数がoverrideを使うことと、実行器がそれを受理することも区別する。今回のAPIは245〜286のcanonical一モデルとmetadata値だけで、DTOの任意overrideやEarth/Moonの一般神器/奇跡合成は未対応。clientのtarget欠損が0へdefaultする挙動を、実行APIのtarget省略可否へ自動的に移さない。after-filter応答maskも、filter prefix自体の合法性を満たす場合だけ完成した応答として使える。

native `guardian_legal` / `guardian_step` とC `gfo_match_guardian_legal` / `gfo_match_guardian_step`、Python `GuardianAction` / `Match.guardian_legal` / `Match.guardian_step` を追加する。scene入口はC `gfo_match_load_scene`、Python `Match.load_scene(OfficialState, owner, seed)`。同じlibraryのlive stateをscope検査後に独立deep copyし、既存Configを維持してRNG/counter/pending workを初期化する。これはsnapshot checkpointの復元やserverイベント再生ではない。Pythonはrawのpending attacksを拒否し、callerはquiescent sceneを渡す。通常の `sample_action` は守護神を自動選択しない。

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

通常ownerのmain actionが全て終わり、既存のowner病気tick/死亡・自然終局/decision cutoffの優先規約を処理した後、自動守護神のstageへ進む。aliveな敵guardian sourceを入力seat順に扱い、それぞれ25%のbounded抽選をする。成功した場合、最初の8種は公開の6/5/4/3/2でcanonical行動を選び、Earthは285、Moonは286を固定候補にする。候補一つでもmoveの抽選を1回消費する。全10typeを残すが、これは**canonical42に限る独自selector**であり、Earth/Moonの一般神器/奇跡選出を実装したという意味ではない。

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

[新版Dream再現script](../scripts/official_dream_v32_oracle.cjs) / [互換報告](../data/official-v2/dream-compatibility-v32.json) は236 IDs・120クラス・146 eligible・90 singleton・992順序付きpairを記録する。開始/既定target1,984件・MP14,880件・main追加702,336件・response追加1,111,040件、計1,830,240比較である。201/239はsingletonとして追加され、旧v29のscript/reportも保存する。これは検査したclient predicateの互換性で、召喚/行動selectorや公式Dream全分布・威力/価格の同値を証明する結果ではない。

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

公開exact規約は以下とする。

```text
experimental:externally-selected-earth-moon-ordinary-model-columns;main-boundary-one-decision;alive-source-owns-explicit-guardian;all-models-in-guardian-category-domain-and-supported;actual-main-append;max18-models;earth120-moon26-standalone-bases;trade-danger-passive-response-only-bases-unsupported;mixed-domain-and-wide-unsupported;explicit-single-alive-target;14-two-fresh-hits-same-target;single-target-percent-and-darkcloud-no-reroll;no-fog-or-dream-reselection;no-source-inventory-retention-ordinary-mp-fee-or-operation-gift;58-launch-source-mp-times2-then-mp0-once;202-one-launch-sign;fixed-origin-guardian-and-launch-values-through-redirects;ordinary-response-counter-death-redraw-and-deferred-gifts;no-owner-tick-gf-or-auto-followup;resume-owner-or-next-alive;general-selector-unsupported
```

[payload一次oracle](../scripts/official_guardian_payload_oracle.cjs) / [保存report](../data/official-v2/guardian-payload-client-v33.json) を追加した。report SHA256は `b35a47b235d4d1645d030cf5ddb967292971bdb1dfaac36a7f17f1f1ccc74861` で、rootの `--check` はbyte一致した。146単品base＋Earth複合10＋拒否形14＋58境界3、計173ケースで、実 `A.p6` decoderと `ks.k` のattackByGuardian受信branchを同期Future shellで最後まで動かし、実main/response append・ATK/属性/MP算式・`gf2/iI` attack表示を照合する。58のsource MP0/1/17/99も含む。

受信branchはgeneric手札消費・MC費用・保持処理を呼ばず、58だけsource表示MPを読み2倍preview→MP0へ変える。これは描画receiverの事実で、権威的serverの所有・費用・retentionを証明しない。DOM/widget/animation/text/Futureはstub、アプリ起動・通信・selector/効果/乱数scheduleは実行しない。実MC baseのcategoryはmiracles、実weapon baseはweaponsのままであり、guardian画像を理由にguardians categoryへ変えない。反応適合もその実category/属性に従う。

拒否例はMoon[210,231]/[210,232]が実appendで不適合、Moon[218,204]とEarth[6,210]はclient append可でもadapter全列domain外、19モデル列はappend可でもlocal18上限外である。source displayが受信できる広い配列を全てadapterへ許す設計ではない。

新native6群を含む126群のRelease/SAN単体が通過した。新[payload bench source](../src/official_guardian_payload_bench.cpp) を `godfield_guardian_payload_bench` として追加し、CTest `official_guardian_payload_benchmark` に登録した。`./build/godfield_guardian_payload_bench 456 9 42 2` のsmokeは146 bases＋6 Earth複合の152列をcoverし、456 scenes/1,157 decisions/701 responses、146 models/152 columns、departure92 rolls/12、chance50 rolls/32 hits/25 forced、174 redraws、checksum `14485308570291186513` を記録した。これは外部指定payloadのscene試行であり、自動一般selectorの分布評価ではない。大規模回帰・bridge・SAN/full suite結果は次の確定欄へ記録する。v32以前の実測を保持し、通常canonical42自動selectorと配布プールの回帰も別に確認する。広い一般selector・207/現象・悪魔・終末が完成したという主張はしない。

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

restoreは保存したquiescent eventの型・phase・guardianのevent-time Fog maskと、各modelの完成postconditionを検査する。元CP合計や旧病気など、既に上書きした全履歴が実際の公式対局で成立したことまで証明しない。後続normal pending actionでは `phenomenon_actions` のprefix累計をanchorから再生/比較する。即時5primitiveに新pending phenomenon variantやdeathFIFOを追加する設計ではない。

公開exact規約は次の文字列とする。

```text
experimental:external-source-free-global-primitives287-288-290-294-296-only;main-boundary-one-atomic-decision;launch-alive-in-input-order-including-all-teams;287-replace-existing-disease-with-fever-preserve-other-curses-no-upgrade-or-heavenfall;288-idempotent-fog;290-direct-hp1-not-damage;294-sum-alive-cp-then-zero-all-before-explicit-alive-recipient-cap99;296-exact-alive-count-explicit-guardian1-to10-same-type-allowed;no-selector-or-rng;no-inventory-mp-fee-gift-owner-tick-gf-or-auto-followup;body-before-decision-cutoff;unchanged-owner;guardian-changes-event-time-fog-filtered;public-model-recipient-only-no-assignment-array-or-cp-total;207-and-other-phenomena-unsupported
```

[現象一次oracle](../scripts/official_phenomenon_oracle.cjs) / [保存report](../data/official-v2/phenomenon-client-v34.json) を追加し、rootの `--check` はbyte一致した。report SHA256は `67532814e73876a1bddcc282e3336610b6f0ff2078113f6783717bdb8b9820ed`。実event decoder・global受信branch・alive roster・curse/resource/guardian操作・Fog fanoutを実行し、298event＋8decoder＋単独 `sd0(null)` 1helperケースを記録する。DOM/widget/animation/sound/Futureのscaffoldingはstubで、backend・アプリ起動・自動selectorは動かさない。

296のraw guardian配列にnull/unknown/nonstringがある場合、実 `A.to` はそれらをfilterして残った既知typeを詰める。nullは解除slotではなく、単独 `sd0(null)` のclearとは別である。不足配列は先行player変更後にthrowし、余剰配列は残る。今回のtyped adapterはそれより強い制約として、exact alive count・既知1〜10・none無しを事前に検査してatomic拒否する。294のalive recipient制約もreceiver単体より厳格である。rendererが内部値を書き換えることだけでは、他viewerへraw CP合計や割当配列を公開する根拠にならない。

native新6群を含む132群を全てmainへ各1回登録し、Release/SAN単体が通過した。Python92件はprimary298eventを265適用・11入力拒否・22 scene範囲外（20自然終局・2 alive HP0）へ分類して対照した。clientのalive判定はdead flagなのでHP0だけでは除外しないが、実行器はstable/quiescent sceneでHP0&&!deadを受理しない。これは即時5primitiveに死亡queueを加える理由にはしない。

新[接合bench source](../src/official_phenomenon_bench.cpp) を `godfield_phenomenon_bench` として追加し、CTest `official_phenomenon_benchmark` に `500 9 42 2` を登録した。各sceneは外部primitive一回の無RNG/同Config/token・GF0/dec1/同ownerと効果postconditionを検算し、その後にordinary prayerを一回、response完了まで進めて毎step snapshotをrestore監査する。初回primitiveと最終stateは別hashにし、`decisions=2*scenes+responses`、phenomenon prefix累計、後続病気tick/auto guardianとの接合を検査する。最初のprimitiveがGFを変えないことと、後続ordinary action終了時にGFが進み得ることを分ける。CPtotal等の統計とfullstate hashは特権bench用で、player観測には追加しない。

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

[targeted現象一次oracle](../scripts/official_targeted_phenomenon_oracle.cjs) / [保存report](../data/official-v2/targeted-phenomenon-client-v35.json) は101ケース（18frame・69numeric・6reflect・8decoder）を記録し、rootの `--check` はbyte一致した。report SHA256は `9c7a8f00b18d35f8121f48a231719151dea9e16ecdae4f52a1b27447c616bca1`。実decoder、frame/summary、HP算術、反応append、受信HP/通常被害/闇被害/反射branchをvisual/widget・同期Futureのshellで動かす照合であり、serverのselector・sourceなしの返却先・counter生成policyは証明しない。sourceNullableの注記とDTOのplayerId default0を区別し、actual UI appendの適合と、ローカル190取消/反撃抑制を混同しない。

新[専用bench](../src/official_targeted_phenomenon_bench.cpp) を `godfield_targeted_phenomenon_bench` として追加し、CTest `official_targeted_phenomenon_benchmark` に登録した。`./build/godfield_targeted_phenomenon_bench 600 9 42 2` は3モデル全てをcoverし、600scenes / 1,246decisions / 646responses、checksum `3353421385653118848` を記録した。外部起動からそのresponse/FIFO完了までを進め、ordinary mainへは進まない。全stepでsnapshotを復元し、最初のworld frameのenvironmental/source不在、派生frameの実source、特にPlayerID0を検査する。world bodyで増えたcounter数が選択187数を超えないことも必須assertにしている。

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

[item shuffle一次oracle](../scripts/official_item_shuffle_oracle.cjs) / [保存report](../data/official-v2/item-shuffle-client-v36.json) は249ケース、792,139bytes、SHA256 `192af0ec5dfbcc81979683c0ba5eb05e888963ab9de8644920a087a073b0aed6` を記録する。内訳はdecoder9・通常inventory152・短いinventory1・不正model3・受信branch36・fresh projection48。実event/item decoder、replaceItems受信branch、nonempty置換loop、used-pane clear、供給model/fake/token/usedのwidget投影とfresh-stateの手札/used表示/face sortを動かす。DOM/visual/widget・同期Future等のshellを使い、アプリ起動・ネットワーク・server selectorは実行しない。

実行結果では、incremental受信は本人の手札だけを置換し、全viewerの該当player used-paneをclearする。そこにはdead/Fog除外や全aliveへ分配するloopが無い。一方fresh snapshotは本人の手札を供給個数で再構築し、供給used flagから各playerの公開used-model表示を作る。両経路の違いを295のserver fanoutや保存則の証明に読み替えない。供給rowの重複/0 token・fake・usedをrendererが扱えることも、Matchで不正なfakeや曖昧なmanifestを受理する根拠ではない。短い置換listは途中の表示変更後にthrowし、余剰rowは未使用となるため、typed APIのexact cover先行検査とtransactional rollbackは独立したローカル契約である。

新[shuffle専用bench](../src/official_item_shuffle_bench.cpp) を `godfield_item_shuffle_bench` として追加し、CTest `official_item_shuffle_benchmark` は `600 9 42 2` を指定する。私的なbefore world・manifest・after worldと全snapshot restoreを照合し、明示shuffleの後に通常samplerの1主動作と全response/purchase決定を進める設計である。world/slot・manifestのhashを「全snapshot field hash」とは呼ばず、非公開のItemShuffleRecord/journalを含む一致はrestore比較で別に検査する。分配の確率分布や公式selectorの証拠ではない。最終検証値は次の確定欄へ記録する。

v36の最終全suiteはRelease **135/135件**（23.82秒）、ASan/UBSan **95/95件**（native1件2.28秒、残94件56.28秒の分割実行）が通過した。native144群は全てmainへ各1回登録し、Release/SAN単体も通過した。Python101件、C API/Python、固定query384件、旧173/298ケースと実clientの2,360回のlegality照会も継続して通過した。新6群はowner別token0、used保持と再使用、Dream境界、最大162copy、dead/empty、fresh IDとupdate枯渇時rollback、18種のproof改竄、反復shuffle後の通常対局再生を検査する。READMEの最小Python例も実行し、移送後のfresh token1/2・used保持・owner/GF不変を確認した。suite時間を性能比較に使わない。

通常/canonical/payload/global phenomenon/targeted phenomenon/shuffleの各2人数、計12組で2人100,000局またはscene・9人10,000局またはsceneを1worker/4workersで実行し、worker数・時間・速度以外の全fieldが一致した。既存の通常/canonical/payloadはv35のgameplay field・checksumを維持した。global/targeted現象はSnapshot.version36をhashへ含むためchecksumは変わるが、その他のgameplay fieldはv35と同値だった。Release/SANの通常1,000局×2人数・canonical420・payload456・global500・targeted600・shuffle600の計7組も非時間fieldが一致した。

| 明示shuffle＋後続1主動作・seed42 | scenes | decisions / responses / purchase | 移送copy / 最大copy | primitive checksum | manifest checksum | 後続world checksum |
| --- | ---: | ---: | ---: | --- | --- | --- |
| 2人 | 100,000 | 273,390 / 71,968 / 1,422 | 2,077,577 / 36 | `15674481000839696594` | `15731540149134808078` | `11018779129227397467` |
| 9人 | 10,000 | 48,722 / 28,117 / 605 | 928,747 / 162 | `8353509864660691491` | `421095288420525988` | `17742503789800449523` |

shuffle smoke600sceneは2,963decisions/1,719responses/44purchase、55,724copy（empty7、used5,029）、mask保存1,588/解除930を記録した。後続通常行動ではcounter138、死亡時frame75、revival54、redraw167、自動guardian1,000も通過し、world checksum `2901487151624391478` がRelease/SANで一致した。shuffle自体はRNG/GF/tick/autoを変えず、これらはその後の普通の主動作に由来する。manifest/world hashと全snapshot restoreを区別し、公式のshuffle分布・server schedulingの証拠とはしない。


## v37：混乱289と外部指定の混乱主動作

`official-execution-subset-v37` はgeneric現象へ289のconfuseEverybodyを加え、9 genericモデルと専用295を合わせて10現象効果を外部入力で扱う。通常配布236モデル/重み499、canonical42自動guardianとtyped Earth/Moon payloadの範囲は変更しない。Snapshot37は旧1〜36を拒否し、公開viewは `official-execution-view-v35`。Match ABI2と既存Commandの幅/opcode/phaseを維持し、同じCommandを受ける外部 `confusion_legal` / `confusion_step` を追加する。207の自動現象抽選や混乱の自動選択分布を実装する更新ではない。旧37profile、official-v2 pins、各版の実測・v10/v16訂正履歴を保存し、学習器を追加しない。

### 現行clientで確定するcountと表示

公式JAは「みんなが三回ずつ勝手に行動する」、ENはlose control for 3 turnsと説明する。実clientのconfuseEverybody受信branchはalive全員のcountへ3を加算し、再付与を3へclampしない。useAttackItemsの末尾は正countだけを1減らし、itemsが空でも同じである。`advanceGF` 受信時は正countなら御乱心の演出を行う。ここにstartTurnというwire actionを仮定しない。死亡表示は本人countを0にし、endGameはalive全員を0にする。

混乱は病気/Fog/Dream/閃光/暗雲のcurse集合とは別の状態で、普通のmild/all cureはcountを解除しない。HP0から208で救済され、実死亡を経なければcountを保持する。公開viewは既存のconfused boolだけを返し、数値countや残り自動選択を公開しない。Fogの既存status pane非表示にも従う。

clientは受信した行動を表示するが、勝手に選ぶcommand・対象・候補の分布、responseや購入確認を誰が選ぶかは示さない。旧FlashのJUPITER/ILLUSIONは見かけの神器を変える別効果で、今回の混乱countや自動行動policyの根拠にしない。

### 選択分布を作らない外部control入口

mainでownerのcountが正の場合だけ、外部 `confusion_step(Command)` を受理する。同じ通常commandのshape・所有token・visible selection・MP等を検査し、受理した起動のbodyより前にcountを1減らす。祈り/捨てる/取引/両替にもuseAttackItems（emptyを含む）と補助eventを対応させて1減らすのは、明示的なローカルscheduleである。responseとpurchase決定は通常のstep/samplerで進み、countを減らさない。

混乱ownerのmainでは通常のlegal/step/samplerを拒否し、その拒否でpolicy RNGも変更せず、playerが自由に選んだ行動を自動選択と呼び替えない。外部driverがcommandを指定する特権入口であり、一様な乱数選択・完全な公式混乱AI・自動連続実行を実装したとは呼ばない。1回の外部指定は1decisionで、既存のbody/反撃/死後work/購入/補充・tick・guardian stageを通常主動作として進める。未来の混乱行動やpolicy RNGを公開しない。

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

[confusion一次oracle](../scripts/official_confusion_oracle.cjs) / [保存report](../data/official-v2/confusion-client-v37.json) は158ケース、137,800bytes、SHA256 `335a67ee5978deeda63fdde663274e5b22078c261e46c335581fa322056c83b5` を記録し、rootの `--check` はbyte一致した。内訳はdecoder/fresh投影22・setter6・global48・safe-number境界5・attack use36・補助event12・cure6・advanceGF9・死亡4・終局4・command活性化6。実decoder、global/useAttackItems/補助/removeCurses/advanceGF/endGameの受信branch、player投影・setter・死亡処理・command活性化を、DOM/widget・同期Future等のshellを使って照合する。アプリ起動・ネットワーク・server自動選択は実行しない。

primaryではpray/discard/sacrifice/useDefenseItemsのevent単独には減算が無く、減算はuseAttackItems末尾にある。したがって実行器で祈り/捨てる/取引/両替を一律1減算へ対応づける部分は、完全なserver event列から導いた事実ではない。正countでも実clientの `fv.fF` は本人のhand/target/command controlsを活性化するが、これもserverが手動入力を受理する証拠ではない。専用外部APIと通常main拒否は明示的な実験制限である。raw JSのunsafeな加算表示を示す境界ケースと、Matchのsafe integerを超えた加算のatomic拒否も区別する。

新[confusion専用bench](../src/official_confusion_bench.cpp) は289を1回または2回適用し、初期alive数×3回までのmain起動と、その全response/purchase決定を進める。countを0にした特権cloneで既存samplerからcommandを提案するのはテストcontrollerだけの方策で、live matchを変更せず、公式の混乱selectorとは呼ばない。通常main API拒否・policy RNG不変、専用APIの適法性、減算前anchor/flag、他者count保存と死亡/自然終局のclearを各stepの全snapshot restoreと合わせて検査する。world/countと選択した累計のhashを全field hashと呼ばず、私的journalまで含む一致はrestore比較で確認する。最終実行結果は次の確定欄へ記録する。

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

[Fate一次oracle](../scripts/official_fate_oracle.cjs) / [保存report](../data/official-v2/fate-client-v38.json) は421ケース、117,149bytes、SHA256 `0f60a34134d1f42eaaf66e478f49aea78b5018d46e74ad7bf1a635f1cd0429b9` を記録する。append296・role4・target/submit48・decoder20・announcement53で、実append/start/default-target、選択・target helper、submit preflight、event decoder、useAttackItems/phenomenon受信、summary/数値helperを同期shellやcaptured RPCで照合する。アプリ起動・ネットワーク・server selectorは実行しない。

実clientでは207にFREE等を追加できず、選択時の自動enemy defaultも無いが、後からgeneric helperで選んだtargetが送信へ残る場合はある。submitが送るのはphysical itemIdsと残ったtargetで、選出した結果・重み・selector seedは送らない。phenomenon受信は整数modelを表示するだけで、無理に供給した非現象modelまで描ける。したがってrenderer受理を結果候補の合法性へ読み替えず、Fateのtarget無し入力・十種の外部結果・単一decision・消費/反射/補充scheduleは明示したローカル規約とする。naked207のresponse UIが190を認めることも、207自身に別の応答窓がある証明にはしない。

v38の最終全suiteはRelease **137/137件**（21.64秒）、ASan/UBSan **97/97件**（50.11秒）が通過した。native160群は全てmainへ各1回登録し、Python109件・C API/Python focused2件・固定query384件も通過した。旧173/298ケースと実clientの2,360回のlegality照会は維持した。新9群はscene限定207/capability/sampler、全10結果、207前anchor、sourceなし応答、postconsume295とgift順、confusion debit、MAX-2の安全加算、死亡/復活/end/cutoff、late GF/gift枯渇rollback、17種のproof改竄、完了後・guardian応答・反復起動のreplay、207のdiscard/Fog売却/購入/除去を検査する。橋ではvariant不一致をmanifest pointer参照前に拒否し、207をoverflow victimにする通常SELLも確認した。READMEのglobal288/295例は実行して記載どおりの結果になった。suite時間を速度改善の指標に使わない。

通常/canonical/payload/global phenomenon/targeted phenomenon/shuffle/confusion/fateの各2人数、計16組で2人100,000局またはscene・9人10,000局またはsceneを1worker/4workersで実行し、worker数・時間・速度以外の全fieldが一致した。既存の通常/canonical/payloadはruleset・scope・rule metadataを除くv37のgameplay field・checksumを維持した。global/targeted現象・shuffle・confusionはSnapshot.version38をhashへ含むためchecksumが変わるが、その他のgameplay fieldはv37と同値だった。Release/SANの通常1,000局×2人数・canonical420・payload456・global500・targeted600・shuffle600・confusion600・fate600の計9組も非時間fieldが一致した。

新しい[Fate専用bench](../src/official_fate_bench.cpp) は `godfield_fate_bench`、CTest登録は `official_fate_benchmark 600 9 42 2` である。全10結果を固定の周期で指定し、各sceneで207を一度使って全responseを完了する。これは公式分布の抽選でも、全sceneの自然決着までの対局でもない。world・選択した累計・manifestのhashと、private journalを含む全snapshot restoreの検査を区別する。

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

### 原子的失敗・復元と残件

privateな非再帰の起動前anchor・action・全responseを保持し、完了後も同executorを通じた全snapshot比較で検証する。private devil_actionsは既存external系の累計と分離し、後続actionも全anchorへそのprefix累計をコピーして再生する。次のaccepted main/externalでeventと証拠をclearし、失敗時はHP・除去・RNG・count・giftを含め元状態を保持する。pending死亡workを途中で捨ててquiescent stateへ見せかけない。

この入口はtiebreakGFのgate、25%出現、ささげる、giftの悪魔置換や連鎖を実装しない。207結果選出・Earth/Moon一般selector、公式Dream全分布・実体不適合入力、終末/悪魔の自動schedule等は残件として維持する。指定した5種への分布正規化も行わず、現行serverの全面再現とは呼ばない。

公開 `devil_rule` はheaderと同じ次の文字列である。

```text
experimental:external240-to244-explicit-alive-target-only;main-boundary-one-external-decision;no-owned-devil-or-gift-selector-tiebreak-threshold-sacrifice-or-cascade;standard236-weight499-and-special207-unchanged;240-241-242-direct-hp-minus10-20-30-no-initial-response-def-reflection-or-counter;positive-hp-loss-uses-local-ten-percent-guardian-departure;243-exact-min2-physical-copies-distinct-owned-token-order-including-used-passives207;no-removal-rng-gift-or-ability-trigger;local-allviewer-actual-model-list-and-existing-fake-reveal-not-server-fanout-proof;244-explicit-hp-mp-cp-plus10-cap99;inactive-resource-and-removal-fields-rejected;shared208-revival112-death-and-response-counter-fifo;no-operation-cost-gift-owner-tick-gf-auto-or-confusion-debit;revival-gift-uses-existing-pool-without-new-devil-roll;resume-live-owner-or-next-alive-without-gf;natural-before-cutoff-body-before-pending-cancel;private-preaction-nonrecursive-anchor-full-response-replay-through-completion;public-model-target-removed-models-only-no-resource-token-or-anchor;failure-atomic;appearance-rate-resource-and-copy-choice-and-server-schedule-unverified
```

### v39の検証

[悪魔一次oracle](../scripts/official_devil_oracle.cjs) / [保存report](../data/official-v2/devil-client-v39.json) は260ケース、749,116bytes、SHA256 `f77dacc0616469a10b204cf07027c57bc23becd088e929ba6625c879a6a8d4b0` を記録する。useDevil60・gift後use6・数値followup120・forced宛先9・除去followup54・decoder11で、実gift/useDevilItem/resource/removeSomething dispatcher、本人cleanup/placeholder、dr/kq/cA/bZのreveal、used-pane削除、資源算術・pane宛先helperを動かす。visual/widget/同期Future等のshellを使い、アプリ起動・ネットワーク・権威的serverは実行しない。

実受信ではuseDevilItem単独はHP/MP/CPを変えず、後続eventが値を供給する。removeSomethingは強制した3rowまで全件表示でき、二個という上限や候補をhandlerが選ぶわけではない。fakeからactualへのrevealはself/foreign/spectator/Fogでも描画され、本人のphysical token削除とpublic used-pane削除は別の処理である。この観測は供給eventの表示内容を示すだけで、全viewerへそのeventを配るserver権限や順序を証明しない。5モデルの型・量と、typed外部選択/二個制限/全viewer fanout/死亡schedule等のローカル契約を区別する。

v39の最終検証はRelease **138/138件**（既存137件18.69秒＋新bench1件0.23秒）、ASan/UBSan **98/98件**（既存97件45.48秒＋新bench1件0.46秒）の分割実行で全て通過した。native168群はmainへ各1回登録し、Python114件・C API/Python focused2件・固定query384件も通過した。旧173/298ケースと実clientの2,360回のlegality照会は維持した。新8群は5モデルの直接効果/資源cap、243のmixed used/fake/passive/207、shapeとphase排他、late update・208gift ID枯渇のrollback/cutoff、208と自然終局、PlayerID0の112 FIFOとMP反撃・生存owner/次alive復帰、local GP退去/Fog、15種の完了proofと4種のpending改竄、後続Fate/shuffleの累計引継ぎを検査した。READMEの243/244例は実行して記載結果と一致した。READMEにあった旧版系Snapshot38という誤記も実値37へ戻し、旧版コード/ABI/pinsは変更していない。suite時間を速度比較へ使わない。

通常/canonical/payload/global phenomenon/targeted phenomenon/shuffle/confusion/fate/devilの各2人数、計18組で2人100,000局またはscene・9人10,000局またはsceneを1worker/4workersで実行し、worker数・時間・速度以外の全fieldが一致した。既存の通常/canonical/payloadはruleset・scope・rule metadataを除くv38のgameplay field・checksumを維持した。global/targeted現象・shuffle・confusion・fateはSnapshot.version39をhashへ含むためchecksumが変わるが、その他のgameplay fieldはv38と同値だった。Release/SANの通常1,000局×2人数・canonical420・payload456・global500・targeted600・shuffle600・confusion600・fate600・devil600の計10組も非時間fieldが一致した。

新[悪魔専用bench](../src/official_devil_bench.cpp)は `godfield_devil_bench`、CTest登録は `official_devil_benchmark 600 9 42 2` である。5モデルを固定周期で指定し、244の資源3種、243の0/1/2コピー、alive/dead/ID0/owner死亡・208/112を含むsceneから、1外部効果とその全responseを完了する。world・選択した累計のhashと、private journalを含む全snapshot restoreの検査を区別し、25%出現率や公式のselector分布を検証したものではない。

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

### 混乱・sampler・再現性

混乱の専用 `confusion_step` もopcode9を受け、他mainと同じくlaunchでcountを1減らす。通常の混乱main拒否、response/purchaseの通常API・count非減算は維持する。samplerは起動時のstageでdiscard proposalのkindを9へ写し、候補の多重度・方策RNGの使い方を変更しない。default threshold=nullの既存対局・配布RNGは変えない。

通常ActionBatchが起動前GF/thresholdとkindを保存して全response journalを再生するため、新しいprivate proof型は不要である。after-stateのGFではなくanchorのlaunch条件で検証する。read-only stateに閾値があることと、公式の全終末scheduleを実行できることは区別する。ABI2の幅は同じでも、clientは新opcode9とruleset40を認識する必要がある。

残件は25%の自動悪魔出現、悪魔種/resource/copyの選出、gift置換・再授与連鎖、207/Earth/Moonの一般selector、公式Dream全分布・実体不適合入力等である。今回を終末完全対応とは呼ばず、旧Flashに見当たらない終末処理を補助仕様として持ち込まない。

公開 `apocalypse_rule` はheaderと同じ次の文字列である。

```text
experimental:decoded-nonnegative-safe-tiebreak-gf-or-null;active-iff-threshold-present-and-current-gf-at-least-threshold;raw0-active-from-gf0-local-domain-not-an-extra-ui-preset;default-reset-threshold-null-and-standard236-weight499-special207-unchanged;main-discard-before-threshold-sacrifice9-at-or-after-threshold;canonical-ui-operation-replacement-not-server-manual-opcode-acceptance-proof;virtual2-not-owned-or-gifted;one-or-more-distinct-owned-physical-tokens-no-target-allocation-or-mp-fee;visible-discard-eligibility-excludes-weapons208209-includes-used-miracles-compatible-dream-only;erase-all-selected-copies-including-used-then-reserve-one-gift-per-copy;shared-selected-fake-reveal-no-item-ability-retention-or-self-curse-trigger;ordinary-termination-cutoff-owner-tick-revival-death-auto-guardian-deferred-gift-gf-next-owner-schedule;natural-and-cutoff-cancel-gifts-as-normal;confusion-api-one-launch-debit;sampler-single-copy-operation-proposals-switch-kind-without-extra-selector-rng;private-prelaunch-gf-threshold-command-and-whole-pending-journal-replay;public-derived-apocalypse-active-bool-no-private-gift-manifest;no-devil-appearance-selection-gift-cascade-or207-distribution;server-gift-and-turn-order-unverified
```

### v40の検証

[ささげる一次oracle](../scripts/official_sacrifice_oracle.cjs) / [保存report](../data/official-v2/sacrifice-client-v40.json) は742ケース、157,046bytes、SHA256 `58e87ec7589141b852fb89af1d23af9e8109e11ca9b51f50c0cbba7cae9ccda9` を記録し、rootの `--check` はbyte一致した。内訳はthreshold decoder60・append592・used/fake16・fresh operation50・submit/consume/events24。実 `fu.gbw/sbh`、`tn`、fresh `fv.fq/fF`、`eu/ld/jS`、`fH.ed`、useAttackItems/sacrifice/discard、`e7/kh` をDOM/widget・同期Future等のshellで照合し、18個の消費から18 empty placeholders、usedMCの完全消費/公開used表示削除、費用0、Flash複数選択と送信operation0を確認した。placeholder表示は実配布の抽選・到着数・悪魔連鎖を証明しない。raw負閾値や互換クラス外fakeのreceiver挙動をMatch受理範囲へ広げていない。

v40の最終検証はRelease **139/139件**（既存138件24.85秒＋新bench1件0.21秒）、ASan/UBSan **99/99件**（既存98件52.37秒＋新bench1件0.49秒）の分割実行で通過した。native175群はmainへ各1回登録し、Python119件・C API/Python focused2件・固定query384件も通過した。旧173/298ケースと実clientの2,360回のlegality照会は維持した。新7群はnull/0/境界/MAX/負閾値/reset、used/fake/178/168/207の複数消費・能力不発/MP0、18usedMCとcutoff、role/identity/phase排他、confusion1debitと512seedのsampler対応、tick死/112/208と自然終局、late update/token/GF枯渇rollback、guardian応答中のlaunch kind/閾値/予約数の4種改竄を検査する。READMEの最小例も実行し、`True False True` と `main 7 1 [(1, False)]` が一致した。suite時間を速度比較へ使わない。

通常/canonical/payload/global phenomenon/targeted phenomenon/shuffle/confusion/fate/devil/sacrificeの各2人数、計20組で2人100,000局またはscene・9人10,000局またはsceneを1worker/4workersで実行し、worker数・時間・速度以外の全fieldが一致した。閾値なしの既存の通常/canonical/payloadはruleset・scope・rule metadataを除くv39のgameplay field・checksumを維持した。global/targeted現象・shuffle・confusion・fate・devilはSnapshot.version40をhashへ含むためchecksumが変わるが、その他のgameplay fieldはv39と同値だった。Release/SANの通常1,000局×2人数・canonical420・payload456・global500・targeted600・shuffle600・confusion600・fate600・devil600・sacrifice600の計11組も非時間fieldが一致した。

新[ささげる専用bench](../src/official_sacrifice_bench.cpp)は `godfield_sacrifice_bench`、CTest登録は `official_sacrifice_benchmark 600 9 42 2` である。5閾値（null/0/17/18/MAX、初期GF17）から適切なdiscard/sacrificeを一度起動し、全responseを完了する。isolated sceneだけでコピー数・新gift数を厳密比較し、rich sceneではtick/112/auto/redraw等の後続と混同しない。worldと選択した累計のhash、private journalを含む全snapshot restoreを区別する。

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

一次資料の再現は[official_grant_oracle.cjs](../scripts/official_grant_oracle.cjs) と[grant-client-v41.json](../data/official-v2/grant-client-v41.json)。`--check` は保存reportとbyte一致し、42ケースの内訳はReference表示20、予約slotへのgift/useDevilItem列12、snapshot decoder probe6、gift event decoder probe4。reportは42,014 bytes、SHA256は `69cdc991cbf06fe36991db69672103d87d6e416716f1b6d833cacc602b3b7143`（現fileから再計算）。実Referenceは5悪魔の出現率を7%/5%/3%/5%/5%と表示し、その合計25%はJA/ENの説明と整合する。実受信queue・既存placeholderへの配送/置換とdecoderを実行し、DOM/演出等のsinkはstubを使う。これらはserverによる悪魔出現・資源/copy選択・債務retryや死亡時の補充scheduleの証明ではない。通常配布のReference分母500も、実行器の236モデル/重み499へ読み替えない。

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

[sourceとローカルreferenceの生成器](../scripts/official_grant_sampler_oracle.cjs) / [保存report](../data/official-v2/grant-sampler-client-v42.json) は、実Referenceの表示14件と、**別区分のローカルgolden**（100ticket境界12件・edge10件）を保存する。後者は独立BigIntによるSplitMix64/逆写像・rejection・partial Fisher-Yatesの参照値であり、公式chooserのgoldenとは呼ばない。full17は実際のH18予約境界、full18は現在到達不能の低水準reference専用として明示している。reportは31,427 bytes、SHA256 `c8ea648daba7af3217fe798909277b40befbaae58322bb0a69a535aeb4062cfa`。rootの `--check` はbyte一致した。

Release CTest **141/141（20.97秒）**、ASan/UBSan CTest **101/101（47.07秒）** が全て通過した。native188関数は配列へ各1回・旧183件の順序を保って登録し、Pythonは新3件を加え129メソッドが重複なく登録された。C API/Pythonのfocused2件も通過した。新native5群は逆写像で作る全100ticketの厳密mass、不変query/rejection、raw0/1/2/17copyとused/fake/passive/207・全資源上限、wrongphaseとchance copy-commit/遅い失敗、snapshot＋外部chance RNGの再開、独立BigIntの到達可能21境界/edge vectorを検査する。Cの9固定goldenは32byte全体のinactive/padding、NULL/phase failureのcanaryを含み、Pythonは独立SplitMix/FY参照と全100ticket、strict uint64 seed等を検査する。

テスト追加後、ASanで旧テストの入口にstack-overflowを検出した。mainへの多数関数のinline合算を避けるため、188件を関数ポインタ配列から順に呼ぶrunnerへ変更した。内容と順序、unused-functionによる未登録検出は保ち、修正後の全SAN suiteで通過した。suite時間を実行器の性能差へ読み替えない。READMEの例も実行し `[242, 0] main 7 3 4354685564936845396` が一致した。

通常/canonical/payload/global現象/targeted現象/shuffle/confusion/fate/devil/sacrifice/明示grant/新grant samplerの各2人数、計24組で2人100,000局またはscene・9人10,000局またはsceneを1worker/4workersで実行し、時間・速度・worker数以外の全fieldが一致した。Release/SAN小行列13組もchecksumを含む非時間fieldが一致した。サンプラーを呼ばない既存11familyのgameplay fieldはv41と同値で、通常/canonical/payloadはchecksumも同値だった。残るfamilyの旧版比較だけはSnapshot.version42を含むchecksumを除き、worker間・Release/SAN間ではchecksumも照合した。

新 `godfield_grant_sampler_bench`（CTest `official_grant_sampler_benchmark 600 9 42 2`）は5起動元とrich sceneから、全grant choiceをhelperで選び、必要な応答を含む元actionの完了まで進める。通常policyと独立chance streamを持ち、各grant境界で2個のrestore cloneに同seedを渡してpayload・next_rng・Match不変を照合する。実stepが成功した後だけchance streamをcommitする。97scene毎はmax_decisions2でsample適用後cutoffを検査する。600scene以上では6outcome、全3資源、used奇跡の除去が実際に選ばれたことを検査する。choice列、chance RNG列、world＋選択counterのchecksumを分け、全private snapshotは各checkpointのrestore完全一致で別に検査する。

| ローカル抽選・seed42 | scenes | normal / devil解決 | 妖精HP / MP / CP | chance checksum | action checksum | world checksum |
| --- | ---: | ---: | ---: | --- | --- | --- |
| 2人 | 100,000 | 167,961 / 55,780 | 3746 / 3676 / 3733 | `14045303676355207807` | `5923566862139370756` | `12269019345486087550` |
| 9人 | 10,000 | 32,203 / 10,729 | 701 / 704 / 695 | `4731159512000537428` | `16856585937920654990` | `4831156122709328760` |

9人600sceneでは4,602decisions、2,592grant解決（normal1903/devil689）、death280予約/214開始、revival164、redraw191、cutoff probe7を含む。243はused13・fake37・207は8copyを選び、244のHP/MP/CP指定は38/43/41回（上限資源の指定61回）。chance/action/world checksumはそれぞれ `10419100228575040986` / `14144487001133331999` / `91435489873529096` でRelease/SAN一致。fixtureの終了・死亡・cutoffにより観測したsample数はscene数と異なり、これを公式serverの対局分布や同値性の検証として扱わない。


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

[配布pool一次調査とローカルreference](../scripts/official_fate_pool_oracle.cjs) / [保存report](../data/official-v2/fate-pool-client-v43.json) は、実Reference16件（207等の1/500対照と現象10種の率labelなし）、全237positive ID・重み500・category件数を固定する。ID順237ranges・独立BigIntの境界5件・Dream signature比較は**別区分のローカルreference**で、公式chooserのgoldenとは呼ばない。207のticket466/467/468周辺を、同seedの旧499版とも比較する。reportは42,845 bytes、SHA256 `149ca93115e05835cd08e80c3ec78c384b0c7dddf3fa340fadd4fc3e35f3c66a`、rootの `--check` はbyte一致した。

Release CTest **142/142（28.40秒）**、ASan/UBSan CTest **102/102（54.98秒）** が全て通過した。native193関数とPython133メソッドは各1回・重複なしで登録し、C API/Pythonのfocused2件も通過した。新native5群では全500ticketの厳密model/giftRate一致、current-queryとglobal能力の分離、6配布経路の207、Dream追加drawなし、通常role/payloadの境界、授かった207の既存Fate使用とpending/完了proof、modeに依存するdrawの再生、defaultfalseの互換、通常操作/受理済みgrantの遅いtoken枯渇rollbackを検査する。Earth payload不許可はsourceが実際にEarthを持つsceneでも検査し、guardian不一致だけによる拒否と混同しない。

新Python4件とC群では全237 ID・合計500、C Configの両flag/offset/tail4byte、NULL出力canary、全配布経路のunused/unmasked207、既存Fate使用/normal sampler除外、defaultfalseの再現性を確認した。READMEの初期207例を実行し、`237 500 236`、hand token0/face207/未使用非偽装、`main 1 1 {'actor': 0, 'model': 207, 'phenomenon': 288}` が一致した。suite時間を速度比較へ使わず、配布massの検証数を公式serverの抽選や全状態遷移の検証数へ読み替えない。

通常/canonical/payload/global現象/targeted現象/shuffle/confusion/fate/devil/sacrifice/明示grant/grant sampler/新Fate poolの各2人数、計26組で2人100,000局またはscene・9人10,000局またはsceneを1worker/4workersで照合し、worker数・時間・速度以外の全fieldが一致した。Release/SAN小行列14組もchecksumを含む非時間fieldが一致した。新flagを使わない既存12familyのgameplay fieldはv42と同値で、通常/canonical/payloadはchecksumも維持した。その他の旧版比較だけはSnapshot.version43を含むchecksumを除き、worker間・Release/SAN間ではchecksumも比較した。

新 `godfield_fate_pool_bench`（CTest `official_fate_pool_benchmark 600 9 42 2`）はreset/pray/1または3copy sacrifice/178redraw/208救済/外部normal0の6経路を回す。到達した207の所有者がmain ownerなら、結果288を明示した既存Fateを一度使い、その後の補充まで完了する。通常rich matrixとは別に、6経路のclean fixtureについて独立500ticket/Dream参照から207到達seedを探し、6つの実sceneでreceipt・RNG・tokenを厳密比較する。検索中にengineを反復するものではない。10000scene以上では全237model出現と授かった207の明示使用が実際に起きたことを検査する。

| 任意237pool・seed42 | scenes | 出現model種数 | 新token発行 / 観測copy | 観測207 / 明示Fate使用 | primitive checksum | world checksum |
| --- | ---: | ---: | ---: | ---: | --- | --- |
| 2人 | 100,000 | 237 | 451,885 / 451,885 | 901 / 435 | `5426914609901215439` | `9144771191245312904` |
| 9人 | 10,000 | 237 | 155,942 / 155,942 | 320 / 47 | `17957368678633913634` | `999832884538341161` |

9人600sceneでは1559decisions、9364新token・9364観測copy、16個の207、明示Fate1回、redraw100、revival132を含む。独立参照の補助probeは6経路、91copyを照合し、207は6経路すべてで到達、reference checksum `10292494696839945612` が一致した。world＋選択counterのchecksumは `10861144430966346463`、全private proofは各checkpointのrestore完全一致で別に検査する。新token数・観測copy数・独立probeの集計は別であり、これらを公式serverの出現分布や超常現象selectorの検証として扱わない。


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

[一次資料と独立ローカルreferenceの生成器](../scripts/official_fate_sampler_oracle.cjs) / [保存report](../data/official-v2/fate-sampler-client-v44.json) は、実Reference等5ケースと、**別区分のLOCAL BigInt golden17件**を保存する。全10ticket、zero-word rejection、owner/ally宛先、0/1/7/161copy、dead除外・他ownerの同token0・別207保持、9人分guardianを含む。これは公式chooser goldenではない。reportは93,246 bytes、SHA256 `a9a9ff64258836e073b713f84b3954c59149ec0190d2182ceba501a9663ce207`、rootの `--check` はbyte一致した。

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

[証拠と独立ローカルreferenceの生成器](../scripts/official_confusion_sampler_oracle.cjs) / [保存report](../data/official-v2/confusion-sampler-client-v45.json) は、JS/JA/EN/cardsとv37のarchive hashを照合する。v37の実receiver検証11ケースを**継承**し、今回の新しいreceiver実行は0件と明記した。生存者+3、useAttackItems後の1減算、補助/防御event単独で減算しないこと等の事実を、自動方策の証明へ読み替えない。

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

[一次appendと独立ローカルreferenceの生成器](../scripts/official_guardian_payload_sampler_oracle.cjs) / [保存report](../data/official-v2/guardian-payload-sampler-client-v46.json) は、実client appendで基底120/26・Earth88×26=2288追加pair・Moon0を照合し、LOCAL BigInt20goldensの全prefix候補も検査する。受信処理とMP表示の事実はv33reportの固定hashからの継承であり、今回の受信再実行とは呼ばない。

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

[独立LOCAL stage reference](../scripts/official_guardian_payload_auto_oracle.cjs) / [保存report](../data/official-v2/guardian-payload-auto-local-v47.json) はBigIntの21例を収録する。四分の一機会の境界・zero word rejection、単品と複合14/58、MP0/17/99、202の符号と条件付きGP離脱、月のsingleton、地球201＋月239の同一decision自己召喚を含む。reportは56,877 bytes、SHA256 `4400ef264fb7e30f55e42682711dd1fe41d67a3243a05fb806aa4eba3512eb7b`。v46の一次append証拠を固定hashで継承し、今回新しく公式selectorやreceiverを発見・実行したとは主張しない。referenceは記載した抽選stageで止まり、対局全体・response/counter/gift/GFを模倣しない。

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

[実クライアントoracle](../scripts/official_active_attack_oracle.cjs)、[保存report](../data/official-v2/active-attack-client-v48.json)、[native用TSV](../data/official-v2/active-attack-vectors-v48.tsv) はp6/tn decode、fu、iI/gf2 summary、iP cardラベル分岐、fq/aT fresh callerとroster lookupを実行する。400 frame例（296単体＋順序付き16＋動的26＋override36＋card境界26）、fresh46例（expected lookup error22）、decoder10例、人工null source2例を区別した。nativeは443行（expected error22を含む）で公開JSONが全て一致した。

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
