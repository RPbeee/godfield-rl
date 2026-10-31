# 現行公式データによる限定実行器 v9

この実行器の `official-execution-subset-v9` は、**公式公開台帳と確認できたクライアント計算を使う実験用の部分環境**である。scopeは `experimental:194-models;not-official-server-equivalence`。公式サーバーと同じターン進行、補充、抽選、死亡順を再現したという主張ではない。

v1は132モデル/重み308、v2は物理反射6種と属性消去189を加えた139モデル/重み319、v3は遮断9種を加えた148モデル/重み328。v4はFREE5種・属性付替2種・倍化231を加えた156モデル/重み337。v5は跳ね返し8種を加えた164モデル/重み351。v6は能力noneの確率攻撃21種を加えた185モデル/重み372。v7は攻撃反撃181/184を加えた187モデル/重み374。v8は資源反撃187/188を加えた189モデル/重み376。v9はHP吸収4種・同被害53を加えた194モデル/重み381。末尾のv1〜v8計測・検証記録は各版の歴史的な証拠として残し、v9の速度や一致結果へ読み替えない。

一次根拠があるものと、動かすためにこの実行器が定義する規約を分ける。

| 区分 | 内容 |
| --- | --- |
| 公式公開データ | 296モデルの属性・数値・MP・giftRate。旧Flashの数値IDとは別体系 |
| 実クライアントで確認したローカル計算 | `tw/pq/l9/tx/la` の表示威力・属性・MP、visible append、防御適合、HP/MP/CPの加減と不足時の資源連鎖 |
| クライアントの消費表示 | 通常品の消去と空枠、奇跡のused保持、表示枠が空いている場合の補充用空枠、捨てる時の空枠を作らない処理 |
| この実行器の実験規約 | 初期配布/資源/上限、配布対象プール、乱数器、固定入力順の主手番、全効果解決・死亡・補充・次手番の順、通常被害の残余式、終局と報酬、意思決定の打切り |

公式JSのイベントhandlerは、受信した `useAttackItems`、`dealDamage`、`die`、`gift`、`advanceGF` 等を順に表示する。handlerの存在だけでは、それらを権威的サーバーがどの条件・順序で発行するかは確定しない。たとえば通常被害は `event.damage` を受け取って減算しており、サーバーのATK−DEF算式を独立に検証する処理ではない。[一次調査](official-current-rules.md)と[fresh viewの契約](official-state-view.md)も参照する。

## 対象モデルと拒否する機能

[official_match.hpp](../include/godfield/official_match.hpp) の `match_supported` / `match_catalog` がv9の対象を定義する。対象は194モデル、正のgiftRateの合計381。category別はweapons93/armor72/sundries9/miracles20。通常の公式全配布プール237モデル/重み500とは異なり、対象内で正規化する。公式の配布分布とは呼ばない。

v1の基礎集合は、正のgiftRate、命中率0、categoryがweapons/armor/sundries/miracles、abilityがnone/boostHP/boostMP/boostCPのいずれかという条件である。固定済みの現行台帳に対するこの基礎条件が132モデル/重み308を作り、v2はさらに後述7モデルを加える。属性は省略属性を含む全7種類を扱う。確率攻撃は後述v6の21種とv9の吸収98に限り、未対応能力を持つカードの一部roleだけを黙って通常品として使う処理はしない。正確な対象可否は名前の推測ではなく公式モデルIDで照会する。将来別版の台帳へ同じ条件を適用するだけで、自動的に互換範囲を広げる契約ではない。

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

全プレイヤーのraw手札を含め、実行状態に未対応モデル、fake、災い、守護神、混乱、終末設定等があれば拒否する。本人に見えない他者の未対応カードでも、この実行器が将来通常品へ誤変換して進めることはない。`MatchError` の `unsupported` と、不正な入力を示す `invalid` は区別する。

霧・夢・病気・閃光・暗雲・守護神・災い反撃・233/234以外の反応奇跡・取引・終末等を、台帳にあるという理由だけで実行可能とはしない。`official_state` のread-onlyな9人状態表示が扱う範囲と、この部分実行器が受理する局面は別である。

## 初期化と入力状態

`MatchConfig` の既定値は初期手札9、HP40/MP10/CP20、総手札表示上限18、意思決定上限512。HP/MP/CPや9/18には公式表示側の根拠があるが、**この組合せのサーバー開始状態・実配布数・物理容量を対局traceで検証したものではない**。ここでは実験用の初期条件として固定する。usedの奇跡も総手札数へ含める。

設定可能域は意思決定上限1以上、手札上限1〜18、初期手札0〜その上限、初期HP1〜99、初期MP/CP0〜99。初期手札0はテスト等のための実験設定として受理する。

プレイヤー数は2〜9人。入力列の順に扱い、PlayerIdとTeamを保持する。主手番の巡回もこの入力順に固定する。公式サーバーのシャッフルや順番抽選を再現したという意味ではない。seedはこのローカル実行器の抽選を再現するものであり、公式サーバーの乱数列と対応しない。

