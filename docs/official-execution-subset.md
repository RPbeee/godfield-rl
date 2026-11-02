# 現行公式データによる限定実行器 v29

**公式公開資料のv2同期（2026-11-02）:** [official-v2](../data/official-v2/README.md) に最新版JA/JSの出典を別世代として固定し、ID19/26/117の画像名と117「石板」を反映した。296モデルの数値・能力・curse・giftRate・ID順はv1から変更0。[v1→v2比較](../data/official-v2/upgrade-review.json) を保存し、旧official-v1のpins・crosswalk・legacy37 fingerprintは変更しない。新JSの抽出関数/分岐の再照合は下記v27節に記録する。全JS・全UI・権威的serverの同値性を保証するものではない。過去のfresh一致記録と各版の計測は、当時の取得物に対する履歴として保持する。

この実行器の `official-execution-subset-v29` は、**公式公開台帳と確認できたクライアント計算を使う実験用の部分環境**である。scopeは `experimental:234-models;not-official-server-equivalence`。公式サーバーと同じターン進行、補充、抽選、死亡順を再現したという主張ではない。

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

[official_match.hpp](../include/godfield/official_match.hpp) の `match_supported` / `match_catalog` がv29の対象を定義する。対象は234モデル、正のgiftRateの合計495。category別はtrade3/weapons107/armor78/sundries17/miracles29。通常の公式全配布プール237モデル/重み500とは異なり、対象内で正規化する。公式の配布分布とは呼ばない。

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

全プレイヤーのraw手札を含め、実行状態に未対応モデル、互換クラス外または不正なfake、病気4種/Fog/Dream/閃光/暗雲以外の災い、守護神、混乱、終末設定等があれば拒否する。本人に見えない他者の未対応カードでも、この実行器が将来通常品へ誤変換して進めることはない。`MatchError` の `unsupported` と、不正な入力を示す `invalid` は区別する。

互換クラス外Dream・守護神・自己購入等の未対応取引・終末等を、台帳にあるという理由だけで実行可能とはしない。`official_state` のread-onlyな9人状態表示が扱う範囲と、この部分実行器が受理する局面は別である。

## 初期化と入力状態

`MatchConfig` の既定値は初期手札9、HP40/MP10/CP20、総手札表示上限18、意思決定上限512。HP/MP/CPや9/18には公式表示側の根拠があるが、**この組合せのサーバー開始状態・実配布数・物理容量を対局traceで検証したものではない**。ここでは実験用の初期条件として固定する。usedの奇跡も総手札数へ含める。

設定可能域は意思決定上限1以上、手札上限1〜18、初期手札0〜その上限、初期HP1〜99、初期MP/CP0〜99。初期手札0はテスト等のための実験設定として受理する。

プレイヤー数は2〜9人。入力列の順に扱い、PlayerIdとTeamを保持する。主手番の巡回もこの入力順に固定する。公式サーバーのシャッフルや順番抽選を再現したという意味ではない。seedはこのローカル実行器の抽選を再現するものであり、公式サーバーの乱数列と対応しない。

resetでは先頭プレイヤーをowner、GFを1にし、入力プレイヤー順に初期手札を配る。次ownerは現在ownerの次から巡回して最初の生存者を選び、GFを1増やす。seedを使う配布はこの実行器のSplitMix64と偏りを除いた有界抽選による。方策のサンプリング用乱数は配布用の乱数とは別に渡す。

`reset` は指定プレイヤー列とseedで開始する。`load_scene` は対応範囲内の既知局面を主手番ownerとseed付きで取り込む。raw snapshotを取り込めたことは、その局面が公式サーバーで到達可能と証明されたことを意味しない。

開始時から全員が同じ非Soloチームなら競合相手が無いため、即時に自然終局して全員をwinnerとする。取り込むsceneでも安定した局面の自然終局を先に判定し、全員deadならdrawを認める。HP0なのにdead=falseの中間sceneは、このclosed runtimeに必要な死亡待ちqueueが無いので拒否する。これらもローカル規約である。

