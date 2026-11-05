# 現行公式への適合計画

**公式公開資料のv2同期（2026-11-02）:** [official-v2](../data/official-v2/README.md) に最新版JA/JSの出典を別世代として固定し、ID19/26/117の画像名と117「石板」を反映した。296モデルの数値・能力・curse・giftRate・ID順はv1から変更0。[v1→v2比較](../data/official-v2/upgrade-review.json) を保存し、旧official-v1のpins・crosswalk・legacy37 fingerprintは変更しない。新JSの抽出関数/分岐の再照合は下記v27節に記録する。全JS・全UI・権威的serverの同値性を保証するものではない。過去のfresh一致記録と各版の計測は、当時の取得物に対する履歴として保持する。

対象は現行のGod Field全体である。旧Flash参考実装の231種duelは比較対象として保存し、
そのABIやflagを増やす方式で公式版へ改称しない。新しい実装は`godfield::official`に置く。
学習器は今回の対象に含めず、削除した学習実装を復活させない。

## 証拠の区分

次の区分を混ぜず、各fixtureへsourceと確認範囲を記録する。

| 区分 | 確認できること | それだけでは確認できないこと |
| --- | --- | --- |
| 公式配信データ | 公開モデル・表示値・説明文 | サーバーがその値で解決した結果、隠れた抽選 |
| 公開クライアントの関数 | UIの属性合成・威力表示・選択の追加可否・MP表示 | サーバー全体の合法性、消費・死亡・補充順 |
| 公式対局の観測trace | そのbuild・mode・事前条件で実際に観測した結果 | 他の条件への一般化、非公開の内部状態 |
| 旧エミュレーター | 旧実装との回帰・差分 | 現行公式との一致 |