resetでは先頭プレイヤーをowner、GFを1にし、入力プレイヤー順に初期手札を配る。次ownerは現在ownerの次から巡回して最初の生存者を選び、GFを1増やす。seedを使う配布はこの実行器のSplitMix64と偏りを除いた有界抽選による。方策のサンプリング用乱数は配布用の乱数とは別に渡す。

`reset` は指定プレイヤー列とseedで開始する。`load_scene` は対応範囲内の既知局面を主手番ownerとseed付きで取り込む。raw snapshotを取り込めたことは、その局面が公式サーバーで到達可能と証明されたことを意味しない。

開始時から全員が同じ非Soloチームなら競合相手が無いため、即時に自然終局して全員をwinnerとする。取り込むsceneでも安定した局面の自然終局を先に判定し、全員deadならdrawを認める。HP0なのにdead=falseの中間sceneは、このclosed runtimeに必要な死亡待ちqueueが無いので拒否する。これらもローカル規約である。

`load_scene` は入力のisOverがfalseの安定sceneを受け取り、この実行器の終局規則で結果を判定する。既存のalias/userIdはraw state内に保存し、表示には既存のview規則を用いる。資源0〜99・設定内の総手札数・usedを持てるのは奇跡だけ、といったclosed runtimeの制約も検査する。

## 行動・選択・応答

`Command` はactor、kind、順序付きの本人instance token列、任意の明示targetを持つ。modelIdを物理tokenの代わりに使わない。同じモデルの別コピーと、同じtokenの重複を区別する。kindは `play`、`defend`、`discard`、`pray`。

通常のplayは生存targetを明示する。v6の確率攻撃playだけはtarget無しに限定する。owner/actor/phaseと本人の所有品を検査する。表示モデルの選択・MP・category制約には公開クライアント由来の関数を使う。既存 `official_state` のappend照会だけで済ませず、実行器の現在phaseも検証する。

通常の武器やATK奇跡は、自己対象でも解決待ちのeffectとresponse phaseを作る。確率攻撃では各生存敵への命中を先に抽選し、hitした対象にだけresponseを作る。応答者はそのtargetであり、ownerとactorが同じ人でもphaseを分離する。普通防御では適合する品の選択順・合成と所有権を検証する。v2から物理反射と属性消去、v3から遮断、v5から跳ね返しを扱う。旧版の自己攻撃時の応答省略を暗黙に流用しない。

HP/MP/CPを増やす品は、自己対象なら即時解決する。他人を対象にした場合には応答待ちを作る。v1では空応答だけ、v2から適合する物理反射、v3から奇跡に対する魔法遮断、v5から奇跡への跳ね返しを許す。普通防具で回復量を減らさない。非攻撃の回復へ属性消去を前置することはUIのincoming_is_attack制約により許さない。

prayは本人の手札に武器がなく、補充を受ける空きがあるときだけ許可する。discardは対象の制限を検査して捨て、補充予定を付けない。旧Flashの「捨て枚数に応じた配布」を公式ルールとして移さない。

discardのitemsは擬似モデル「捨てる」自身を含めず、捨てる実instanceを1個以上列挙する。discard/pray/defendはtarget無し、prayはitemsも空である。自分、味方、敵のいずれであっても、通常playは生存targetを明示する。確率攻撃playにはtargetを付けず、明示target付きのaliasは拒否する。対象の敵味方による別制限をサーバーから検証した主張ではない。

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

これらの全敵展開・固定順・命中分布/乱数消費・途中skip/取消・死亡/補充scheduleは**公式server未検証の実験規約**である。公開top-level `chance_rule` に `experimental:targetless-all-alive-enemies-in-input-order;roll-before-response;redirects-no-reroll;skip-dead;cancel-on-origin-death` を明示する。既存 `bounce_target_rule` の未検証表示も維持する。

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

公開top-levelの `counter_rule` は `experimental:actual-hp-loss;survivors-only;selection-order-fifo-before-next-target;skip-dead-source-or-target;single-target;roll-before-response;redirects-no-reroll`。既存のchance_rule・bounce_target_ruleも維持する。

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

公開 `damage_effect_rule` は `experimental:positive-nondark-remainder;current-source-side-effect-before-target-damage;counter-basis-target-damage-only`。HP primitive以外の量・順序・counter基準と死亡接合は現行server未検証である。災い/Flashの最小scope案は将来候補として留め、v9で状態を黙って追加しない。

## 死亡・次手番・終局・報酬

効果を解決してから、このsubsetが定義する順に脱落を確定する。v7の全action batchは前述の受理コマンド境界で自然終局/cutoffとcounter FIFO・残り対象を判定し、補充・次ownerは全action完了後へ遅延する。それ以外は1対象の完了後に補充・次ownerへ進む。反射/跳ね返しを経ても次ownerの巡回起点は元の主手番ownerのまま。deadの人は以後の主手番や生存対象から除く。病気tick、蘇生、死に際攻撃、守護神等をこの間へ推測で追加しない。HP0→脱落→次手番の具体的scheduleはサーバー未検証の実験規約である。