`load_scene` は入力のisOverがfalseの安定sceneを受け取り、この実行器の終局規則で結果を判定する。既存のalias/userIdはraw state内に保存し、表示には既存のview規則を用いる。資源0〜99・設定内の総手札数・usedを持てるのは奇跡だけ、といったclosed runtimeの制約も検査する。

## 行動・選択・応答

`Command` はactor、kind、順序付きの本人instance token列、任意の明示targetを持つ。modelIdを物理tokenの代わりに使わない。同じモデルの別コピーと、同じtokenの重複を区別する。kindは `play`、`defend`、`discard`、`pray`、`exchange`、`sell`、`buy`、`accept`、`decline`。exchangeだけが型付きallocationを持つ。

exchangeはtarget無しの自己再配分とし、通常のplayとsellは生存targetを明示し、buyは本人以外の生存targetを明示する。v17の94はtarget無し/既知aliveヒントを受理し、その指定を選択に使わない。確率攻撃playはtarget無しに限定し、v14の232を含むweapon列はtarget無し/任意の既知alive targetを受理して指定をrosterに反映しない。owner/actor/phaseと本人の所有品を検査する。表示モデルの選択・MP・category制約には公開クライアント由来の関数を使う。既存 `official_state` のappend照会だけで済ませず、実行器の現在phaseも検証する。

臼誘引による即時bodyを除き、通常の武器やATK奇跡は、自己対象でも解決待ちのeffectとresponse phaseを作る。確率攻撃では各生存敵への命中を先に抽選し、hitした対象にだけresponseを作る。応答者はそのtargetであり、ownerとactorが同じ人でもphaseを分離する。普通防御では適合する品の選択順・合成と所有権を検証する。v2から物理反射と属性消去、v3から遮断、v5から跳ね返しを扱う。旧版の自己攻撃時の応答省略を暗黙に流用しない。

HP/MP/CPを増やす品は、自己対象なら即時解決する。他人を対象にした場合には応答待ちを作る。v1では空応答だけ、v2から適合する物理反射、v3から奇跡に対する魔法遮断、v5から奇跡への跳ね返しを許す。普通防具で回復量を減らさない。非攻撃の回復へ属性消去を前置することはUIのincoming_is_attack制約により許さない。

prayは本人の手札に武器がない時に許可する。v16は満杯でも受理し、その場合gift0/環境RNG0で通常の手番・cutoffへ進む。v1〜15の空き必須というscope制限は維持しない。discardは対象の制限を検査して捨て、補充予定を付けない。旧Flashの「捨て枚数に応じた配布」を公式ルールとして移さない。

discardのitemsは擬似モデル「捨てる」自身を含めず、捨てる実instanceを1個以上列挙する。discard/pray/defend/exchangeはtarget無し、prayはitemsも空である。自分、味方、敵のいずれであっても、通常playは生存targetを明示する。確率攻撃playにはtargetを付けず、明示target付きのaliasは拒否する。232を含むweapon列と94は前述の任意target規約を使う。対象の敵味方による別制限をサーバーから検証した主張ではない。

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

`Match::legal` はこのsubsetの現在局面に対する検査。`step` の不正入力・未対応入力では状態、乱数、意思決定数を変更しない。reset/load/restoreも検査とコピーを済ませてから置き換え、例外時に既存のmatchを一部更新しない。v29の `MatchSnapshot` はversion29を持ち、config、raw state、owner/actor/phase、待機effect、補充予定、乱数・token・意思決定数、終局結果を保持する。旧version1/2/3/4/5/6/7/8/9/10/11/12/13/14/15/16/17/18/19/20/21/22/23/24/25/26/27/28を自動移行せず拒否する。`restore` は対応範囲と構造・待機effectの整合を検査後に一括で置き換える。任意のraw sceneの履歴が公式サーバーで到達可能であることまで証明しない。