[公式入口](https://godfield.net/)は最大9人を案内し、
[`main.dart.js`](https://godfield.net/main.dart.js)を配信する。
そのclientは[`i18n/ja.json`](https://godfield.net/i18n/ja.json)を別途取得する。
取得日時・URL・HTTP metadata・byte数・SHA256をまとめて固定する。
JSのhashが同じでも別取得の台帳・説明文が変わるため、bundleのhashだけでは同一仕様としない。

初回v1で取得したJSのSHA256は`764a50524e4b6b3f510415da7128abd8ad99dcb87b45b8572d98ddd96889cabd`、
日本語JSONは`03a4782dc6899aeedd9b7986a49bd3c0217b96ac38c0779deb588bc94cca3e7c`。
これは特定の取得物を識別する値であり、将来のURL内容を固定するものではない。

## 最初に固定する現行差分

公式公開台帳は296モデル。通常配布の正のgiftRateを持つモデルは237、合計500。
残りは操作・悪魔・守護神・超常現象を含む。296全てを同じ配布プールに入れない。
categoryはtrade5、weapons107、armor78、sundries19、miracles30、devils5、guardians42、phenomena10。
旧数値IDとの自動同一視を禁止し、公開モデルのimageNameと取得物内indexの対応を保持する。

| 項目 | 現行公開物の内容 | 受入の限界 |
| --- | --- | --- |
| 光 | どの属性でも防御できないという説明とclientの個別防御判定 | 反撃の闇を混ぜた列も公式対局traceで照合する |
| 病気 | 自然悪化5%という説明 | tick・再付与・悪化後効果の順は別途照合 |
| 属性付替 | 現行2モデルはisPlusAtkかつATK2/5、clientは追加威力にも加算 | 実際の消費・元武器との相互作用はtraceが必要 |
| 昇天弓 | 通常ATK1・25%、死亡時値30 | 多人数の死亡queue・対象・発火順は未検証 |
| 回復奇跡 | SpringはHP10・MP7 | 旧216/MP6を現行IDとして使わない |
| 除去 | 説明は神器3個、使用済み奇跡2個。起こした奇跡はホウキ対象外、受信itemsの表示経路あり | v12は逐次抽選等を実験規約で実行。v19で直近解決結果をlast_removalへ投影。server抽選順/empty乱数/履歴寿命は未照合 |
| 破棄 | 武器・太陽のお守り・あぶないウスは不可という説明 | 旧245のskip-noopを現行へ移さない |
| 守護神 | 敵turn後25%、被害時10%で去るという説明 | 旧HP20肩代わり/30%procを採用しない。解決順はtraceで確認 |
| 終末 | 捨てるからささげるへ切替、配布時に悪魔25%という説明 | 開始条件・各悪魔・特殊現象・勝敗との順を照合 |

## 状態と実行器の変更

現legacy coreにはplayers/order/rewardsの2要素固定、`1-seat`、XORでの反射元推定、
単一winner、32個のcounterqueue、相手1人だけを置く観測がある。
公式coreは次を明示的に持つ。

- 有効人数と最大9座席、固定SeatId、teamId、敵味方関係、生存者の巡回順。
- 元のturn owner、itemの不変origin、現在のsource/target、全体攻撃の対象集合と解決cursor。
- 通常item・反撃・多人数死亡時効果・守護神・終末を進める一般effect queue。
- HP0の暫定状態と脱落確定、チーム生存、複数勝者・引分けを含むGameOutcome。
- 一回だけ行うowner tick・守護神機会・補充の進行stage。応答待ちから再開しても繰り返さない。
- 物理個体token、真のモデル、見かけのモデル、保持した奇跡の順序付き個体。

2人だけで成立した反射のsource parityや32queue上限は再利用しない。
全体攻撃の対象をいつ固定するか、途中脱落・味方・自分を含む抽選は別々の規則として照合する。
神器の初期枚数・最大容量・数値上限は現行の証拠で確定し、旧10/16/99を既定の正解にしない。

## 視点と秘匿

`view(viewer_id)`は`actor`から分離する。待機中の預言者、脱落者、観戦者の視点も持つ。
霧では単に現在値を0へ置換せず、unknown maskと最後に知った情報・公開履歴を区別する。
過去の公開行動からの推測は可能だが、現在の隠れた真値を特徴量へ混ぜてはいけない。

夢では物理個体と見かけのモデルを分け、プレイヤー向け候補は見かけから計算する。
内部の真値に対する`legal()`を外部から無制限に問い合わせると情報oracleになる。
UI追加判定、送信された選択、解決時に明らかになる事実を別の契約にする。
真の裏側だけが違い、表示と公開履歴が等しい2状態で、view・候補・未送信時の拒否挙動が
変わらないことを検査する。将来の抽選tapeも同じ非干渉検査に含める。

## C ABIとPython

旧ABIの固定幅はlegacyのまま保つ。公式用には一つのversioned descriptorで人数・容量・
観測schema・行動schema・必要buffer長をqueryし、機能ごとにprefixを追加し続けない。
報酬や結果は`batch × player_capacity`、active maskとteam outcomeを明示する。
行動は物理token、保持token、順序列、対象座席集合、取引承認・配分等を表せる構造にする。
caller-owned bufferの境界、padding、全row事前検証による原子性、版違い拒否をABI試験に含める。

## 適合fixtureと短期検査

fixtureはevidence level、取得物hash、mode、人数・teams、viewer、事前状態の既知範囲、
入力列、明示抽選結果、観測events、期待view差分を記録する。
公式のserver seedを推定してC++のseedと同一視しない。
隠れた値が観測できないtraceでは、その値を推測で埋めず、観測可能なprojectionだけを比較する。
未観測の条件はunknownとして残す。

短期に実装できる検査は以下。

1. 台帳生成を全296行・全fieldで原JSONと照合し、重複・不明enum・欠損default・分布を検査。
2. clientのtw/pq/l9/tx/la/e1/kO等を出典位置付きで固定し、C++のpure kernelへ表形式ケースを追加。
3. setter ATK加算、DOUBLEの順、FREEの直前costと属性扱い、光防御、反応categoryを回帰検査。
4. clientの選択predicateと、人数・手札個体・MP・真値を検証するserver側規則を混同しない試験。
5. 既存差分C01〜C05を現行buildの説明/client/実対局の三列で再分類。
6. 自己管理の対局で収集したtraceをoffline再生し、旧版一致テストとは別target・別集計にする。

最初のpure kernelが通っても、対局状態機械・全神器解決・多人数・秘匿・終末は未完了である。
各項目はデータ、根拠、実装、適合fixtureの4点が揃って初めて対応済みとする。

## 状態・view部分の実装状況

`official_state.hpp` は最大9人の識別子/チーム、手札個体token・真のmodel・fake・usedを分離し、
入力済みsnapshotから通常状態欄・使用済み奇跡欄・自己手札のfresh viewを作る。
`gfo_state_*` とPython `OfficialState` はこの投影と、本人の見かけモデルによる選択照会を公開する。
`raw_snapshot` は全attack tail/events/unknownを含む特権的な原資料であり、方策へ渡すviewとは別である。

元の50,270件に加え、公式clientの実関数・抽出した表示分岐由来の22,140件を照合した。
非公開の真モデル、相手未使用手札、霧下の資源、transport版やuserIdを変えても、
同じ可視情報のviewと選択照会が変わらないことを独立したnative/Python検査で確かめる。
観測の範囲・18枠の描画域・未投影の攻撃/rating/履歴は[表示契約](official-state-view.md)に固定する。

このread-only API自体は対局状態機械ではない。次節の限定実行器とは分け、
取引のサーバー受理条件、一般effect queue・病気tick・守護神・終末の権威的serverとの一致は引き続き照合対象とする。
9人のstate/view対応、限定対局の実行、公式サーバー同等性を同じ完了項目として数えない。

## 限定実行器の段階

`official_match.hpp` / `official_match.cpp` はv1の132モデル・配布重み308、
物理反射6種と属性消去189を加えたv2の139モデル・配布重み319を経て、
遮断9種を加えたv3の148モデル・配布重み328を経て、FREE5種・setter2種・倍化231を加えた
156モデル・配布重み337を経て、跳ね返し8種を加えたv5の164モデル・配布重み351を経て、単純確率攻撃21種を加えたv6の185モデル・配布重み372を経て、攻撃反撃181/184を加えたv7の187モデル・配布重み374を経て、資源反撃187/188を加えたv8の189モデル・配布重み376を経て、HP吸収/同被害5種を加えたv9の194モデル・配布重み381を経て、閃光付与/反撃/解除7種を加えたv10の201モデル・配布重み404を経て、暗雲付与/反撃3種を加えたv11の204モデル・配布重み407を経て、除去2種を加えたv12の206モデル・配布重み409を経て、MP武器58/二回攻撃14を加えたv13の208モデル・配布重み411を経て、全敵化232を加えたv14の209モデル・配布重み412を経て、HP増減202を加えたv15の210モデル・配布重み414を経て、自動復活208を加えたv16の211モデル・配布重み415を経て、杵94/臼209を加えたv17の213モデル・配布重み417を経て、v18で両替3を加えた214モデル・配布重み437を経て、v19で同じプールのまま除去結果を公開し、v20で売却4を加えた215モデル/重み457を経て、v21で購入5を加えた216モデル/重み477を経て、v22で病気4状態/付与6種を加えた222モデル/重み483を経て、v23で168/198を加えた224モデル/重み485を経て、v24でFog4種と観測/対象選択を加えた228モデル/重み489を経て、v25で112を加えた229モデル/重み490を経て、取引配送overflowとFogのforeign SELLを扱う `official-execution-subset-v26` へ進め、v27で規約を変えずにofficial-v2へ同期した。
現行は `official-execution-subset-v56`。既定236モデル/重み499、`include_fate=True` なら通常配布237モデル/重み500を使う。
201/239召喚・canonical42自動行動、限定Earth120/Moon26のopt-in自動payload、外部現象9種と295専用shuffle、
Fate/悪魔の外部入力と専用ローカルsampler、終末閾値/ささげる、予約補充phase4までを実験規約で扱う。
Snapshot56、`official-execution-view-v54`、Match ABI2・Config24/Command184/Status248 byte、phase0〜4が現行契約である。
C Configのbyte6は `external_grants`、byte7は `include_fate`、byte20は `local_guardian_payloads`、byte21は `external_artifacts` の独立したstrict boolで、byte22〜23は0を要求する。
2〜9人/チームで通常行動・確率batchから応答、反射/跳ね返し連鎖・遮断、効果適用、
死亡、補充、次の生存者へ進む実験用対局を実行する。固定座席の報酬、自然終局/打ち切り、
別policy乱数、プログラム内snapshot再開、C/Python実行APIを持つ。

資源更新は公式client由来7,898ケースで照合する一方、ATK−DEFの残余算式、
補充枚数・発行順、即時脱落・GF/次手番の順はまだ公式server traceで確定していない。
それらを明示したローカル規約は[限定実行器の契約](official-execution-subset.md)へ固定する。
未対応モデル/状態は拒否し、通常攻撃へのfallbackをしない。通常配布237/500のopt-inは実装したが、207は通常CommandではなくFate APIで使う。
通常 `sample_action` と従来の `random_rollout` だけでは正の混乱main/phase4を進められず、207のFate起動も生成しない。 新しいtyped choiceとdriverはこの旧入口の制限を変えず、別APIで既存のchance入口へ接続する（v49節）。
外部driverが専用chance helperとstepを使う必要があり、これはplayerへ現象結果や非公開tokenを選ばせることを意味しない。

反射は現在source/targetを交換し元のpayloadとownerを保ち、消去後の無属性を維持する。
原発奇跡を再課金・新sourceへ取得させず、応答者ごとの消費と補充予定をledgerへ記録する。
Snapshot version47は全action journalの特権的token/抽選/反撃来歴を検査し、旧version1〜46を拒否する。公開observeへtoken・乱数anchor・未来RNGを出さない。
自己反射もresponseを継続するのは明示したローカル規約であり、clientの自己target表示だけからserver規則を確定しない。

v3の魔法遮断8種は現行の回復奇跡HP235/CP236も止め、武器遮断233は無属性武器にmatchingする。
MP増加奇跡は現行台帳に無く、categoryで判定する一般predicateからMP回復雑貨の遮断を導かない。
＜壁＞単品は初回/再使用ともMP6、初回はused保持し、補充は空き次第で1または0。v4は直後FREEによる費用0も許可する。
blockは現在payload全取消の終端応答とし、reflection ledgerへ保持しない。各選択の消費・費用・予約を処理後、
未終局/未打切りの場合だけまとめて配布して元ownerから巡回する。この全体順はclientのblock演出だけで証明できず、実験規約である。
v3/v4の公開JSONはofficial-execution-view-v2、v5は跳ね返し件数/規約を加えたview-v3、v6は現在命中率とchance_ruleを加えたview-v4、v7はpending.counterとcounter_ruleを加えたview-v5、v8はresource_counter_ruleを加えたview-v6、v9はdamage_effect_ruleを加えたview-v7、v10はflash_ruleを加えたview-v8、v11はdarkcloud_ruleと現在forced_hitを加えたview-v9、v12はremoval_ruleを加えたview-v10、v13はmana_weapon_rule/repeat_attack_ruleを加えたview-v11、v14はwide_attack_ruleとrepeat規約更新を加えたview-v12、v15はrandom_hp_ruleを加えたview-v13、v16はrevival_ruleを加えたview-v14、v17はdanger_ruleと実行済launchのlast_dangerを加えたview-v15、v18はexchange_ruleを加えたview-v16、v19はremoval_event_ruleとlast_removalを加えたview-v17、v20はsell_ruleとlast_saleを加えたview-v18、v21はbuy_rule/purchase/last_purchaseと確認phaseを加えたview-v19、v22はdisease_ruleとboardの病気状態を加えたview-v20、v23はdisease_item_ruleを加えたview-v21、v24はfog_ruleとforced_hitのviewer別null化を加えたview-v22、v25はdeath_rule/pending.deathを加えたview-v23、v26は取引overflow_modelと規約を加えたofficial-execution-view-v24、v27は静的出典3fieldを加えたofficial-execution-view-v25とした。現行v52のviewは前述のofficial-execution-view-v50である。v18のMatch C ABIはtyped allocation追加のため2へ進め、catalog/state ABI1は維持する。

v4の無料化は各MCの即後FREEペア全てを免除し、MP不足prefixではなく完成列の最終費用を検証する。
壁→FREEは全選択を消費/used保持する契約で、旧版の未消費suffixは採らない。
追加FREEのATK/攻撃属性はskipし、通常DEFと先頭46のATK12は保つ。setterはATK加算と色上書き、倍化は累積×2/NE合成で順序を維持する。
samplerもMC+FREE pairをatomicに提案し、途中のMP不足だけで完成可能な無料列を除外しない。

v5のbounceでは応答者が新sourceになり、対象は全aliveから自己・味方・旧sourceを含めて一様に選ぶ。入力seat順・SplitMix64有界抽選、候補1人でも1draw、反射0draw、自己response継続は公式server未検証の実験規約として公開する。元power/category/abilityとfilter結果、owner、原発MCの既払い状態は保つ。
保持234は5MP、直後FREEなら0MPで再使用できる。旧physical-only `n*18` や固定32ではなく、`decisions-1` と `n*(hand_limit+99/5)`、各seatのMP/gift/消費token会計でredirect履歴を検証する。主動作の乱数anchorから全履歴を再生してtargetと最終RNGを一致させる。
view-v3の `bounce_target_rule` は `experimental:uniform-all-alive-in-input-order;includes-self-and-teammates`、公開pendingは反射件数と跳ね返し件数を分離する。1対象のredirect連鎖中の死亡・補充は無く、これもローカル規約として扱う。

v6はtarget無しの確率playを生存敵の入力順batchとして処理する。自己/同非Soloチームを除き、処理時deadは無draw skip、生存者へ有界100抽選1回、hitだけresponse、redirect再抽選無しとする。効果解決時に死亡を確定し、受理コマンド末尾で自然終局→cutoffを判定する。自動miss/skipはdecision増無しで、launchは最初hitか全missまでdrainしてから判定する。response完了後は未打切りの場合だけ次対象を抽選し、origin死亡なら残り取消、補充はbatch全完了後だけとする。これは原典clientから未確定なserver scheduleを明示した実験規約である。
v6のChanceBatchはv7で全pending play用ActionBatchへ統一する。非再帰launch前state/RNG/counters/commandとresponse journalを内部executorで再生し、対象処理やcounterでaliveが変わる局面も全状態一致を検査する。v7のresponse上限 `n*(2*hand_limit+99/5)+targets.size()` はv8で回復MPを含む予算へ更新する。公開chance_ruleにtargetless/all-enemy/順序/roll-before-response/no-reroll/skip-dead/origin-death取消を示し、roster/cursor/anchor/journal/RNGとnative chance_rolls/hitsは公開しない。

v7の攻撃反撃は実HP減少>0の生存者だけが選択順に生成し、181は火/1倍/75%、184は土/2倍/必中とする。category armorを保ち、通常DEF/filterと190だけが適合。counter FIFOは次の初期chance対象より先、childcounterは末尾、開始時source/target死亡なら無draw skip。元owner死亡で残り初期chanceを取消しても、生存条件を満たす既予約counterは続ける。全action後だけalive補充/owner巡回とし、trigger/HP基準/順序はcounter_ruleに明示するserver未検証の実験規約である。
ActionBatchのcounter生成数/予約数はn*hand_limit以下、nativecounter集計とfutureFIFO/journalは非公開。raw×2の指数的威力を採らず、HP0〜99から各生成power<=198を保つ。全beforestate+原発/responseの完全replayで現在state/RNG/MP/gifts/frame/FIFOを比較する。

v8の187は実HP損失×2のMPを生成者selftargetへ明示responseで回復し、188は実HP損失額を旧incoming.sourceから現在counter.sourceへCP徴収する。power0/armorNE nonattackなので190だけが適合し、MP99cap/CPdebit→fullnominalcredit/不足連鎖を処理する。対象・自己response・生成基準はexperimental、資源primitive順は一次根拠ありと区別する。resource-debtから新counterは作らず、card guardian metadataでplayerguardianを召喚しない。
回復MPによりpaid234の旧19回/人上限は不適切で、response予算は `2*n*hand_limit+floor(99*n*(hand_limit+1)/5)+targets.size()`、counter生成/予約はn*hand_limit以下とする。初期MP<=99*n・各187実増<=99・全counter生成数から導き、全execute再生で資源/FIFO/journalも照合する。resource_counter_ruleを公開し、counter_ruleのtarget表現はsingle-targetへ更新する。

v9は27/45/98/216吸収と53同被害を通常damage bodyへ閉じて追加する。正nonDark残余rawの現在source副作用を先に、targetdamageを後に適用し、selfalias逐次clamp/全body後dead確定/target段階だけのcounter基準を明示する。現行clientが受信HPイベントを更新する算術と、参照Flashを参考に採る量・順序の実験規約を分ける。現scopeではDark吸収/53へ到達せず、合法例を捏造しない。
98は木75%の既存chance、216は保持MP10/FREEを扱い、新RNGやMP予算変更は無い。damage_effect_ruleを公開し、派生counterへの能力非継承・source側HP変動からcounter非生成を全replayで検査する。v9には状態を含めず、v10で70/185/224と解除199/200/237/238を追加する。閃光の1枚応答制限をlive legality/samplerへ結び、FREEより先に検査する。付与trigger/致死時/持続/反撃FIFOは実験規約、v10当時の解除4種は閃光だけの閉じた状態集合で実行し、他curse/重複Flash/fake等を拒否した。v11は後述のFlash/DarkCloud集合へ広げ、全action replayで状態と選択制限を照合する。

v11は41/186/223暗雲を加え、percent初期対象と181counterの活性化時に無drawforcedhitとする実験規約を公開する。元hit_rate/forced_hitをredirectで維持し、recheckしない。実draw成功hitsとforced累計を分け、miss=rolls−hitsの定義を保つ。Snapshot/ActionBatch anchorへ両forced累計を加えて完全replayし、将来/累計RNG情報を公開viewへ出さない。
状態集合をFlash→DarkCloudのcanonical順へ広げ、再付与idempotent、mildFlashのみ/all両方を解除、all解除まで持続/no tickとする。JA/ENの必中説明・clientのhit演出と、適用時点/無draw/持続のserver未確認規約を分ける。病気・霧/夢・他curse反撃は追加せず拒否する。

v10検証範囲の訂正：専用native Flash2関数がmainへ未登録だったため、そのcounter FIFO/canonical偽造/late rollback等をv10実行済みとした範囲は撤回する。CTest件数・C API/Python18ケース・大規模benchの記録値は維持し、v11で登録を修正して初実行し、全source/test最終freeze後の検証でもfixture修正無しでRelease/SANとも通過した。 GNU/Clangではofficial_match native test targetに `-Wunused-function -Werror=unused-function` を追加し、同様の未使用テスト関数をcompile errorにする。

v12は205unused最大3/206usedMC最大2を加える。現行説明とusedMiraclesNoteを根拠に対象数/used除外を固定し、inventory順eligible再構築・逐次without replacement・empty0draw/sole1draw・stable erase・除去gift0は実験規約として分離する。使用品/190の既予約giftとそのRNGは残し、206は付与済みcurseを解除しない。
最終bodyをatomicに処理するため、pending replayは除去前の消費/反射/RNGとpayload上限を照合する。完了後任意rawstateの到達履歴まで証明せず、決定的replayはbody抽選と以後giftの一致を検証する。v12〜18で欠けていた公開remove event結果はv19のlast_removalへ投影する。直近の解決済みmodels以外の手札、slots/RNG・候補母数は公開しない。

v13は58/14を加え208モデル/重み411（97/74/13/24）。58はclient preview/useAttackItemsと同じ完成列費用後MP×2を基底に順序boost/Auraし、元MPを一度だけlaunchで全消費、0MPでもweapon response、redirect再計算/再支払い無し。全journal replayはbatch初期ownerMPを明示して同じ威力を再構成する。
14単体は原発fresh payloadを同targetへ二度、支払い/消費一度、counter FIFO先行・dead target skip・origin死亡残り取消・自然終局/cutoff優先・全action後giftを採る。二回の説明以外のscheduleはserver未確認で、旧Flashにもrepeat処理の根拠が無い。6damage一括ではない。
metadataは `mana_weapon_rule='experimental:base-2x-mp-after-composed-miracle-cost;ordered-boosts;consume-all-mp-once-at-launch;redirects-no-repayment'` / `repeat_attack_rule='experimental:two-fresh-payloads-to-same-target;single-consumption-and-payment;counter-fifo-before-next-hit;dead-target-skip;origin-death-cancels-rest;gifts-after-whole-action'`。Snapshot13は旧1〜12拒否/view-v11/C ABI1、新RNG/MP増加源なしで既存予算を維持する。原発MP後にcounter MPが増えても再計算せず、hit1のfilter/redirectをhit2へ持ち越さないことを検証する。

v13最終source/test freeze後の全Release CTest129件/20.39秒・ASan/UBSan89件/51.58秒、focused Release3件/0.94秒・SAN2件/1.52秒が通過した。Python25ケース・native零引数37関数main明示登録、最終2build/前段3ログwarning/error0、公式assetsのfresh hash/296 facts一致を確認。1/4 workersとRelease/SAN各1,000試合の全決定的fieldも一致した。独立速度と集計は[限定実行器v13記録](official-execution-subset.md#v13の過去の検証と集計)へ分け、公式server同等性や版間速度改善率の証拠とはしない。

v14は232＜蜃気楼＞（MP5/isPlusAtk/NE/ATK0/率0）を追加し209モデル/重み412（97/74/13/25）。単独不可・必中weapon列のみ追加、通常+0/NE合成・各copy直後FREEは公開clientの計算/選択根拠がある。公開UIのtarget保持に合わせ、ローカル入力はtarget無し/任意の既知alive targetを受理するが、指定は表示anchorでrosterへ影響しない。
launch alive敵input順固定、14は全roster2pass、58はlaunch費用後MP基底固定、拡大は冪等/各copy支払い別、chance roll/forced無し/FIFO先/dead skip/origin死亡取消/全action後giftを実験規約にする。旧WIDEの100%化は使わない。target server受理/順序/無drawは未検証として分ける。
metadataは `wide_attack_rule='experimental:initial-alive-enemies-in-input-order;optional-alive-target-does-not-alter-roster;repeat-by-full-roster-pass;single-payment;counter-fifo-before-next-target;dead-target-skip;origin-death-cancels-rest;gifts-after-whole-action'` / 更新した `repeat_attack_rule='experimental:two-fresh-passes-over-initial-targets;single-consumption-and-payment;counter-fifo-before-next-hit;dead-target-skip;origin-death-cancels-rest;gifts-after-whole-action'`。最大targets2*(n−1)=16なのでresponse bound最大3,725、Snapshot14旧1〜13拒否/view-v12/C ABI1。全journal replayで二巡とframeごとのfilter/redirect独立、消費/費用一度、任意target指定の非干渉を検査する。

v14は全Release CTest129件/19.09秒・ASan/UBSan89件/50.23秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、Release/SAN/bench/SAN bench/bridgeの5ログはwarning/error0。focused Release3件/0.81秒（native0.32/C API0.01/Python0.47秒）・SAN2件/1.17秒（native0.83/C API0.34秒）が通過した。Python MatchはAST確認27ケース、native零引数40関数は全てmainへ明示登録を確認した。公式assetsのfresh取得hash/296モデルfactsも一致した。 独立速度と大規模集計は[限定実行器v14記録](official-execution-subset.md#v14の過去の検証と集計)へ分ける。

v15は202ドキドキ涙（sundry/NE/power0/値10/p1w2）を加え210モデル/重み414（97/74/14/25）。launch bounded2一度、0heal+10/1damage−10のsigned payloadを前公開、反射で固定/self即時/他者190-only/negativeもnonattack、HPclip後の死亡・全action後giftを実験規約とする。現行表示はHP+10またはdamage10までで、確率/抽選時点/前公開・動的attack.atkを含むserver受理は未確定。旧237のlaunch/COMMAND.hpは参考に限定する。
`random_hp_rule='experimental:launch-once;uniform-plus10-or-minus10;zero-heals-one-damages;fixed-through-reflections;nonattack-no-defense-or-counters;hp-clips-0-99;gifts-after-action'`。Snapshot15旧1〜14拒否/view-v13/C ABI1、launch前anchorから一度だけdrawし、純粋factoryへ固定deltaを渡して全replayする。両符号・0/99clip・反射no-reroll・DEF/filter/counter拒否・cutoff・forged delta/RNGの拒否を検証し、DarkCloud非干渉は専用fixtureではなくsource auditで確認する。未来RNG/anchorは観測へ出さない。

v15は全Release CTest129件/18.87秒・ASan/UBSan89件/49.59秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、release/sanitize/bench/SAN bench/native/bridge-fixの6ログはwarning/error0。focused Release3件/0.63秒（native0.08/C API0.01/Python0.53秒）・SAN2件/1.00秒（native0.68/C API0.32秒）が通過した。Python MatchはAST確認29ケース、native零引数43関数は全てmainへ明示登録を確認した。公式assetsのfresh取得hash/296モデルfactsも一致した。 独立速度と大規模集計は[限定実行器v15記録](official-execution-subset.md#v15の過去の検証と集計)へ分ける。

v16は208太陽のお守りを加え211モデル/重み415（97/74/15/25）。公開説明のHP0→HP+10、main/discard禁止、指定instanceのee(null)消費/補充用空枠はclient根拠がある。実gift数/最初copy/生存人数gate/死亡・counter順は未確認であり、旧244のgift0・手動discard可を移さない。
全body後input順・各時点HP0&&!dead/not.dead>=2・最初copy一枚/境界・HP+10/遅延gift1、復活前被害量/復活後survivor counter、満杯no-weapon祈りgift0/環境RNG0進行を実験規約にする。`revival_rule='experimental:post-body-input-order;hp0-and-not-dead;two-not-dead-required;first-copy-once-per-boundary;hp-plus10;one-deferred-gift;counter-basis-pre-revival-loss;passive-and-undiscardable;full-hand-prayer-no-draw'`。Snapshot16旧1〜15拒否/view-v14/C ABI1、privileged revivals/Batch anchorを全replayで照合し観測非公開、budget3,725/MP源無し/途中gift無しを維持する。複数copy/両HP0の非対称/ownerと座席順の分離/202negative/53両者HP0/反射・counter継続/terminalcutoff/満杯祈りを検査し、202統計に復活HPを混ぜない。CP債務とWideへの208接合は専用native fixtureではなく、共通resolve→finalize_deathsと次対象のdead判定をsource auditして確認する。

v16は全Release CTest129件/17.77秒・ASan/UBSan89件/47.15秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、release/sanitize/bench/SAN bench/native/bridgeの6ログはwarning/error0。focused Release3件/0.62秒（native0.08/C API0.01/Python0.53秒）・SAN2件/1.21秒（native0.90/C API0.31秒）が通過した。Python MatchはAST確認31ケース、native零引数46関数は全てmainへ明示登録を確認した。公式assetsのfresh取得hash/296モデルfactsも一致した。 独立速度と大規模集計は[限定実行器v16記録](official-execution-subset.md#v16の過去の検証と集計)へ分ける。

v16検証範囲の訂正：C APIの固定seed照会6本の開始値を誤って275に変更し、条件seed<64へ入らなかったため、閃光解除・暗雲forced・ホウキ・石けん・MP武器・HP増減の各64回、計384照会がv16では未実行だった。この384件をv16で実行済みとした範囲は撤回する。全Release129件/SAN89件の通過・他assert・native/Pythonの相当照会・大規模計測値は事実として保持する。v17では6loopを `seed{}`（0）へ戻し、`fixed_query_probes==384` の完走必須assertを追加した。Wide+FREE探索も開始275から0へ戻すが、こちらはv16でも275以降で探索を実行していた。v17の修正後Release/SANで384照会完走を再検証して通過した。過去のv16検証範囲を遡って増やさない。

v17は94光ATK30と209attractDanger99を加え213モデル/重み417（98/74/16/25）。現行の99damage・指定臼instanceのee(null)消費/補充用空枠・通常追加MCの許可と、旧999/臼非消費/列長1の相違を一次と分ける。臼入りdefense paneがresponse選択分岐を避けるUI構造から、server拒否やgift数まで断定しない。
任意aliveヒントを無視/input順最初aliveholder・copy/無臼時全alive uniform draw1、基底99→順序合成/臼一枚消費gift1/0DEF応答無し、無臼時基底30/自己もresponse、redirect再選択無しを実験規約にする。94+232だけはclientが許しても優先仕様未確認でruntime unsupported（-5）、他の対応boost/MC/FREEは許可。
`danger_rule='experimental:launch-once;first-alive-mortar-in-input-order;otherwise-uniform-all-alive;optional-alive-target-ignored;base99-before-ordered-boosts;consume-one-mortar-with-deferred-gift;attracted-body-no-response;redirects-no-reselection;wide-combination-unsupported'`。Snapshot17旧1〜16拒否/view-v15/C ABI1。last_dangerの実行済origin/target/models/mortar/power/elementと次main clear/response保持を検査し、token/RNG/未選択holderは非公開にする。draw有無/初期copy/順序合成/反射不変/臼消費/復活/205除去/unsupported atomicを全journal replayへ接合する。

v17は全Release CTest129件/18.30秒・ASan/UBSan89件/48.50秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、release/sanitize/bench/SAN bench/native/bridgeの6ログはwarning/error0。focused Release3件/0.76秒（native0.32/C API0.01/Python0.43秒）・SAN2件/1.25秒（native0.93/C API0.31秒）が通過した。Python MatchはAST確認33ケース、native零引数49関数は全てmainへ明示登録を確認した。公式assetsのfresh取得hash/296モデルfactsも一致した。修正C APIの6本×64照会はRelease/SANで `fixed_query_probes==384` の完走を確認した。 独立速度と大規模集計は[限定実行器v17記録](official-execution-subset.md#v17の過去の検証と集計)へ分ける。

v18は両替3（trade/p5w20）を加え214モデル/重み437（trade1/98/74/16/25）。UIの0〜99/HP0/sum保存、wire mp/cp、受信source3値直接代入、単品/MP0/物理空枠を根拠に、target無し自己allocationを作る。MPfeeを価格5から導かず、同値再配分も許す。
新kind4/owned3一個/typed allocation各0〜99・配分前sum保存→物理消費gift1→direct assign→既存208/死亡/自然終局cutoff→gift/手番とする。scheduleは実験規約 `exchange_rule='experimental:single-owned-exchange;targetless-self;three-resources-0-99;sum-conserved-before-revival;zero-mp-fee;no-response;one-deferred-gift;death-and-revival-before-gift'` に明示する。復活後の最終sumと配分前sumを混同せず、policy RNGだけで配分を選び環境drawはgiftだけ、各batch初期MP<=99を保ちresponse bound3,725を維持する。
Snapshot18旧1〜17拒否/view-v16、Match ABI2 command184（旧168へ16byte typed allocation追加）/config24/status248、catalog/state ABI1/legacy37維持。旧Match clientの構造体呼出前version/size拒否と移行、配分型/range/sum/他kind混入/target/duplicate token/HP0救済/死亡cutoff/環境RNG/late rollbackを検査する。sacrificeは終末機構無しでは追加せず、94+Wide gapも維持する。

v18は全Release CTest129件/17.79秒・ASan/UBSan89件/46.30秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、release/sanitize/bench/SAN bench/native/bridgeの6ログはwarning/error0。focused Release3件/1.08秒（native0.29/C API0.02/Python0.77秒）・SAN2件/1.36秒（native1.00/C API0.35秒）が通過した。Python MatchはAST確認36ケース、native零引数52関数は全てmainへ明示登録を確認した。既存C API384照会guardも完走し、公式assetsのfresh取得hash/296モデルfactsも一致した。 独立速度と大規模集計は[限定実行器v18記録](official-execution-subset.md#v18の過去の検証と集計)へ分ける。

全効果の一般キュー、互換クラス外Dream・取引server挙動・Earth/Moonの一般神器/奇跡選出・207/残る現象・悪魔・終末と、
公式の消費/補充/死亡/進行の実対局照合は引き続き未完了である。
限定対局が動くことを、公式サーバー全体との同等性の証明として扱わない。
v1の132モデル、v2の139モデル、v3の148モデル、v4の156モデル、v5の164モデル、v6の185モデル、v7の187モデル、v8の189モデル、v9の194モデル、v10の201モデル、v11の204モデル、v12の206モデル、v13の208モデル、v14の209モデル、v15の210モデル、v16の211モデル、v17の213モデルとv18の214モデルの計測・checksumは過去の証拠として保持し、現行v47の236モデルの検証とは分ける。


## v19：直近除去結果の公開と検査

v19はモデル/重みを増やさず214/437（trade1/98武器/74防具/16雑貨/25奇跡）を維持し、除去結果の公開gapを解消する。rulesetは `official-execution-subset-v19`、Snapshot19（旧1〜18拒否）、公開schemaは `official-execution-view-v17`。Match ABI2/command184/config24/status248、catalog/state ABI1・legacy37は維持する。

`last_removal` はnullまたは `{origin,source,target,model,removed_models}`。originは原発者、source/targetは反射後を含む解決時の現在座標、modelは205または206の単一整数ID、removed_modelsは実際の抽選順の整数ID配列で同モデルのコピー重複を残す。最終bodyが解決したときだけ作り、候補0の解決も空配列[]のeventを残す。使用品自体の消費や後のgiftをremoved_modelsへ混ぜない。

次のmain commandの受理で消去し、不正/失敗commandでは保持する。response処理自体は消去契機ではない。v19〜24の205/206は単一bodyのaction完了時にeventを作り、response phaseとeventは同居しなかった。v25以降の後続death workは別に扱う。reset/load_sceneで消去する。解決後のcutoffではeventを残し、未解決でcutoffした場合はnullのままとする。token/slot・RNG・元eligible数・非除去の他者手札を公開しない。これは全履歴の蓄積ではない。

公開 `removal_event_rule` は `experimental:resolved-body-only;public-removed-models-in-draw-order;duplicates-and-empty-preserved;clear-on-next-accepted-main;no-tokens-or-rng`。他者が対象でもeventを受信したviewerに除去品を表示する経路には公開clientの根拠がある。ただし全参加者/観戦者へのserver配信や空結果eventの必ずの発行は確認していない。ローカル抽選順をevent順とすること・履歴寿命・cutoffの保持は**公式server未検証の実験規約**である。配布プール、除去抽選、資源/死亡/gift/手番の遷移はv18から変更しない。

復元時はdecisions>0、全endpointが既知、model205/206、countが各3/2以下、対応モデルIDだけ、206なら奇跡のみを検査する。v19〜24ではresponse phaseのeventも拒否していた。完了eventとlast_danger/last_sale/last_purchaseの同居は拒否する。完了raw snapshotは構造整合を確認するだけで、その公開結果が過去の公式またはローカル対局で実際に起きたことまで証明しない。pendingはevent無しを要求し、既存の全action replayを維持する。


v19は全source/test最終freeze後のfull Release CTest129件/21.01秒・ASan/UBSan89件/51.62秒が通過した（各 `-j4` 同時、rolloutとの競合あり）。最終Release/SAN full buildはwarning/error0。focused Release3件/1.27秒（native0.46/C API0.02/Python0.79秒）・SAN2件/1.18秒（native1.03/C API0.15秒）が通過した。native零引数55関数のmain明示登録/unused guard、Python AST38ケース、Release/SAN双方の `fixed_query_probes==384` 完走を確認した。公式配信assetは上記の差分確認であり、fresh hash一致とは記録しない。

| v19人数 | 試合数 | 意思決定数 | 遷移checksum | 解決済み除去event | 空結果event | 除去event checksum |
| --- | --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,839,935 | `4941009284767353250` | 10,194 | 4,120 | `16555731515668663194` |
| 9 | 10,000 | 2,428,289 | `11062027547654796276` | 7,817 | 2,655 | `13048935337781861835` |

seed42の2人100,000試合/9人10,000試合を各1 worker/4 workersで実行し、worker数・時間以外の全fieldが一致した。v18 baselineともruleset・時間等の計測fieldを除く既存fieldがchecksumまで全一致し、除去結果の公開追加が遷移/抽選/既存集計を変えないことを確認した。全試合の終了snapshotを復元できた。`removal_events=hand_removal_resolved+used_removal_resolved` と公開removed_modelsの累計数が実除去コピー数に一致するguardも通過した。空結果は0候補でも解決したbodyのeventで、未解決cutoffを含めない。Release/SAN seed42各1,000試合も時間以外の全fieldが一致し、v18の既存非時間fieldとも一致した。遷移checksumは2人 `1463939626686470704` / 9人 `375287159278559213`、新除去event checksumは2人 `17881167250890207477` / 9人 `11841601995346163861`。今回はquiet速度を再計測せず、v19の速度値や改善倍率を主張しない。

詳細は[限定実行器v19記録](official-execution-subset.md#v19の過去の検証と公開除去結果)。


## v20：売却4の限定実行と未対応配送

v20は4売る/SELL（trade/sell/価格5/giftRate20）を加え、215モデル/重み457（trade2/weapons98/armor74/sundries16/miracles25）とする。rulesetは `official-execution-subset-v20`、Snapshot20（旧1〜19拒否）、公開schemaは `official-execution-view-v18`。Match ABI2のcommand184/config24/status248は変更せず、新kind sell=5を追加する。catalog/state ABI1とlegacy37を保持し、学習器を追加しない。

`Command(actor, kind="sell", items=(sell_token, commodity_token), target=target)` は主手番で、本人所有の物理4と未使用商品一個の順を要求する。二tokenは別物、生存targetを明示し、自己/味方も許可、allocationは無し。used奇跡は商品不可だが、武器・208/209・別コピーの4・未使用奇跡は売れる。play4や追加FREE列へ変換しない。MP費用0で、価格5は操作料ではない。

原発時に4と商品を物理消費し、操作4だけにgift1を予約、商品の補充は無し。奇跡の商品もused保持や取得効果を起こさない。他者宛はtrade/power0/無属性/amount=商品priceのnonattackとしてresponseを作り、空応答または190万能反射だけを許す。自己宛は即時解決。反射で商品とpriceを固定したまま現在source/targetを交換し、再消費しない。

最終bodyでは現在targetのCPから名目priceを引く（不足はMP→HPへ波及）→現在sourceのCPへ名目price全額を加算（上限99、実所持CP額で制限しない）→現在targetへ商品の新token/unused instanceを直接渡す→208復活/死亡→自然終局/cutoff優先→既予約gift/次ownerとする。自己aliasもこの逐次順で、CP不足のHP損失を後のCP加算が戻すことはない。直接配送は環境RNGを使わず、通常の後続gift抽選と区別する。受け取った208による救済もこの順のローカル規約である。

v20〜25では配送先が実物で満杯なら明示unsupported（-5）でatomicに拒否し、samplerも除外する。自己売却は二品消費後の空きを考慮する。現行clientにはserver指定overflowItemの処理があるが、実物のeviction選択は未確定である。旧Flashの満杯配送失敗を無視して料金だけ処理する挙動を流用せず、満杯配送まで含む完全SELL対応とは呼ばない。

`last_sale` はnullまたは解決時の `{origin,source,target,model,price}`。modelは商品IDで、originは原発者、source/targetは現在の解決座標。解決時のみ作り、次main受理/reset/loadでclear、不正/失敗時は保持、解決済みcutoffでは残し未解決cutoffではnullにする。last_danger/last_removal/last_purchaseと排他、v20〜24ではpending/response phaseにも完了eventを置かなかった。現在もpurchase quoteとの同居は禁止する。token・配送先の他手札・RNGを出さず、完了raw snapshotの構造検査を全過去履歴の到達証明とみなさない。

v20〜25時点の公開 `sell_rule` は `experimental:one-unused-commodity;explicit-alive-target;zero-mp-fee;one-operation-gift;foreign-response-and-reflection;buyer-debit-before-current-source-credit;fresh-token-delivery-before-death;full-recipient-unsupported`。商品の232/Aura231/MP武器58/杵94/復活208等のabilityを売却payloadの構成時に実行・継承せず、単一対象・power0・属性無し・商品priceだけを用いる。配送後208の独立した救済hookは別に扱う。商品fakeも既存unsupportedのまま。構造体は同じABI2でも、clientは新opcode5の認識とruleset20のloader照合が必要である。

直接配送後の予約giftは `min(old_gift, hand_limit−new_items_count)` へ防御的に正規化する。実物を追い出す処理ではない。現scopeの正常経路では原発二品消費/gift1、190消費/gift1が作る空きにより通常は超過しない。実物満杯のunsupportedをこのclampで回避しない。

ベンチの `sell_plays` は操作回数、`sell_resolved` は自己即時/他者空応答で最終bodyが完了した数（190反射途中は除く）、`sell_nominal_cp` は名目price総額で実CP支払/HP損失/加算clip量ではない。`sold_miracles` は配送商品が奇跡だった件数でcast/学習ではない。`sold_miracles<=sell_resolved<=sell_plays` を確認する。v20の `physical_used` はSELL一回につき操作4だけ+1、商品は移送として除外し、通常play/defend/exchangeの選択使用や奇跡first/recastへ混ぜない。


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

詳細は[限定実行器v20記録](official-execution-subset.md#v20の過去の検証と売却集計)。


## v21：購入5の確認・返却・公開境界

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

詳細は[限定実行器v21記録](official-execution-subset.md#v21の過去の検証と購入集計)。


## v22：病気の根拠・owner境界・非公開tick来歴

v22はonDamage武器30/33/48/62と状態奇跡219/220の六モデルを加え、222モデル/重み483（trade3/weapons102/armor74/sundries16/miracles27）とする。病気はCold/Fever/Hell/Heavenの4状態。v22時点では168のselfCurseと198のboostMPAndAddCurseは未対応で、関連八モデルを一括実装していなかった。rulesetは `official-execution-subset-v22`、Snapshot22（旧1〜21拒否）、公開schemaは `official-execution-view-v20`。Match ABI2の幅・opcode0〜8・purchase phase3/finished2、catalog/state ABI1・legacy37は維持し、学習器は追加しない。

一人の病気は一種類とし、Flash/DarkCloudとは独立に持つ。病気が無ければincomingを付け、既存病気があれば `max(incoming_rank,current_rank+1)` へ進める。既にHeavenへ病気を再付与するとHP0に落とし、Heaven状態を保つ。Flash/DarkCloudの付与はこの病気再付与と見なさず、Heavenfallを起こさない。これらの厳密なrank/落下処理はローカル実験規約である。

30はNE/ATK8/Hell、33はNE/ATK9/Cold、48はNE/ATK13/Cold、62はStone/ATK1/Coldで、正の通常target HP実被害に応じて既存onDamage経路で付与する。219はCold/MP6、220はHeaven/MP15の状態奇跡でused保持・FREE・適合する反射/遮断/跳ね返しを扱う。再付与でのHeavenfall分をcounter基準へ足さず、既存のregular target damage段階の実損失だけを使い、208救済後に生存していればsurvivor counterの対象となる。

mild解除199/237はCold/Fever/Flashを取り、Hell/Heaven/DarkCloudを残す。all解除200/238は対応する病気4種とFlash/DarkCloudの全六種類を対象とする。霧/夢まで新たに実行可能にする意味ではない。

元ownerの全actionが完了してadvanceへ入る冒頭でだけ病気tickを行う。ownerが既deadまたは病気無しならdraw無し。対象ならbounded100を一度引き、roll<5で一段悪化→変化後のCold HP−1/Fever−2/Hell−5/Heaven HP+5を適用する。Heavenからの悪化はHP0へのfallで、そのtickのhealはしない。次に208救済/死亡→自然終局→既予約gift→次ownerへ進む。固定5%を採用し、旧FlashのworseChance増加式を使わない。

body側の自然終局/decision cutoffが先に成立した場合はadvance/tickへ入らない。完成bodyでのcutoffでもtick無し。反射・連撃・全体攻撃の各frame間、counter FIFO途中、BUY見積中にはtickしない。BUYのaccept/decline等で全actionを完了した場合の対象は現在buyerではなく元ownerである。ownerが生きたまま同action中に208救済された場合も、この全action完了の境界で判定する。これらの対象・抽選順・境界・gift scheduleは公式server未確認の実験規約である。

nativeの特権 `last_tick` はHP/病気before-after・worsened/fallen・revival_tokenを記録し、body被害/202統計・除去・tickによる208消費を分離する。内部tick record・token・RNGを公開JSONへ加えず、公開するのはdisease_ruleとboardの病気/資源状態だけ。完了raw snapshotの構造検査は全公式履歴の到達証明ではない。

v22/23時点の公開 `disease_rule` は `experimental:one-disease-plus-flash-and-darkcloud;reapply-max-incoming-or-next;heaven-reapplication-hp0;owner-only-after-whole-action;natural-and-cutoff-before-tick;fixed5-percent-before-hp;post-upgrade-tick;post-tick-revival-before-gifts`。v22/23のraw stateは病気最大一種→Flash→DarkCloudのenum順・重複無し、最大三curseのcanonical集合だった。v24のFog拡張は後述する。tick recordは次main受理でclearし、現在の公開boardから過去のroll/tokenを復元して返す機能は作らない。 現行clientの病気表示は新たな受信curseで旧病気一種を置き換え、非病気Flash/DarkCloudは独立に残す。旧FlashのHeaven中に非病気を付けても転落させる挙動は流用しない。

ベンチの `disease_ticks` / `disease_worsened` / `disease_falls` はtick/自然悪化/Heavenfall件数、`disease_tick_revivals` はtick由来208救済数。`disease_hp_gained/lost` は復活前のtick HP増減を集計し、救済時はrawAfter=0として208の+10を除き、Heavenfallの実HP損失も含める。累計 `revivals` はbody由来とtick由来を含む。HP増減202のbody集計からtick HPdeltaとtick救済を外し、205/206のtoken消失数から同targetのtick208消費を除いて公開除去eventと照合する。これらの内部会計のためにlast_tick/tokenを利用しても、公開JSONには出さない。


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

詳細は[限定実行器v22記録](official-execution-subset.md#v22の過去の検証と病気tick集計)。


## v23：自己病気とMP/Heavenのbody接合

v23は168熱狂仮面（Fire armor/普通DEF10/selfCurse Fever/価格10/giftRate1）と198天国草（NE sundry/MP+20とHeaven/価格20/giftRate1）を加え、224モデル/重み485（trade3/weapons102/armor75/sundries17/miracles27）とする。MP費用/guardianはどちらも無い。rulesetは `official-execution-subset-v23`、Snapshot23（旧1〜22拒否）、公開schemaは `official-execution-view-v21`。Match ABI2の幅・commands0〜8・purchase phase3/finished2、catalog/state ABI1・legacy37を保持し、学習器は追加しない。

168はmain使用不可の普通防具で、属性適合を満たせば同copy複数を順序付きで使える。Flashなら一枚だけ、leading filter→168→普通防具は可能、168と190/壁/跳ね返し等の特殊反応は同じ列に併用できない。通常の全response消費後、選択順に各168の自己Feverを一回ずつ付ける。同modelだからと副作用をdedupせず、完全防御でincoming被害0でも自己付与する。

その後incoming bodyを適用し、全body後にだけ208救済/死亡を確定する。prefixでHeavenfallによりHP0になっても本体前に蘇生させず、HP0&&!deadは同じtransactional step内だけに留める。応答待ちsnapshotへHP0を許すように制約を緩めない。counter基準はincoming bodyのtarget HP実損失だけで、prefix落下分を足さない。ただし自己HP吸収aliasならprefix HP0→source回復→target被害という順で正のbody損失が発生し得るため、prefix落下後のcounterが常に0とはしない。最終survivorは既存208救済後に判定する。

198はmainのplay専用でresponse品には選ばず、MP+20を先に0〜99clipしてからHeavenを付与し、全body後に救済/死亡を処理する。既にHeavenならMPは増加した後に再付与fallが起こり得る。自己は即時、他人はnonattack utilityとして空応答または190万能反射だけ、反射後の現在targetへ適用する。FREE/追加品・保持化・MP費用は無く、能力自体のRNGも0。全action後の病気tick/gift RNGは別である。

168/198をdiscardや売買商品として消費/移動しても、この新abilityを発動しない。商品として届いたことから自己FeverやMP20/Heavenを付けず、guardianを召喚しない。既存の一病気+Flash/DarkCloud、mild/all、固定5%/元ownerの全action完了tick、自然終局/cutoff優先、208/gift順はv22を維持する。

公開 `disease_item_rule` は `experimental:response-self-curse-in-selection-order-after-consumption;prefix-before-body;death-and-revival-after-whole-body;counter-basis-body-target-loss-only;mp-boost-before-disease;no-trigger-on-trade-or-discard`。clientに根拠がある普通防具の選択/属性・selfCurse選択indexの演出・198のMP20/Heaven表示と、消費後の順序/暫定HP0/208/body/counter/死亡を接合する**公式server未確認の実験規約**を区別する。失敗時は状態/RNG/出力を含む全stepをrollbackし、新たな副作用history/token/RNGを公開しない。 198は全response後のbodyでMPを増やし、nonattackへの反応は190だけなので同batchの有償MP反応予算を増やさない。v23/24時点のjournal上界は非BUY3,725/BUY3,726だった。v25のdeath追加分は後述する。新ベンチ `fever_mask_uses` は応答で使った168コピー数、`heaven_herb_plays` は198のmain開始数で、捨てる/商品移動を含めない。`heaven_herb_resolved` は自己即時または他者空応答の最終body解決数（190反射途中は除く）、`heaven_herb_mp_gained` は当該targetのMP after-before実増分で、99cap後の値を数える。同body後のtick/giftはMPを変えないため分離でき、名目20倍値に置き換えない。resolved<=plays/実増<=20*resolvedを検査する。未解決cutoffなら開始だけ、解決済みcutoffならbody実増も計上する。


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

詳細は[限定実行器v23記録](official-execution-subset.md#v23の過去の検証と仮面天国草集計)。


## v24：Fogの表示/選択と情報境界

v24は72（Water weapon/ATK3/onDamage Fog/p10w1）、97（Water weapon/ATK3/50%/onDamage Fog/p8w1）、182（Water armor/counter Fog/power0/DEF0/p10w1）、221（Water miracle/Fog/MP3/w1）を加え、228モデル/重み489（trade3/weapons104/armor76/sundries17/miracles28）とする。rulesetは `official-execution-subset-v24`、Snapshot24（旧1〜23拒否）、公開schemaは `official-execution-view-v22`。Match ABI2の幅・kinds0〜8・purchase phase3/finished2、catalog/state ABI1・legacy37を維持し、学習器は追加しない。

raw stateは病気最大一種→Fog→Flash→DarkCloudのenum順/重複無し、最大四curseへ広げる。mildはCold/Fever/Fog/Flash、allは対応する病気4種/Fog/Flash/DarkCloudの全七種類を解除する。Fogは非病気でHeaven再付与fallを起こさず、Fog単独には自然tickを追加しない。72/97の付与は既存の正target HP実損失、182はsurvivor/FIFOの純curse反撃、221は保持/FREEと適合する魔法反応に従う。

Fogの本人がmain playまたはbuyで明示的に敵を指定した場合、launchでalive敵をinput順に列挙して一様bounded drawを一度引き、候補一人でもdrawする。202を使う場合はこの対象draw→符号drawの順。自己/味方指定は変えず（自己BUYの既存unsupportedは維持）、%攻撃・Wide232・Danger94はFog drawを除外する。14は選択先に二回、redirect/bounce/counter/購入YESNOで選び直さない。祈り/discard/exchangeもこの敵指定launchに含めない。

教典の「敵に使った神器は敵内でrandom」に対し、一様性/一人時draw/launch時点/例外/順序は**公式server未確認の実験規約**である。特権 `last_fog={owner,intended,target,models}` は次main受理でclearし、公開JSONへ出さない。pendingの全journal replayは元command/状態/RNGから対象選択とpayloadを検査するが、完了raw sceneの全歴史の到達証明ではない。

観測は既存project_viewで、生存Fog viewerかつ未終局のときだけ生存敵statusをnullにする。自己/味方/dead敵、観戦/dead viewer/終局後には覆いを掛けない。通常status欄と別の使用済み奇跡paneに表示される名前・team・usedモデルは公開のまま。未使用の他者手札枚数/中身は出さない。

`pending.forced_hit` は現在targetではなく、命中判定時の `initial_target` のboard paneがFogで隠れるviewerに対し、内部true/falseの両方をnullにする。元のhidden敵のDarkCloud判定が190反射で可視selfへ戻ってもnullを維持し、可視の元targetならboolを返す。内部の必中判定/乱数消費は変えない。pending payloadやpurchase/removal/sale等の公開eventは別表示経路として保持するが、全viewerへのserver fanoutは未照合である。

v24/25ではFog本人のforeign SELLは敵/味方を問わず、配送容量を読む前にunsupported（-5）として拒否し、自己SELLは許す。samplerはFogの有無にかかわらずSELLを自己targetに固定し、hiddenな他者未使用手札数を配送先候補から読まない。非Fogで手動foreign SELLをしたときの旧overflow-capacity依存gapは残す。全query/全profileの情報非干渉を達成したとは主張せず、この限定拒否と方策変更を明示する。

ベンチの `fog_main_commands` は受理main前のownerがFogだった全kind、`fog_launches` はそのmainでlast_fogを生成した数、`fog_target_changed` はintended!=selectedの数。response中のrecord残存を重複計上せず、changed<=launches<=fog_main_commands<=main_commandsを検査する。2人戦の候補一人で対象不変でも、launch drawは一回消費する。view-v22のforced_hitはbool/nullの三状態として読み、nullをfalseと同一視しない。

v24/25時点の公開 `fog_rule` は `experimental:launch-once-for-explicit-enemy-target;uniform-alive-enemies-in-input-order;one-draw-including-singleton;self-and-allies-unchanged;chance-wide-danger-excluded;redirects-no-reselection;fog-foreign-sell-unsupported;sampler-sell-self-only`。canonical集合を広げた現行 `disease_rule` は `experimental:one-disease-plus-fog-flash-and-darkcloud;reapply-max-incoming-or-next;heaven-reapplication-hp0;owner-only-after-whole-action;natural-and-cutoff-before-tick;fixed5-percent-before-hp;post-upgrade-tick;post-tick-revival-before-gifts`。病気のrank/5%/owner境界やFlash/DarkCloudの既存規約を変えず、Fogの導入を旧版のnot-forced条件へ置き換えない。


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

詳細は[限定実行器v24記録](official-execution-subset.md#v24の過去の検証とfog集計)。


## v25：死亡時攻撃と全main kind replay

v25は112昇天弓（Light weapon/通常ATK1・25%/死亡時ATK30・75%/価格10/giftRate1）を加え、229モデル/重み490（trade3/weapons105/armor76/sundries17/miracles28）とする。rulesetは `official-execution-subset-v25`、Snapshot25（旧1〜24拒否）、公開schemaは `official-execution-view-v23`。Match ABI2の幅・kinds0〜8・purchase phase3/finished2、catalog/state ABI1・legacy37を維持し、学習器は追加しない。

主動作の112は通常の25%攻撃としてtarget無しのalive敵rosterを使い、消費/支払/命中/RNG/応答は既存規約に従う。死亡時は別の自動payloadで、Light30/75%・モデル112・death来歴を持つ。通常の1/25%から動的値を識別し、旧117の通常75%を現行112へ流用しない。

208救済を先に処理し、実際にdeadになった人物だけを対象とする。全bodyのactual death flags確定後に自然終局なら112を消費/予約しない。非終局なら新死者の在庫112を全copy在庫順に直接消し、copyごとにその境界のalive敵input順へframeを一括予約する。死亡者へgift予約は増やさず、通常consume helperとは分ける。同じcopyが複数enemy payloadの来歴になっても、物理消費は一回だけである。

decision cutoffはbody/死後の112消費・予約より後に判定する。従ってこの場合のcopyは消費済みで、残workを取消し、まだ開始していない命中roll/responseは行わない。自然終局優先では生存者一人・全員同じ非Solo team等の既存条件も使い、duelの最後の敵死亡から道連れを発生させない。このgate・全copy・消費順・cutoffの差は**公式server未確認の実験規約**である。

新しいdeath FIFOを普通counter FIFO・初期rosterより先にdrainし、途中で新たに死亡した人物のframeは末尾へ追加する。死亡者はsource/originのまま自動攻撃できるが、dead actorの手動commandを許す意味ではない。ownerは元mainのまま。対象deadはskip、alive対象への活性化時に75%を抽選し、DarkCloudならdraw無し必中。Fogの追加再選択はせず、反射/跳ね返しで再命中rollしない。返された先がdeadならframeを終了、bounceは既存alive候補へ向ける。普通防御・属性filter・適合反応は現在payloadに従う。

全main kindのprestate/原command/受理response journalから同executorを再生し、死者・消費copy・death FIFO・RNG・普通counter・現在frameを照合する。病気tickによる死亡も同じ仕組みへ接続し、特権 `tick_processed` stageで元ownerのtickを一回に限る。tick後に死亡時応答を待っても、完了時に同ownerへ再tickしない。giftと次ownerは全work後まで遅延し、自然終局/cutoffで残work/giftを取消す。安定raw sceneの全過去履歴まで証明する契約ではない。

death frame数の保守的上界は `player_capacity*(player_capacity−1)*hand_limit`（最大9×8×18=1,296）。copy数とframe数を混同せず、journal上界へこのdeath_boundを加える。旧v23/24の最大3,725/BUY3,726だけを死亡queueへ流用しない。死亡FIFO・自動消費token・未来RNG・stage・内部累計は公開せず、現在の `pending.death` booleanとdeath_ruleを公開する。死亡時payloadでもforced_hitのinitial_targetに対するFog null化は維持する。

ベンチの `death_bows` は自動消費した112コピー数で、通常の `physical_used` へ混ぜない。`death_created` は全予約frame数、`death_started` は75%命中または暗雲forcedでalive actorのresponseを開始したframe数、`death_rolls/hits` は実抽選/成功、`death_forced_hits` は無draw命中、`death_responses` は反射等を含む実defend回数。bows<=created<=8*bows、hits<=rolls<=created、started=hits+forced、forced<=created−rolls、responses>=startedを検査する。新しいtickだけを集計し、死後応答中に残るlast_tickを重複計上しない。自己205/206の後に同tickで消えた208/自動112を除去eventのtoken消失数へ混ぜない。公開JSONへ累計/未来FIFO/来歴stageを追加しない。通常配布で未対応のgiftモデルは残り8種で、Dreamは調査のみとする。

公開 `death_rule` は `experimental:revival-before-actual-death;nonterminal-boundaries-only;consume-all-bows-in-inventory-order;copy-then-alive-enemies-in-input-order;light30-75-percent;death-fifo-before-counters-and-initial-roster;dead-source-allowed;darkcloud-forces-without-draw;redirects-no-reroll;dead-redirect-target-ends-frame;no-fog-reselection-or-gifts;owner-tick-once;natural-and-cutoff-cancel-pending`。教典の死亡時75%攻30とclientの受信演出を、ここで選んだqueue/終局/再帰/救済/giftのserver規約と取り違えない。


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

詳細は[限定実行器v25記録](official-execution-subset.md#v25の過去の検証と死亡時攻撃集計)。


## v26：取引配送と容量を使うqueryの境界

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

詳細は[限定実行器v26記録](official-execution-subset.md#v26の検証と取引overflow集計)。

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

36/80の付与は既存の通常targetdamage段階の実HP損失、183は既存survivor counter/FIFO、222は非attackの直接付与として扱い、trigger・死亡・反撃・補充順は既存の実験規約を引き継ぐ。Dreamは病気とは別に重複を持たず、Heavenへの非病気付与で転落させない。自然tickは追加しない。mild解除はDreamを残し、all解除はDreamと現在の全fakeを解除する。

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

[LOCAL reference生成器](../scripts/official_driver_oracle.cjs) / [保存report](../data/official-v2/driver-local-v49.json) は独立BigIntでenvelope23例、別区分のFate chance stage6例、dispatch契約4行を記録する。reportは55,335bytes、SHA256 `6f8ba9de99f5c6ee3f6440b27faa1c64f4fb80a6729a038a42126e42c381b438`。新しい一次クライアント選択器の発見・実行は0であり、既存v44等の固定証拠を継承する。N0やresponse/purchaseのreferenceは旧samplerへ委譲する直前で止まり、旧sampler自体のdrawまで0と呼ばない。

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

[実client oracle](../scripts/official_active_display_oracle.cjs)、[保存report](../data/official-v2/active-display-client-v50.json)、[native TSV](../data/official-v2/active-display-vectors-v50.tsv) は実ks.kの3受信branchとaa/fu/fq/aT・roster lookupを実行し、47受信シーケンス・53event callを記録する。full handler error8／core error7を分け、native ERROR10にはtyped adapterのstrict未描画field拒否・safeGF上限も含める。native用47vectorはJSON/core結果または指定されたERRORと全て一致した。

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

## v52：公開遷移3件と対局v48の照合

[機械可読witness](../data/official-v2/live-transition-witnesses-v52.json) と [UI観測記録](official-live-observations.md) に、公式CPU修行の公開checkpoint3件を保存した。光3空防御、32の無属性9自己攻撃、吸収26にDEF7という入力から、C++が各資源・次owner/GFを事前計算する。実際のdamageをエンジンへ渡すfixtureではない。3件目の公開結果19から中間heal/damage順まで確定したとは扱わない。

2件目では、本人に未選択のDEF7がある状態でも、32を本人へ使うplayだけでHP37→28・次のCPU手番へ進んだ。対局エンジンはこの普通武器の明示自己targetに不要なresponseを要求していたため、限定したDEF0・1command解決へ変更した。`official-execution-subset-v48` / Snapshot48 / view-v46とし、Match ABI2の幅を維持して `self_attack_rule` getter・属性・observe fieldを公開する。32以外の普通属性・追加材への適用には限定一般化が含まれ、特殊self/redirect/counter/guardian/payloadの経路は今回変更しない。

[新native test](../tests/official_live_trace_test.cpp) は3件×CPU在庫長3通り×未選択品3profile×任意seed8通り＝216投影を検査する。これは結果に影響しない在庫差替えの検査であり、実在手札を再構成した216公式対局ではない。特にself32後のCPU1枚のprobeは、その後に観測した3枚攻撃までの全観測を再現するものではない。比較はその次行動前の資源/GF/ownerに限る。

既存native回帰は217関数を一意登録し、新しいself経路のcutoff・208救済・遅いgift token枯渇rollback・grant中断のsnapshot/restore再開・Wide境界・特殊baseの旧responseも検査する。C/Pythonには通常selfの追加defend拒否とgetter整合を追加した。これらのlocalな死亡/補充/tick順の回帰は、今回の非致死実観測とは別の検証である。

最終ソースのRelease CTestは **155/155件、32.36秒** で通過した。 ASan/UBSanの全CTestも **114/114件、365.20秒** で通過した。Python Matchは148ケースを通過し、C11公開ヘッダのConfig24/Command184/Choice208/Status248と新getter宣言も構文検査を通過した。Release/SANのビルドはwarning/error0。 その後、通常自己攻撃による死亡から112の応答FIFOへ入るsnapshot/restore回帰を追加し、当該native全217関数をRelease/SANで再実行して各1/1件（0.57/12.86秒）が通過した。本体コードの追加変更はない。

次の行列はseed42、2人1,000局/scene・9人100局/sceneについて、両ビルドの各1/4workers計16recordを比較したもの。4群それぞれで時間・速度・worker数以外の全fieldが一致した。独立familyは最大2系統並行、速度の比較・改善主張には使わない。

| 実行系 | 人数・件数 | Release/SAN・1/4workers共通checksum |
| --- | --- | --- |
| 通常対局 | 2人1,000局 | `17012641369213081577` |
| 通常対局 | 9人100局 | `14587398145354569665` |
| choice/chance driver | 2人1,000scene | `1562149086175909460` |
| choice/chance driver | 9人100scene | `5615713826154359954` |

自己攻撃のdecision数と処理時点が変わるため、以前のrulesetのgameplay checksumが一致することは要件にしない。旧pins・旧oracle・過去のベンチ表を上書きせず、今回の値を別世代として記録する。一般のserver schedule/selector、178終末一新・94＋232・互換外Dream・一般Earth/Moonは引き続き主要残件である。

## v53：Dark自己終局と反射剣の追加応答除去

[新しい観測記録](../data/official-v2/live-transition-witnesses-v53.json)には、終末なしG.F.6の81自己使用から本人HP9→0・CPU勝利・G.F.6維持・手札9→8を確認した完了遷移1件と、終末1で38自己使用後に追加の本人defend無しでG.F.4へ進んだ経路観測1件を分けて保存した。公開資源・終局を照合する完了遷移は累計4件、経路のみは別1件、full wire traceは0件である。

反射剣38の後には授与とCPUの別手番が入っており、そのイベントを取得できていない。本人の最終HP30やCPU HP55を38単独の効果goldenにしない。反例として使うのは、本人defendを必須にしていた旧経路である。主動作で使用した38の反射能力を自動発動する処理も加えない。

修正は明示self・先頭weapons・必中・非Wideというv48条件を保ち、先頭abilityをnoneまたはreflect_weaponへ広げた。対局ruleset49 / Snapshot49 / view-v47、ABI2固定。C++定数・C getter・Python属性のself_attack_ruleは同じ範囲を示す。38以外の同abilityや追加材への限定一般化、他の特殊自己攻撃・redirect・counter・外部payloadの従来経路という境界を[実行器契約](official-execution-subset.md)に残す。

nativeは4完了例×72通り＝288公開投影比較に、38の独立した72合成経路検査を追加する。38はexternal_grantsを使うローカルなbody後停止点を検査し、そのHP30・未実行授与・RNGを公式中間stateとは呼ばない。C API/Pythonにも38単品自己使用の消費1回・decision1・防御要求無し・追加defendの原子的拒否を追加した。native既存217関数、Python149メソッドはそれぞれ一意登録を確認した。

focused Releaseは5/5件・7.64秒、ASan/UBSanは4/4件・15.24秒で通過した。全Release CTestも155/155件・38.79秒、全ASan/UBSan CTestも114/114件・362.05秒が通過。両ビルドはwarning/error0。旧pins・旧witness・旧版の検証表を上書きしていない。

seed42の通常対局とchoice/chance driverについて、2人1,000局/scene・9人100局/sceneの両ビルド×1/4workers計16recordを照合し、4群全てで時間・速度・worker数以外の全fieldが一致した。実験規約が変わったため、v48以前のgameplay checksum一致を要件にしない。独立familyは最大2系統並行し、速度改善を主張する計測ではない。

| 実行系 | 人数・件数 | Release/SAN・1/4workers共通checksum |
| --- | --- | --- |
| 通常対局 | 2人1,000局 | `3377087730564053909` |
| 通常対局 | 9人100局 | `5284858347239428124` |
| choice/chance driver | 2人1,000scene | `1577905839749268940` |
| choice/chance driver | 9人100scene | `15970925910578989730` |


## v54：大木の自己使用と保持奇跡の再使用

[v54観測記録](../data/official-v2/live-transition-witnesses-v54.json)に、213大木の自己使用、19鎖ガマへの空防御、使用済み213の後の自己使用という完了遷移3件を追加した。累計7完了例・独立routing-only 1件・full wire trace0件。G.F.3の別の再使用はCPUの次行動を取り逃したため、不完全区間として分離する。CP/MP変化を逆算して未観測のCPU両替を決めたり、新しいDEF2の由来を決めたりしない。

同じ213のused表示を保持しながらMP3を払い、追加の本人defend無しで木6を受け、補充で表示枚数が1つ増えることを初回・後の再使用で確認した。本体は能力なし・hitRate0・非Wideの通常main自己攻撃奇跡を、既存の支払い・消費・保持処理とDEF0 bodyへ接続する。大木以外の同条件7種へは限定一般化であり、8種すべてのlive観測とは呼ばない。補充は従来から各使用で予約しており、今回giftの分布・候補・予約数を変更していない。

対局ruleset50 / Snapshot50 / view-v48、Match ABI2固定。native218群では8種×未使用/使用済み×FREE有無、Dark終局、cutoff、外部grantからの再開、late allocationの全snapshot rollback、敵対象のresponse維持を検査する。213base+Wide232は実clientの追加条件に反するため原子的拒否を検査し、合法な武器+Wideの既存回帰と区別する。C API/Pythonでも213自己使用1decision・MP支払い・同token保持・余分なdefendの原子的拒否を確認する。Pythonは150メソッド、nativeは218関数を各1回登録した。

新3完了例を含む7×72＝504公開投影比較と、反射剣の別枠72経路検査を維持する。合成の未選択在庫・seed・tokenの変化は観測投影の非依存性を調べるもので、実手札の再構成やserver RNGの再現ではない。初回大木の比較点は次の鎖ガマが未解決の画面であり、後の再使用はCPUの次launch前を連続捕捉した。

main/driverの2人1,000件・9人100件をRelease/ASan、1worker/4workersで計16record比較し、時間・worker以外の全fieldが一致した。checksumはmain2人 `2119345984215049797`、main9人 `8347905528679061107`、driver2人 `411534287975272152`、driver9人 `17637447469635151454`。これはローカル実行の再現性で、公式対局の乱数同等性や版間速度向上を主張しない。

全Release CTestは155/155件・170.16秒で通過した。Python150ケースもこの全suiteに含む。Release/ASanビルドのwarning/errorは0件。実行時間は環境と並行実行の影響を含み、速度改善値として使わない。旧pins・旧witness・保存oracle・legacy37を上書きしていない。

全ASan/UBSan CTestも114/114件・1,210.23秒で通過した。今回は全suiteを各ビルド内で逐次実行しており、過去の並行suite実行時間との速度比較には使わない。


## v55：特殊main自己攻撃のclient経路と限定一般化

[実client生成器](../scripts/official_self_attack_oracle.cjs)と[保存report](../data/official-v2/self-attack-client-v55.json)は、main選択・自己target・捕捉した送信と、別入力の単一受信frameのpane/activationを分離して照合する。入力した受信frameを、捕捉した送信に対してserverが生成したtraceとは呼ばない。新しい完了実対局観測は0、累計7完了例・独立routing-only1例・full wire trace0件を維持する。ブラウザ操作接続は再初期化後もhost/native pipeエラーで、権限要求や公式のコマンド拒否を示す結果ではなかった。

対局ruleset51 / Snapshot51 / view-v49は、通常mainの明示自己target・hitRate0・非Wideについて、27/45/216の吸収、14の二回、58のMP威力、53の同被害を既存実験bodyで直接解決する。単発selfの効果順は従来のresolveを変えず、14だけ同じ起動payloadからfresh bodyを2回実行する。支払・消費・accepted decisionは各1回、各body後の自然終局または本人actual deathで残りを取り消す。208・112・死亡FIFO・補充・cutoffは既存の機構へ接続し、特殊selfの未観測の内部順を公式事実へ格上げしない。全死亡・補充scheduleの確定は残件である。

旧main自己防御で作っていた反撃の算術検査は、敵への起動から反射／bounceを通る実際のresponse境界へ移す。明示自己mainへ未選択防具の自動効果を加えず、redirect/counter/guardian等の別入口を一括して変更しない。旧snapshot1〜50を拒否し、Match ABI2・catalog/state ABI1・旧pins・旧witness・legacy37は維持する。


nativeは新3群を含む221関数、Pythonは151メソッドを各1回登録した。新native群は6基底の自己body・吸収cap/保持/FREE・53全body後の208・58起動MP0/1/99・14の二回/途中蘇生/死亡112/FIFO・cutoff・外部grantからの再開・late token/update失敗・proof改竄を検査する。完了済みmainのrestoreは過去bodyの履歴証明ではなく、grant停止点ではanchorからの全再実行一致を検査する。

main/driverの2人1,000件・9人100件をRelease/ASan・1worker/4workersで計16record比較し、時間・worker以外の全fieldが一致した。checksumはmain2人 `16866981254746083985`、main9人 `2959968806296183525`、driver2人 `11467765941077267296`、driver9人 `16171709013863546710`。これはローカルの再現性検査で、公式乱数同等性や版間速度比較ではない。client生成器の `--check` も再取得JSでbyte一致を確認した。

全Release CTestは155/155件・113.35秒で通過した。Python151ケースを含む。変更した公式エンジン関連のASan/UBSan検査は `^official_` を対象とし、前版で通過した未変更のlegacy環境全体は再実行しない。両ビルドはwarning/error0件。suite時間は並行実行環境の値であり、速度比較には使わない。

ASan/UBSanの公式関連37/37件も229.92秒で通過した。native221群・C API・driver・公開遷移の既存504比較と72routing検査、公式関連サンプラー／表示／台帳の検査を含む。特殊selfの新しい実対局遷移数は0件のままである。


## v56：実対局の46自己使用と自己売却の比較

[v56記録](../data/official-v2/live-transition-witnesses-v56.json)は、自己SELL `[4,26]`、Stone69への空防御、cutCost46の自己使用という新しい完了遷移3件を加える。累計は10完了例・独立routing-only1例・full wire trace0件。G.F.1→2とG.F.3→4は連続した画面取得でCPU次launch前を捕捉し、途中の相手行動から資源を逆算した比較ではない。46は追加の本人defend無しでHP38→26・MP10/CP20維持、同カード消費と表示枚数9維持を確認した。

本体はruleset52 / Snapshot52 / view-v50へ進め、weapons baseのcut_costを既存の通常main明示自己target・hitRate0・非Wide経路へ加える。固定台帳で新しく該当するのは46だけで、他カテゴリのcutCostは対象外。完成列のMP/威力/属性・保持・消費・gift算術や一般queue順は変えない。単品46実測からMC/FREE合成への適用は限定一般化であり、それら全列の新しい実対局照合と呼ばない。旧snapshot1〜51を拒否し、Match ABI2・構造体幅・opcodeは維持する。

native公開比較は10件×72変種＝720と別枠72routing。新gift46を同定できても、任意seedにそのモデルを出させる比較は行わない。自己売却の元operation/product消費・spare保持・商品配送、46の選択token消費はローカルな物理コピー不変条件として確認し、合成giftの4/26/46寄与を既知copy投影へ含めない。server tokenが見えたとの主張でもない。価格3の自己授受の資源netを比較し、operation表示価格5を追加の費用へ読み替えない。


nativeは新1群を含む222関数、Pythonは151メソッドを各1回登録した。46の単品自己使用と未選択DEF保持、Auraの通常費用／隣接FREE、MP不足、cutoff、外部grantのpause/resume、late token失敗の全snapshot rollback、敵対象／bounce後の応答維持を検査する。

全Release CTestは155/155件・41.99秒、ASan/UBSanの重点4件（official_match、official_live_trace、official_match_c_api、official_driver）は4/4件・34.89秒で通過した。両ビルドはwarning/error0。未変更の旧環境を含む全ASan suiteを今回再実行したとは呼ばない。過去の全suite通過記録はその時点の履歴として残す。

main/driverの2人1,000件・9人100件をRelease/ASan・1worker/4workersで計16record比較し、時間・worker以外の全fieldが一致した。checksumはmain2人 `8683080092853518022`、main9人 `429222971013924689`、driver2人 `17221520816642828748`、driver9人 `16247101765229435314`。これはローカル再現性の比較で、公式の乱数・配布モデルの一致や版間速度向上を主張しない。

## v57：観測17件とnative比較15件の区別

[v57公開観測記録](../data/official-v2/live-transition-witnesses-v57.json)は、SELL4＋46、17への空防御、222のDream付与、206のused奇跡除去、46＋オーラ・購入の複合grant、祈りからの241と通常品を加える。記録日は2026-11-05で、個別captureは2026-11-04〜05の範囲に限り特定できる。対局ruleset52 / Snapshot52 / view-v50、既定236/499・任意237/500、Match ABI2を維持し、本体の進行・grant順序を変更しない。

完了公開観測は従来10件から17件、full wire traceは0件、独立routing-only観測は1件である。ただし46＋オーラ・購入の2件を単一効果のgoldenへ数えない。nativeへ追加するのは先4件＋祈り例の5件で、累計15件×72変種＝1,080公開投影比較と独立72routing検査になる。合成在庫やseedの変種数は公式対局数と分ける。以前の累計10件・720比較はv56時点の履歴として保持する。

売却・17・222・206の4例では操作列から価格20、ATK4、MP6、used222除去を計算し、対応する公開資源・手札・Dream持続と比較する。206で除いたused222は発動済みのDreamを解除しない。未観測の補充modelやCPU在庫は補完せず、テストを完了するnormal fallbackがあってもlocal completionに限定する。全server event列やselectorへの適合とは数えない。

祈り例ではDream中の本人が表示上武器なしで祈り、HP50→241の20被害→HP30→通常品受領、手札8→9、G.F.8→9を連続確認した。nativeでは元の予約1slotを悪魔後も保持し、通常品で1回埋める既存規約を比較する。観測していない通常品の正体をseedから合わせず、祈り・悪魔・通常授与の名目と実資源を分ける。この例を243の盗難補償や、reset/redrawにも同じ経路がある証拠へ広げない。

46＋オーラ例のendpointは46＋231のpower24−DEF12、MP6支払と後続244/243列に整合するが、未観測CPU普通giftをnormalと仮定した完全再生へ置き換えない。購入例のBUY5とchance列も非公開在庫を伴う。両例のrecipient間順序は、画面のCPU先・本人後という表示順とbackend player array順を同一視せずに保持する。先にCPU244と仮のnormal fallbackを処理して同じ終点を作れても、観測されたevent順に一致したgoldenにはならない。最終手札数から243追加補償の有無を一意に確定することも避ける。

今回の顔231は異なる真モデルへのrevealが確認されず、保存画像のicon比較だけでDream互換外と断定しない。178一新途中grant、94＋Wide、Dream全分布・不適合入力、一般Earth/Moon、公式の死亡/手番/補充scheduleという主要gapを維持する。旧pins、旧観測・oracle、過去の検証値と訂正、legacy37を変更せず、学習器を追加しない。

変更した `godfield_official_live_trace_tests` をReleaseとASan/UBSanでビルドし、CTest `^official_live_trace$` は各1/1件が通過した（Release0.31秒、SAN0.76秒total）。両方で15例1080投影＋72routingを確認し、両ビルドのwarning/errorは0件。今回はテストと記録のみを変更し、本体・C/Python API・legacyを含む全suiteやベンチマークを再実行したとは呼ばない。


## v58：一新コンポーネントと全体観測の比較範囲

[v58公開観測記録](../data/official-v2/live-transition-witnesses-v58.json)は、G.F.11の本人201→CPU空応答→CPUPluto271→本人178→後続授与→G.F.12という完了観測1件を加え、累計18件とする。独立routing-only1件・full wire trace0件は維持する。ruleset52 / Snapshot52 / view-v50、Match ABI2、配布poolと進行規約は変更しない。

本人HP38/MP0/CP20、CPU HP5/MP4/CP20から、178単品DEF14でDark2を防ぐ無事→Dream→神器一新と旧used奇跡2個の消失を連続取得した。frame58は7ordinaryが明瞭だが8thは重なりで不明、frame63/64には9ordinary＋transient244がある。置換中popupなしは8ordinaryの証明ではない。後続に244CP+10、244MP+10が現れ、G.F.12 CPUmainの本人はHP38/MP10/CP30・Dream・fresh-unused手札10、CPUはHP5/MP4/CP20。244二回のslot対応や、見かけ各品の実モデルは確定しない。

新しい72一新コンポーネント検証は、外部271への178応答について、Dream・DEF14によるDark2の被害0・旧used奇跡2個の消失という限定観測を比較する。nativeの8fresh ordinaryは現仮説に基づく合成中間境界であり、UIの8ordinary確認や完全な直接境界比較ではない。原発201の予約1も持たないfixtureなので、公式endpointの手札10・G.F.12には比較しない。full比較の15件×72＝1,080と別枠72routingは従来どおりであり、18件すべてをnative full goldenへ変換したとは呼ばない。

[一新の実client oracle](../data/official-v2/redraw-client-v29.json)と[grant oracle](../data/official-v2/grant-client-v41.json)の受信表示処理と整合するが、backend選出・全イベントfanout・debtの証明とは分ける。8ordinary置換＋後続reserved244と、7ordinary＋244の置換＋reserved normalはともに排除できない。sortとawaitしない移動・重なり順の変化により、途中画面の位置からslotの由来も確定できない。現行即時redraw規約の反例も、grant接続が必須である証拠も得ていない。次の観測対象は、一新内の悪魔可否、2slotの対応、複数帽子、208/112と死亡中断、backend recipient順、Dreamの真値・分布である。旧観測・検証値・v10/v16訂正・pins・legacy37は保持する。

変更したlive targetをRelease→ASan/UBSanでビルドし、CTest `^official_live_trace$` は各1/1件が通過した（0.49秒／0.87秒）。両出力で1080投影＋72routing＋72一新コンポーネントを確認し、ビルドwarning/errorは0件。通常0の解決前で停止する部分検証で、最終手札9を実行したとは記録しない。本体変更が無いため、全suiteとベンチマークは再実行していない。

別GETで既存v2 pinと同じ本文を得たJSを使い、既存のredraw oracle（在庫223＋短入力1＋used pane19）とgrant oracle（42項目）の `--check` も保存reportへbyte一致した。旧reportやpinsを更新せず、receiver関数の一致をserverの生成規則の証明に広げない。

## v59：受信game文書と通常chance入力の接続

CPU修行の `room.game` を含む4連続更新を匿名化して保存し、3完了操作をnative replayへ追加した。累計21完了観測のうち、native比較の対象は従来15例＋新3例。従来15×72＝1080投影、72routing、72一新部分検証は維持し、新3操作は8seed・4 checkpoint＝32比較の専用wire targetで検証する。記録だけの複合3例はfull native比較へ加算しない。初期状態1＋受信更新batch3を取得したが、全対局・全backendイベント列の取得は0のままである。

新ruleset53 / Snapshot53 / view-v51は設定プール内・非Dreamの通常授与を明示chance入力する。通常0と悪魔入力、既定samplerとRNG、ABI2を保ち、記録結果にRNGをfitしない。受信slot再利用とfreshローカルtokenを対応させ、CPU184は手札差分の記録結果として扱う。raw model0 placeholderのread-only importとpending scene再開は未対応を維持する。詳細は[実行契約v53](official-execution-subset.md#対局v53作業世代v59記録された通常授与の明示入力)。

次の観測は178一新に含まれる授与と後続授与の区別、94＋Wide、Dreamの真モデル・mask、死亡中断と全体scheduleを優先する。今回の受信CPU在庫を公開方策へ渡さず、受信eventsを全内部処理とみなさない。

検証はRelease全156 CTestターゲットを、先行155/155（42.09秒）とnative追加後1/1（1.02秒）に分けて実行し全通過した。Python Matchは155 method、native Matchは224登録callbackである。ASan/UBSanでは最終の `official_live_trace` / `official_wire_trace` / `official_driver` / `official_match` / `official_match_c_api` が5/5通過（27.04秒）。driver benchmark・grant benchmark・grant sampler benchmarkも各通過した。全legacyを含むSAN suiteは再実行していない。最終Release/SAN buildのwarning/errorは0件で、旧pins・旧witness・旧reportは変更していない。

explicitordinaryのconfigured pool/207 gate、Dream拒否とinactive payloadの原子性、fresh unused token・非詠唱・無RNG、normal0と既定経路のstate/RNG比較、private journalのrestoreと改ざん拒否、token/update/GF枯渇rollback、cutoffでのbody先行を追加検証した。受信fixtureはprivate原本から指定した匿名化だけで全値一致し、全4本文の同一room文書参照も識別子を出力せず確認した。

## v60：完結した受信列と明示empty adapter

新しい1完了観測で累計22例・native比較対象19例となる。開始〜終局を含む連続受信gameの2状態を1対局分取得し、受信列の完結と、全内部serverイベントtrace0を区別する。CPU通常2結果の順は未観測なので2順×8seedで再生し、2checkpoint計32比較を追加する。v59の8再生32checkpointと既存15例1080投影・72routing・72一新部分検証は別枠を維持する。

補充中の死亡はGF1のまま終了し、dead本人のempty1が残った。新 `ReceivedState` はcanonical emptyを元位置付きmetadataへ分離して物理stateを既存C++で投影する。rawと物理snapshotはprivilegedで、公開viewへ他者手札/emptyを加えない。旧strict constructorやABI、Matchの再開制約を緩めず、ruleset53/Snapshot53/view-v51を維持する。

Dream全分布・一新中の授与・94＋Wideと一般schedulerは未確定のまま。今回のCPU243は追加補償なしのcompletionと整合するが、全局面の補償法則や未観測のCPU内部retryまで確定したものではない。

検証はReleaseの `official_wire_trace` / `official_state` / `official_state_c_api` / `official_state_python` / `official_match_python` が5/5通過（6.79秒）。State Pythonは既存12＋ReceivedStateの6群＝18 method、Match Pythonは155 methodである。新wireのRelease/SAN個別実行も各1/1通過（0.27／0.40秒）、最終build warning/errorは0件。今回は本体を変更せず、全156ターゲットや全legacy SAN suiteを再実行したものではない。

PythonからSAN版libraryを使う追加試行は、指定した `DYLD_INSERT_LIBRARIES` がPython processへ保持されず、ASan interceptor初期化でabortしたため、成功したSAN Python検査へ数えない。Python adapterの型検査・所有権・close・pending import拒否はReleaseで確認した。rawと匿名化gameの全値、artifact SHA、同一room参照と元識別情報の除去も独立検査した。

## v61：観測結果を指定するDream授与の入口

`ArtifactGrantAction` と独立したC16 byte/Python APIを追加し、既存9key互換のactual/fakeを補充のchance入力にできるようにした。fake無しは明示unmaskedとしてDream中も受理する。通常0の抽選、旧GrantAction、sample_grant・driver、配布poolは維持し、ゲームRNGやローカルDream抽選統計へ入力結果をfitしない。型付きjournalへ保存して全snapshot再生と不整合拒否・late rollbackを検証する。ruleset54 / Snapshot54 / view-v52、Match ABI2・旧サイズ不変である。

今回の公式探索では新しいDreamの実model/fake pairを確定していない。完了観測22・native対象19・独立routing1、受信lifecycle1・全backendtrace0を維持する。新入口の合成fixtureを公式実測ケースへ加算しない。非互換Dream・50%全分布・一新中の授与・94＋Wide等は残件である。

最終Release build後の全156 CTestが通過した（21.86秒）。Python Matchは158 unique method、native Matchは226 unique callback。重点Release6対象も6/6通過（8.70秒）。ASan/UBSanでは本体・C API・wire・driverの4/4（14.79秒）と、live・driver benchmark・grant benchmark・grant sampler benchmarkの4/4（17.49秒）が通過した。最終Release/SAN buildはwarning/error0。全legacyを含むSAN全suiteやPython SANを実行したという意味ではない。

互換masked/明示unmasked、Dream gate、configured207、型・reserved・NULL・出力canary、本人faceと他者への非漏洩、使用時actual/reveal、混合typed journalの全snapshot再生と不整合拒否、token/update/GF枯渇rollback、cutoffを確認した。既存samplerは独立64seed参照との一致を維持し、旧grant/driver/live/wireの比較も通過した。旧pins・観測記録・oracle・legacy37は変更していない。

## v62：非終末の通常補充と天国病の受信順

`external_artifacts=False` を追加し、任意設定で全GFの予約通常補充を外部結果へ分離する。悪魔許可は従来のexternal_grantsかつactive apocalypseだけとし、normal-only helperはchance entropy0で通常0を返す。driver・元anchor再生・GF閾値跨ぎ・cutoffを同じ条件へ揃える。Config24 byteの21を使用し、ruleset55/Snapshot55/view-v53、ABI2と旧行動幅を保つ。

[新しい受信2状態](../data/official-v2/wire-game-updates-v62.json)は、終末なしの198自己使用から、MP＋20→Heaven→disease→HP＋5→本人191補充→次CPUの191自己回復→G.F.3までを示す。CPU新207は手札差分からの結果で、giftイベントや選出率を取得したものではない。通常授与の結果を乱数へfitせず入力できるようにする今回の入口と接続する。病気悪化の確率は従来のローカル規約を維持し、全server schedulerや率の証明へ広げない。

新1複合観測で累計23完了例・native対象20例となる。v62は従来と同じ固定8seedで2checkpoint計16比較を加え、全てのseedを実行する。初回病気rollは35/65/10/87/13/35/69/13で既存の非悪化枝を通り、seed検索・除外や病気結果の上書きはしない。この一致を公式悪化率の測定へ広げない。通常191とCPU207は明示結果で与え、全物理model/fake/usedと資源・Heaven・GF/owner、各native境界のsnapshot/restoreを比較する。

最終Release検証は、wire以外155/155（22.95秒）と新wire1/1（0.26秒）を合わせ全156ターゲットが通過した。Python Matchは161 unique method、native Matchは229 unique callback。SANは本体1/1（12.96秒）、C API/driver/liveとdriver・grant・grant sampler benchmarkの6/6（19.41秒）、新wire1/1（0.40秒）の8対象が通過した。最終Release/SAN buildのwarning/errorは0件。全legacy SAN suiteやPython SANを再実行したものではない。

20個のflag×閾値scene、normal-onlyのdriver/2stream無消費と旧通常0のgame RNG一致、defaultfalse、非終末masked proof・late token/update/GF rollback・cutoff、reset/178即時redraw/取引配送の例外、208復活で元攻撃と復活の別2予約が一度だけ満たされることを検証した。テストfixtureでは通常199自己使用を補充契機とし、無補充のdiscardを使わない。旧非終末flagだけの捏造grant累計をrestoreで拒否する従来の厳格性も維持する。


## v63：Dream自己使用と受信identityの検証

80の自己使用と同一番号135/135補充を新規1完了観測として保存し、累計24完了例・native対象21例とする。最初のgame応答は取得できず、権威的資料はpost-only updateCount2の1状態である。残在庫とuseイベントから80/id1・CPU60/id2を戻した再構成sceneを使い、従来の固定8seedで1checkpointずつ照合する。未取得の初期draw・配列順・物理token・CPU内部gift列を補完せず、server updateCountとnative decisionsは比較しない。

本体はDream on-damage武器の明示self直接解決、受信同番号maskの受理とpresence保持を扱う。旧異番号9key selector・RNG消費は変更せず、C/PythonでNoneとの差、非漏洩、実体使用・reveal、restore/rollbackを検証する。36は同traitの一般化、identity防具の使用やquoteはローカル合成回帰として、80の公式完了例から分ける。旧oracle・pins・観測資料・legacy37は保持する。


最終Release CTestは156/156（16.39秒）通過。native Matchは231 unique callbacks、Python Matchは163 unique methodsである。ASan/UBSanは本体1/1（12.66秒）、C API・driver・live・wireの4/4（2.47秒）、driver/grant/grant sampler benchmarkの3/3（7.56秒）を合わせ重点8対象が通過した。更新したwireも再確認で通過した。最終Release/SAN buildのwarning/errorは0件。全legacy SAN suiteやPython SANを再実行した意味ではない。

nativeではself80/36の一入力解決と無RNG・予約補充、foreignへの適合Stone防御・Cold/Wide除外、同番号受信とNoneの本人表示差・他者非漏洩、使用/reveal・全解除、pendingからの再開・grant proof改変拒否を検証した。C/Pythonは外部同番号授与・無RNG・fresh unused・実体DEF・公開reveal、型/NULL/旧幅、late failure原子性を含む。旧Dream生成器の参照比較と全旧Releaseテストを維持し、合成回帰を公式の追加使用観測へ数えない。


## v64：Dream偽装の実使用と4受信checkpoint

runtimeを変更せず、公式CPU修行のupdate4〜7を新たに保存した。通常195の自己使用と195/195受領、135/135のDEF5応答、195/195の自己回復、197/fake195の自己回復を4完了例として数える。CPU自動後続を独立例へ重複加算せず、累計28完了例・native対象25例とする。先行GF2空防御は今回の独立完了件数へ加えない。

見かけ195/MP＋5に対し実197でMP＋15となる受信例を得た。135/195同番号の使用も受信fake field付きで確認したが、異番号pairは旧9key互換内である。候補集合・全50%分布・不適合入力・非zeroコスト差への一般化は行わない。CPU188/196/202は在庫差分からの明示completion、recipient順はローカル規約のままである。

テストは再構成GF3 sceneから4受信checkpointを既存固定8seedで32比較する。全物理モデル・fake presence・unused・資源・Dream・GF/owner/pendingを照合し、各native境界のsnapshot/restoreと使用時公開を検査する。原初期draw/order・game3・server updateCountとnative decision数・CPU内部gift/retry順は比較対象外である。公式の連続した開始〜終局game列1対局分・全backend内部trace0、旧世代資料・pins・legacy37を維持する。

ReceivedStateも同じ4記録を直接読み込み、135/135・195/195・13/13のpresenceと197/fake195の真値保持、本人のface/referenceが195のままであること、CPU空slot1→0、先頭攻撃23と後続main、spectator手札なしを回帰検査する。Python Stateのunique methodsは18→19、Matchは163→164、native231は変更しない。これらは受信frameの読み取り・投影の検査で、pending Matchを暗黙再開する入口ではない。

Python Matchには受信update6のquiescent sceneを明示importし、197/fake195の本人face/reference195→自己使用MP35・公開reveal→13/13補充→CPU195自己使用と202差分補充→update7の資源と両者の手札投影を比較する回帰を加える。これは先行4checkpointのnative再生とは別の、既存C/Python接口を通す1区間の検査である。


最終Release CTestは156/156（16.82秒）通過。更新したwire targetはRelease1/1（0.29秒）、ASan/UBSan1/1（0.47秒）通過し、今回の両wire buildのwarning/errorは0件。Python State19・Match164 unique methodsを含む全Releaseを確認した。runtime本体の変更はなく、SAN全suiteやPython SANを再実行した意味ではない。

独立監査では、再構成前の両18行・4終点の両72行をfixtureと機械照合し、model/fake/used・受信順・再構成slot順が一致した。4 private原本からの匿名化も全件差分0、公開artifactのbytes/SHA256はデータREADMEと照合した。旧世代の記録・oracleは変更していない。


## v65：Fate288の受信再生とNeptuneの効果単体

2対局各3更新を権威的初期状態から保存した。新4viewer操作（201召喚、233壁、207/Fate288、142防御）のうちFate対局2操作を全体のnative投影へ加え、累計32完了例・native対象27例とする。Neptune対局2操作はrecord-onlyのまま、276のHP5効果のみを別枠8component検証へ分ける。CPU自動後続や初期手札探索6対局を独立完了例へ重複加算しない。

Fateは明示288結果を既存所有FateActionへ渡し、固定8seedで初期・Fog/Cobra保留・142防御後の3checkpoint＝24比較を行う。全positiveモデル・fake省略・used・資源・Fog・GF/owner/pendingを比較し、各native境界をsnapshot/restoreする。明示192/166/CPU26配送の前後はRNG不変、Fog一人対象drawは保持する。server updateCountとdecision数、現象selectorやCPUの省略gift/retry・授与順は比較対象外である。

Neptune対局は201→Neptune→gift196、block→276/HP＋5→gift123という有限受信順を保持する。276componentは最終受信の正の在庫とused233/231を固定し、HP5だけを戻して外部GuardianActionを適用する。これは自動scheduleや201 selectorを再生した比較ではない。旧pins・oracle・観測世代・legacy37とruntime56/Snapshot56/view-v54を維持する。


Pythonの受信frame回帰では両対局6状態を読み、Fog時の他者資源非表示、Neptune、used233/231、empty0/2/0および0/1/0を確認する。Python Matchでは権威的Fate初期sceneから207/288・明示192・CPU76・142防御・166/CPU26補充をC接口経由で再生し、最終資源・両者の手札投影を比較する。State20、Match165 unique methods、native Match231 callbacksである。

最終Release CTestは156/156（13.14秒）通過。更新wireはRelease1/1（0.30秒）、ASan/UBSan1/1（0.49秒）通過し、両wire buildのwarning/errorは0件。runtime本体は変更せず、SAN全suiteやPython SANを再実行した意味ではない。

独立監査でprivate6ゲーム状態の匿名化後と両wire記録が全件一致し、Fate54行・276component20行の数値転記も一致した。fixed8seedに検索/skipはなく、selector未再現の2観測とfull Fate24 checkpoint・別枠component8を区別する。公開artifactのhashはデータREADMEと照合した。旧世代記録・oracleは変更していない。


## v66：特殊自己使用2例と状態投影の限界

45自己使用と14自己使用の2対局を収録し、累計34完了例・native対象29例とする。各before/after2状態×固定8seed＝32比較、全model/fake省略/unused・資源・GF/owner/pending・各native境界のrestoreを確認する。45後CPUのBUYは未応答responseで止め、購入offer RNG/YESNOへ入らない。14は先行CPU197後の受信GF2からロードし、そのCPU操作と元初期draw/orderを再現したとはしない。CPU47は在庫差分である。

45の受信dealDamage12→boostHP12とローカル先healは順序が違う。HP40では両方40なので終点比較は可能だが、ordered-resource-event-replay=falseとして内部clip/死亡順は未解明と記録する。14はdamage3/nextAttack/damage3と一消費/一giftを受信した。どちらも単品・非致死の有限例で、一般吸収・二回攻撃scheduleの完成とはしない。runtime56/Snapshot56/view-v54・旧データ/legacy37を維持する。


Python Stateは両対局4状態を直接読み、viewerが1/2の両配置、slot6の45→161/14→19、absorb後の161同モデル2copy、BUY先頭frameとemptyを検査する。Python Matchは受信beforeから自己play・明示結果・次CPU行動をC接口経由で再生し、両者の資源と手札投影を比較する。State21、Match166 unique methods、native Match231 callbacksである。

最終Release CTestは156/156（18.00秒）通過。更新wireはRelease1/1（0.41秒）、ASan/UBSan1/1（0.63秒）通過し、両wire buildのwarning/errorは0件。runtime本体を変更しておらず、SAN全suiteやPython SANの再実行を意味しない。

独立監査でprivate4状態の匿名化と両wire記録が全件一致し、native72行の数値・配列順も一致した。45の終点比較と内部順未検証、14のGF2開始、BUY未応答停止、CPU47のdelta区分、新2/cumulative34・native29を確認した。公開artifact hashはデータREADMEと照合し、旧世代記録・oracleは変更していない。


## 作業世代v68：低HP自己吸収と39の直接自己使用

新しい公式CPU対局の初期状態から6更新を受領し、自己39＋162・CPU19への空防御・自己19・CPU67への空防御・自己27の5操作を保存した。本人HP列は40→15→10→5→3→7。27のHP3・被害7→回復7・生存結果は従来bodyの先healによるHP3と区別できるため、自己aliasだけdamage→nominal healへ修正した。39＋162の最初の操作は公式で1command解決するが旧nativeはresponseを待っていたため、武器39のreflect_miracleも明示selfのDEF0直接経路へ追加した。両変更はruleset57/Snapshot57/view-v55で固定し、ABI2/構造体幅/台帳は維持する。

`official_wire_trace` は既存8seedを探索・skipせず、6state＝48追加比較が一致する。初期在庫から通常Commandと明示ArtifactGrantで進め、受信damageをeffectへ注入しない。各boundaryの完全private restore、消費copyと再利用slotの写像、CPUの省略giftを在庫差分として入力する範囲を検査する。raw server updateCount・全配布順・server RNGや、start→endの完全対局再現は対象としない。公式観測39・native対象34・record-only5・wire224比較、完全な受信start→endは従来1対局のまま。

新しいnative群を含む233callback、Cのtyped低HP/上限/208112/phase・出力原子性、Python168methodを検査する。合成境界は45/216、99上限、nominal192、counterのtarget実損失基準、Heavenfall prefixHP0、208/112、未選択防具、38維持と防具非拡張を含む。これらを新たな公式実測へ加算しない。独立HP reducerは10群となり、v68の受信3→0→7と従来HP2,617行×2分岐を検査する。

Release全CTest157/157（17.96秒）、Debug ASan/UBSanの変更4target（core/C API/wire/HP）4/4（18.07秒）、compile warning/error0、`git diff --check`通過。両build/suiteを別directoryで並行実行し、全SAN suiteやPython SANを実行したとは主張しない。最終native/C/Python・fixtureは独立のread-only監査も受け、raw source6件のhash、mode0600、匿名化以外のgame literal一致を確認した。旧世代pins・records・legacy37は保持する。


## 作業世代v69：受信19/fake27と一新後の局所再現

公式CPU修行の7受信更新に、非終末178（safe→Dream→replaceItems8→gift1）、19/fake27自己使用、113/fake135防御、両替、空防御、CP0の他者BUY拒否という6操作を保存した。実19/face27は旧9fieldのabilityが一致せず、見かけ攻7/吸収から実5被害・回復なしで解決する。この方向だけ受信predicateへ追加し、選択前の真値gateを増やさず、通常generatorは旧9key・候補・率・乱数消費を維持する。v58/Snapshot58/view-v56、既存ABI2と型幅を維持する。

新native wireは受信update2（一新後GF2）を初期sceneとしてupdate2〜6の5状態×既存固定8seed＝40比較。一新の8drawと偽装、複数候補からモデル5を選んだBUYをreplayするためのseed探索や未観測結果の注入は行わない。CPU105/142は在庫差分として供給し、server内のgift順を推定しない。資源・原所有者/応答者・Dream/fake presence・全positive在庫・placeholder・slot再利用と各native境界の完全private restoreが一致する。

native234 callbackとC/Python回帰は、typed19→27受領、同face27で異なるactual19/27の公開view/合法性/policy sampleの非干渉、実5被害・reveal・消費、reverse/他pair/noDreamの拒否・原子性を検査する。Pythonは170 method。CPU修行では所持金0・MP30で価格5のモデル5がbuy→canNotBuyとなりHP13/MP30/CP0を維持したため、既存CP-only gateを変更しない。この1例を全商品・self BUY・一般CP debt順の同値性へ拡張しない。

Release CTest157/157（16.60秒）、Debug ASan/UBSanのcore/C API/wire3対象3/3（18.41秒）、compile warning/error0、diff-check通過。全SAN suite/Python SANの実行とは区別する。fixtureはraw7sourceのhash/mode0600と匿名化以外のgame literal完全一致を確認し、独立read-only監査も受けた。完了観測45・native対象38・record-only7・wire264checkpoint、完全なstart→end受信列は従来1対局のまま。最終CPU BUYは未応答で退出・ホーム復帰・Network Monitor終了を確認した。


## 作業世代v70：外部置換結果を含む178の元action再生

新しい公式観測は増やさず、v69のreplaceItems8を `RedrawResponseAction` で供給する。通常Commandとは別のprivileged入力とし、全選択consume後・各178のDream後に同数のfresh unusedへ置換してから既存incoming bodyへ進める。reservedgiftを置換に混ぜず、置換のgenerator/RNGだけを差し替える。通常step/driver/samplerとlocal redrawは旧経路のまま。rules59/Snapshot59/view57、C ABI2・既存構造体幅は維持し、追加型8/16byteとC/Python入口を公開する。

ActionBatchの疎なresponse_index結果と非再帰redraw_proofを保存し、普通の応答・guardian/payload・phenomenon・Fate・Devil/112・grant interleaveを元actionから再生する。grant_proofとの共存は完全一致を要求し、finish/cutoff/次main/externalのsync/clearを検査する。空置換がdefault0drawと状態同値な場合、呼び出し由来だけの別markerは置かない。入力不足・個数/モデル/fake/padding/extent不正や後段失敗ではworld/RNG/outをcommitしない。

新v70wireは、受信CPU35/slot2の消費copyだけを戻したGF1 scene→launch→178＋供給8枚→別のCPU5在庫差分と本人203通常gift→既存4操作までを8seed×6状態＝48比較する。元のCPU初期draw/order、CPU-vs-viewerの暗黙gift順は復元しない。v69post-scene40も維持するため、同じ受信状態を別経路から検査する重複を含む総312比較となる。観測45のまま、既存178の1例を条件付きnative対象へ移して39、record-only6とする。低CP BUYの多候補selectorは再現しない。

native238callbackは、used置換・複数178・空結果・新208/112のbody接続・cutoff・次main BUYへのproof非干渉・全origin・snapshot偽造・token/update後半失敗・通常step/driverのstate/RNG一致を検査する。C bridgeはnested storageの借用/所有copy、8/16layout、NULL・extent・padding・fake presence・out canary、C/native parityを検査し、Python173methodにはfrozen DTOの所有tuple・実8枚・空/multi/失敗回帰を含む。

Release157/157（18.05秒）、Debug ASan/UBSanの変更3target3/3（18.50秒）、compile warning/error0、C11 -Werrorの新旧型layout/prototype、実行規約のPython使用例、diff-checkが通過した。全SAN/Python SANや公式selector分布の証明ではない。独立read-only監査も通過し、既存v69/pinsは変更せず、新しい再生reportへ検証範囲と結果を保存する。


## 作業世代v71：既知の商品選出から購入拒否を再生

通常のpolicy選択と分けた `PurchaseResponseAction` で、応答解決時のunused seller tokenまたは空候補を供給する。選出だけgame RNGを使わず、CP-only判定・返却・確認・配送は既存処理を共有する。full-origin replayへ疎なresolved_purchasesとpurchase_proofを接続し、grant/redraw journalとの一致、完了・cutoff・次actionでの保持/消去を検査する。rules60/Snapshot60/view58、旧C ABI2/型幅を維持して16byteの追加型を導入する。

新しい公式観測は0。既存v69の7受信状態を一新前から8固定seedで56比較し、商品5のCP0拒否・同コピー返却・237補充・返却品による次のCPU BUY保留までを条件付き再生する。v70とv69の独立経路も維持するため、重複込みでwire累計368比較。観測45例中のnative対象を40へ、record-onlyを5へ更新する。最終CPU BUYの応答、商品選出分布、一般server scheduleは未検証のまま、start→end受信列も従来1対局である。

検証はRelease CTest157/157（39.39秒）、Debug ASan/UBSanのcore/C API/wire3対象3/3（14.55秒）が通過。native241群・Python175method、compile warning/error0、C11 -Werrorの新旧layout/prototypeと実行規約に掲載した購入APIのPython使用例も確認した。時間はsuiteの経過で、速度比較には使わない。全SAN suite/Python SANの実行とは区別する。


## 作業世代v72：41の直接自己使用と4受信終点

公式CPUの単品41自己使用で、防御を挟まず11被害・暗雲付与・補充となることを取得し、旧追加response待ちを修正する。41だけの新trait経路をrules61/Snapshot61/view59へ固定し、ABI2/型幅・生成器は維持する。追加材・cutoff・208・foreign/redirect等はnative/C/Python合成境界と区別する。

update2〜5の4受信終点を、消費41とCPU92だけを戻したGF1再構成sceneから8seedで32比較する。179で闇10を防ぎ、49の自己13被害後、CPU19+43+25の22へ空防御して終局する。初期wire/元draw/orderとbackend補充順は未取得で、完全start→endの累計1は増やさない。新観測4・累計49、native対象44・記録のみ5、wire重複込み400。低HP45の検証は準備中の致死攻撃で未成立と記録する。

混乱を起点とする供給BUYと、購入後の既存Saturn自動応答・190反射のwhole-origin proofを独立した合成2群へ追加する。これは公式の混乱/守護神selectorの再現や、新しい公式観測ではない。

検証はRelease全157対象のうち155が初回（38.78秒）に通過し、追加C/Pythonテストのsemantic legal期待を修正後、該当2対象を再実行して2/2（8.05秒）通過した。runtimeはこのテスト修正では変更していない。Debug ASan/UBSanは初回core/wire通過、Cを同じ期待修正後に再実行し1/1（1.29秒）通過し、主要3対象を確認した。単一の全件clean runや全SAN/Python SANとは区別する。native244群・Python176method、両buildのwarning/error0、diff-check通過。


## 作業世代v73：55の直接自己使用と2件目の完全受信列

55の単品自己使用は1操作で15被害・補充となるため、武器blockMiracleの余分なresponse待ちを修正した。55実測と42/50/56の同能力への限定一般化を区別し、roles/foreign/redirect/Wide/guardianを従来経路に保つ。rules62/Snapshot62/view60へ更新、ABI2/型幅は維持する。

今回はauthoritativeなinitial/update1から終局update4まで取得し、固定8seedで32 checkpoint比較する。55→184補充→CPU40→184/11被害/22反撃→CPU190反射→空防御の死亡を、受信ATKの注入やseed選別なしで解決する。初期handとCPU操作・通常184受領は明示入力なので、初期配布分布・CPU選択方策・server RNGの証明ではない。非致死counter1例と、clip/死亡/複数counter/FIFOの未検証部分を分ける。

新3完了操作で観測52/native47/record-only5、wire重複込み432。新たな完全start→endの受信列1対局を加えて累計2とする。165の不適合クリックは未選択で完了件数やrouting-only件数へ加えず、terminalは受信本文で確認する。ホーム復帰とNetwork Monitor終了を確認した。

Release CTest157/157（39.27秒）、Debug ASan/UBSanのcore/C API/wire3対象3/3（14.22秒）が通過した。native245群・Python177method、最終両buildのwarning/error0、diff-check正常。全SAN suite/Python SANや、版間の速度比較とは区別する。


## 作業世代v74：21の自己使用と欠番付きendpoint

21のself40→35・追加防御無し・次CPU BUY待ちをUIで観測し、武器bounceWeaponだけを直接自己使用へ追加する。34/bounceMiracle、応答/redirect/Wide/guardianは維持する。rules63/Snapshot63/view61へ更新し、ABI2/型幅と通常の弾き選出/商品selectorは変更しない。

authoritative更新は1と3のみで、missing2を明示する。初期手札から21操作、44在庫差分、CPU BUY5、正確なseller23 token、CPUの購入成立、13在庫差分を入力し、更新3のCP22/18・seller手札8/buyer10へ再生する。8固定seed×受信2状態＝16比較、21直後とquoteはlocal中間検査である。新2completed UI観測で累計54/native49/record-only5・wire448、新しい完全受信lifecycle0/累計2を維持する。raw use21/gift44やCPU launchを取得済みと表現しない。

Release CTest157/157（39.40秒）、Debug ASan/UBSanのcore/C API/wire3対象3/3（14.14秒）が通過。native246群・Python178 unique method、両buildのwarning/error0、diff-check正常。全SAN/Python SAN、一般の商品分布・server RNGの同値性とは区別する。


## 作業世代v75：58の実測済み範囲を追加

58単品自己使用をHP40/MP10→HP20/MP0として初期1と使用後2の2受信状態で比較する。実gift23を供給し、CPU221保留まで再生する。既存固定8seedを探索/除外せず16比較、完了操作は58の1件だけを追加する。累計55/native50/record-only5・wire464、routing-only1・完全start→end2・backend内部trace0を維持する。runtime変更・API追加はない。53、低HP45/216、58の合成/FREE/MP境界等は残件。ホーム復帰とNetwork Monitor終了を確認した。

検証はRelease CTest157/157（28.96秒）、変更したwireテストのDebug ASan/UBSan1/1（0.69秒）が通過した。両buildのwarning/error0、diff-check正常。runtime/APIは変更せず、全SAN suiteやPython SANの実行とは区別する。


## 作業世代v76：Cold自己使用のrouting修正

48の自己使用で13被害→Cold→owner tick1被害→通常58補充を受信し、余分な本人responseを取り除く。33/62は同traitによる限定一般化、Hell/Flash/Fogや他entryは従来経路に保つ。rules64/Snapshot64/view62・ABI2固定。

初期1/使用後2を固定8seedで16比較し、累計56観測/native51/record-only5・wire480へ進める。初期両手札/CPU SELLの選択は入力、58は実giftの供給、病気は既存ローカルRNGで進める。サーバー乱数との同一視・悪化率推定・枝の探索/除外はしない。未応答SELL/途中退出を新完了操作や終局に数えず、完全start→end2・routing-only1・backend内部trace0を維持する。

Release CTest157/157（38.70秒）が通過した。Debug ASan/UBSanはcore/wireの2対象2/2（13.22秒）と、別実行のC API1/1（1.47秒）が通過し、主要3対象を確認した。全SAN suite/Python SANや一回の3対象合計時間とは区別する。native247 callbacks・Python179 unique methods、最終buildのwarning/error0、diff-check正常。