生存者が0人ならdraw、1人ならその人が勝者になる。複数生存者が全員同じ非Soloチームならチーム終局とする。勝利チームのwinnerリストには、先にdeadになった同チームの人も含む。後者はクライアントのending表示 `h7.lV` の集合規則を参考にするが、それ自体をサーバーの勝敗判定器とみなさない。

報酬はactor相対ではなく、入力プレイヤー列に固定したベクトル。winnerへ+1、それ以外へ−1、drawなら全員0。意思決定上限によるtruncationは自然終局と別フラグで、報酬は全員0とする。512という既定上限は公式ゲームの終了条件ではない。

意思決定はplay/defend/discard/prayの各受理コマンドで1ずつ数える。自然終局を先に判定し、未終局で上限へ達した場合にtruncationとする。応答待ちを作ったplayがその上限なら、待機effectと補充予定を取消して打ち切るため、未実行の被害や補充を後から適用しない。消費・MP支払いが既に行われた時点の状態は残す。完了済みの行動でも、打切りが先に成立した場合は補充・次ownerへ進めない。

## APIと再現性の境界

`Match::legal` はこのsubsetの現在局面に対する検査。`step` の不正入力・未対応入力では状態、乱数、意思決定数を変更しない。reset/load/restoreも検査とコピーを済ませてから置き換え、例外時に既存のmatchを一部更新しない。v9の `MatchSnapshot` はversion9を持ち、config、raw state、owner/actor/phase、待機effect、補充予定、乱数・token・意思決定数、終局結果を保持する。旧version1/2/3/4/5/6/7/8を自動移行せず拒否する。`restore` は対応範囲と構造・待機effectの整合を検査後に一括で置き換える。任意のraw sceneの履歴が公式サーバーで到達可能であることまで証明しない。

応答待ちsnapshotには主動作command、全response journal、各現在frameの `RedirectFrame{defender,models,tokens,target}` を特権来歴として保存する。v7はActionBatchのlaunch前状態から全executeを再生し、主動作/防御の選択token重複・普通品消費・奇跡used保持・MP・各人の正確な補充予定に加え、派生counterのモデル/威力/originとFIFOを検証する。counterのモデルを主動作で直接使用したtoken列と取り違えない。`EffectFrame.redirect_rng` やActionBatchのRNG anchor、counter FIFO・journalは再現検査用であり、公開pendingへ出さない。v1の「owner以外の補充予定は必ず0」という条件は、反応者も品を消費する現行には適用しない。

raw snapshotには他人の手札や将来のローカル乱数状態があるため、ポリシーの観測として渡さない。表示は `observe(viewer)` などの投影経路から取得する。snapshotはデバッグ・再現用の特権状態であり、公式の通信スナップショットと同じ形式でも、バージョンをまたぐ永続checkpointでもない。

[official_match_c_api.h](../include/godfield/official_match_c_api.h) の `gfo_match_*` ABI1と [Python `official_match.py`](../python/official_match.py) はこの実行器のsubset・実験scopeを明示し、read-onlyの `OfficialData` / `OfficialState` と区別する。`Match` / `Config` / `PlayerSetup` / `Command` を用い、reset、legal、step、status、observe、外部policy RNGによるsample_actionやrandom_rolloutを提供する。raw scene/checkpointのimport・restoreはこのC/Python公開APIへ追加していない。

v9の `observe` はschema `official-execution-view-v7` を使い、C ABIは1を維持する。rulesetとscopeはv9/194モデルへ更新する。top-levelに `damage_effect_rule` / `resource_counter_rule` / `counter_rule` / `chance_rule`（前述の実験規約）と `bounce_target_rule="experimental:uniform-all-alive-in-input-order;includes-self-and-teammates"` を明示する。実験matchのphase/owner/actor・終局フラグ・意思決定数、fresh board view、元の待機effectモデル/威力、`pending.reflections` に反射件数、`pending.bounces` に跳ね返し件数、`pending.hit_rate` に命中率（通常攻撃は0）、`pending.counter` に現在payloadがcounterかを返す。counter181の75は開始後の表示で、初期全敵chance攻撃とは区別する。属性消去の回数は返さず、現在属性 `element=null` に反映する。次の配布・乱数・他者未使用手札・補充予定の内訳・来歴token・action batchの将来roster/cursor/anchor/journal・counter FIFOは観測へ出さない。対応モデルの照会やローカルランダム実行をできるようにしても、公式オンライン対局へ接続したりコマンドを送ったりする機能ではない。

実行例は `python3 python/official_match.py`。ライブラリパスを別に指定する場合は同スクリプトの引数へ渡す。これは初期化してローカルに進める例であり、公式対局traceの取得ではない。

旧37profileとその台帳・Env・C ABI/Pythonは変更しない。この段階で学習器や学習済みモデルは追加しない。公式完全対応に向けては、一般effect queue、実配布・抽選・死亡・ターン順の一次照合、災いcounter等の未対応能力、9人/チームでの多段効果、守護神、終末等が残る。

## 検証と計測

次節にv9の検証・計測を示し、後続のv1〜v8記録は各版の歴史的証拠として保持する。過去の記録を194モデルのv9の結果へ読み替えない。

### v9の検証と集計

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