応答待ちsnapshotには主動作command、全response journal、各現在frameの `RedirectFrame{defender,models,tokens,target}` を特権来歴として保存する。v7はActionBatchのlaunch前状態から全executeを再生し、主動作/防御の選択token重複・普通品消費・奇跡used保持・MP・各人の正確な補充予定に加え、派生counterのモデル/威力/originとFIFOを検証する。counterのモデルを主動作で直接使用したtoken列と取り違えない。`EffectFrame.redirect_rng` やActionBatchのRNG anchor、counter FIFO・journalは再現検査用であり、公開pendingへ出さない。v1の「owner以外の補充予定は必ず0」という条件は、反応者も品を消費する現行には適用しない。

raw snapshotには他人の手札や将来のローカル乱数状態があるため、ポリシーの観測として渡さない。表示は `observe(viewer)` などの投影経路から取得する。snapshotはデバッグ・再現用の特権状態であり、公式の通信スナップショットと同じ形式でも、バージョンをまたぐ永続checkpointでもない。

[official_match_c_api.h](../include/godfield/official_match_c_api.h) の `gfo_match_*` ABI2と [Python `official_match.py`](../python/official_match.py) はこの実行器のsubset・実験scopeを明示し、read-onlyの `OfficialData` / `OfficialState` と区別する。`Match` / `Config` / `PlayerSetup` / `Command` を用い、reset、legal、step、status、observe、外部policy RNGによるsample_actionやrandom_rolloutを提供する。raw scene/checkpointのimport・restoreはこのC/Python公開APIへ追加していない。

v29の `observe` はschema `official-execution-view-v27` を使い、Match C ABIは2とする。rulesetはv29、scopeは234モデルとする。top-levelに `overflow_rule` / `death_rule` / `fog_rule` / `disease_item_rule` / `disease_rule` / `buy_rule` / `sell_rule` / `removal_event_rule` / `exchange_rule` / `danger_rule` / `revival_rule` / `random_hp_rule` / `wide_attack_rule` / `mana_weapon_rule` / `repeat_attack_rule` / `removal_rule` / `darkcloud_rule` / `flash_rule` / `damage_effect_rule` / `resource_counter_rule` / `counter_rule` / `chance_rule`（前述の実験規約）と `bounce_target_rule="experimental:uniform-all-alive-in-input-order;includes-self-and-teammates"` を明示する。実験matchのphase/owner/actor・終局フラグ・意思決定数、fresh board view、前述の実行済launch要約 `last_danger`、解決済み除去要約 `last_removal`、解決済み売却要約 `last_sale`、公開購入見積 `purchase` と完了要約 `last_purchase`（両取引eventのoptional overflow_modelは失ったモデルIDだけ）、元の待機effectモデル/威力、`pending.reflections` に反射件数、`pending.bounces` に跳ね返し件数、`pending.hit_rate` に命中率（通常攻撃は0）、`pending.counter` に現在payloadがcounterか、`pending.death` に112の死亡時payloadか、`pending.forced_hit` に暗雲による無抽選必中だったかを返す。counter181の75は開始後の表示で、初期全敵chance攻撃とは区別する。属性消去の回数は返さず、現在属性 `element=null` に反映する。次の配布・乱数・他者未使用手札・補充予定の内訳・来歴token・action batchの将来roster/cursor/anchor/journal・counter FIFOは観測へ出さない。対応モデルの照会やローカルランダム実行をできるようにしても、公式オンライン対局へ接続したりコマンドを送ったりする機能ではない。

実行例は `python3 python/official_match.py`。ライブラリパスを別に指定する場合は同スクリプトの引数へ渡す。これは初期化してローカルに進める例であり、公式対局traceの取得ではない。

旧37profileとその台帳・Env・C ABI/Pythonは変更しない。この段階で学習器や学習済みモデルは追加しない。公式完全対応に向けては、一般effect queue、実配布・抽選・死亡・ターン順の一次照合、互換クラス外Dream、自己購入、9人/チームでの多段効果、守護神、終末等が残る。

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
