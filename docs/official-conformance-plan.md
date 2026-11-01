# 現行公式への適合計画

**配信最新版との差分確認（2026-11-02）:** fresh取得の公式JA/JSは固定snapshotとhashが異なる。296モデルの数値・能力・curse・giftRateは比較済みで変更0、差は画像名19/26/117と117の表記「石版→石板」等である。これら表示名/画像名の追従は未反映とし、既存pinsを維持する。JAにはスペイン語の言語項目追加とcostMP/priceCP文言の再編もある。最新版の除去handler・描画・reveal・decode経路は識別子を正規化した制御構造/リテラル比較と手読で照合したが、新JS全体の同値性は保証しない。[取得物と差分証跡](../data/official-v1/upstream-review-2026-11-02.json)を参照。過去のfresh一致記録を今日のhash一致として扱わない。 v20の再取得もこのNov2 reviewの新hash/bytesと一致し、新しい変更は検出していない。元pinsとの差が解消したという意味ではない。

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

今回取得したJSのSHA256は`764a50524e4b6b3f510415da7128abd8ad99dcb87b45b8572d98ddd96889cabd`、
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
購入・満杯配送等の未対応取引・サーバー受理、一般effect queue・病気tick・守護神・終末等は引き続き実装と照合対象とする。
9人のstate/view対応、限定対局の実行、公式サーバー同等性を同じ完了項目として数えない。

## 限定実行器の段階

`official_match.hpp` / `official_match.cpp` はv1の132モデル・配布重み308、
物理反射6種と属性消去189を加えたv2の139モデル・配布重み319を経て、
遮断9種を加えたv3の148モデル・配布重み328を経て、FREE5種・setter2種・倍化231を加えた
156モデル・配布重み337を経て、跳ね返し8種を加えたv5の164モデル・配布重み351を経て、単純確率攻撃21種を加えたv6の185モデル・配布重み372を経て、攻撃反撃181/184を加えたv7の187モデル・配布重み374を経て、資源反撃187/188を加えたv8の189モデル・配布重み376を経て、HP吸収/同被害5種を加えたv9の194モデル・配布重み381を経て、閃光付与/反撃/解除7種を加えたv10の201モデル・配布重み404を経て、暗雲付与/反撃3種を加えたv11の204モデル・配布重み407を経て、除去2種を加えたv12の206モデル・配布重み409を経て、MP武器58/二回攻撃14を加えたv13の208モデル・配布重み411を経て、全敵化232を加えたv14の209モデル・配布重み412を経て、HP増減202を加えたv15の210モデル・配布重み414を経て、自動復活208を加えたv16の211モデル・配布重み415を経て、杵94/臼209を加えたv17の213モデル・配布重み417を経て、v18で両替3を加えた214モデル・配布重み437を経て、v19で同じプールのまま除去結果を公開し、売却4を加えた215モデル/重み457の `official-execution-subset-v20` へ進めた。
2〜9人/チームで通常行動・確率batchから応答、反射/跳ね返し連鎖・遮断、効果適用、
死亡、補充、次の生存者へ進む実験用対局を実行する。固定座席の報酬、自然終局/打ち切り、
別policy乱数、プログラム内snapshot再開、C/Python実行APIを持つ。

資源更新は公式client由来7,898ケースで照合する一方、ATK−DEFの残余算式、
補充枚数・発行順、即時脱落・GF/次手番の順はまだ公式server traceで確定していない。
それらを明示したローカル規約は[限定実行器の契約](official-execution-subset.md)へ固定する。
未対応モデル/状態は拒否し、通常攻撃へのfallbackや237全配布での実行は行わない。

反射は現在source/targetを交換し元のpayloadとownerを保ち、消去後の無属性を維持する。
原発奇跡を再課金・新sourceへ取得させず、応答者ごとの消費と補充予定をledgerへ記録する。
Snapshot version20は全action journalの特権的token/抽選/反撃来歴を検査し、旧version1/2/3/4/5/6/7/8/9/10/11/12/13/14/15/16/17/18/19を拒否する。公開observeへtoken・乱数anchor・未来RNGを出さない。
自己反射もresponseを継続するのは明示したローカル規約であり、clientの自己target表示だけからserver規則を確定しない。

v3の魔法遮断8種は現行の回復奇跡HP235/CP236も止め、武器遮断233は無属性武器にmatchingする。
MP増加奇跡は現行台帳に無く、categoryで判定する一般predicateからMP回復雑貨の遮断を導かない。
＜壁＞単品は初回/再使用ともMP6、初回はused保持し、補充は空き次第で1または0。v4は直後FREEによる費用0も許可する。
blockは現在payload全取消の終端応答とし、reflection ledgerへ保持しない。各選択の消費・費用・予約を処理後、
未終局/未打切りの場合だけまとめて配布して元ownerから巡回する。この全体順はclientのblock演出だけで証明できず、実験規約である。
v3/v4の公開JSONはofficial-execution-view-v2、v5は跳ね返し件数/規約を加えたview-v3、v6は現在命中率とchance_ruleを加えたview-v4、v7はpending.counterとcounter_ruleを加えたview-v5、v8はresource_counter_ruleを加えたview-v6、v9はdamage_effect_ruleを加えたview-v7、v10はflash_ruleを加えたview-v8、v11はdarkcloud_ruleと現在forced_hitを加えたview-v9、v12はremoval_ruleを加えたview-v10、v13はmana_weapon_rule/repeat_attack_ruleを加えたview-v11、v14はwide_attack_ruleとrepeat規約更新を加えたview-v12、v15はrandom_hp_ruleを加えたview-v13、v16はrevival_ruleを加えたview-v14、v17はdanger_ruleと実行済launchのlast_dangerを加えたview-v15、v18はexchange_ruleを加えたview-v16、v19はremoval_event_ruleとlast_removalを加えたview-v17、v20はsell_ruleとlast_saleを加えたofficial-execution-view-v18とする。v18のMatch C ABIはtyped allocation追加のため2へ進め、catalog/state ABI1は維持する。

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

全効果の一般キュー、閃光/暗雲以外の災いcounter・購入/満杯配送・吸収/閃光以外の固有効果付き確率攻撃・霧/夢の実効果・守護神・終末と、
公式の消費/補充/死亡/進行の実対局照合は引き続き未完了である。
限定対局が動くことを、公式サーバー全体との同等性の証明として扱わない。
v1の132モデル、v2の139モデル、v3の148モデル、v4の156モデル、v5の164モデル、v6の185モデル、v7の187モデル、v8の189モデル、v9の194モデル、v10の201モデル、v11の204モデル、v12の206モデル、v13の208モデル、v14の209モデル、v15の210モデル、v16の211モデル、v17の213モデルとv18の214モデルの計測・checksumは過去の証拠として保持し、現行v20の215モデルの検証とは分ける。


## v19：直近除去結果の公開と検査

v19はモデル/重みを増やさず214/437（trade1/98武器/74防具/16雑貨/25奇跡）を維持し、除去結果の公開gapを解消する。rulesetは `official-execution-subset-v19`、Snapshot19（旧1〜18拒否）、公開schemaは `official-execution-view-v17`。Match ABI2/command184/config24/status248、catalog/state ABI1・legacy37は維持する。

`last_removal` はnullまたは `{origin,source,target,model,removed_models}`。originは原発者、source/targetは反射後を含む解決時の現在座標、modelは205または206の単一整数ID、removed_modelsは実際の抽選順の整数ID配列で同モデルのコピー重複を残す。最終bodyが解決したときだけ作り、候補0の解決も空配列[]のeventを残す。使用品自体の消費や後のgiftをremoved_modelsへ混ぜない。

次のmain commandの受理で消去し、不正/失敗commandでは保持する。response処理自体は消去契機ではないが、現205/206は単一bodyのaction完了時にeventを作るため、response phaseとeventは同居しない。reset/load_sceneで消去する。解決後のcutoffではeventを残し、未解決でcutoffした場合はnullのままとする。token/slot・RNG・元eligible数・非除去の他者手札を公開しない。これは全履歴の蓄積ではない。

公開 `removal_event_rule` は `experimental:resolved-body-only;public-removed-models-in-draw-order;duplicates-and-empty-preserved;clear-on-next-accepted-main;no-tokens-or-rng`。他者が対象でもeventを受信したviewerに除去品を表示する経路には公開clientの根拠がある。ただし全参加者/観戦者へのserver配信や空結果eventの必ずの発行は確認していない。ローカル抽選順をevent順とすること・履歴寿命・cutoffの保持は**公式server未検証の実験規約**である。配布プール、除去抽選、資源/死亡/gift/手番の遷移はv18から変更しない。

復元時はdecisions>0、全endpointが既知、model205/206、countが各3/2以下、対応モデルIDだけ、206なら奇跡のみを検査する。response phaseのevent、last_danger/last_saleとの同居は拒否する。完了raw snapshotは構造整合を確認するだけで、その公開結果が過去の公式またはローカル対局で実際に起きたことまで証明しない。pendingはevent無しを要求し、既存の全action replayを維持する。


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

配送先が実物で満杯なら明示unsupported（-5）でatomicに拒否し、samplerも除外する。自己売却は二品消費後の空きを考慮する。現行clientにはserver指定overflowItemの処理があるが、実物のeviction選択は未確定である。旧Flashの満杯配送失敗を無視して料金だけ処理する挙動を流用せず、満杯配送まで含む完全SELL対応とは呼ばない。

`last_sale` はnullまたは解決時の `{origin,source,target,model,price}`。modelは商品IDで、originは原発者、source/targetは現在の解決座標。解決時のみ作り、次main受理/reset/loadでclear、不正/失敗時は保持、解決済みcutoffでは残し未解決cutoffではnullにする。last_danger/last_removalと排他、pending/response phaseへ完了eventを置かない。token・配送先の他手札・RNGを出さず、完了raw snapshotの構造検査を全過去履歴の到達証明とみなさない。

公開 `sell_rule` は `experimental:one-unused-commodity;explicit-alive-target;zero-mp-fee;one-operation-gift;foreign-response-and-reflection;buyer-debit-before-current-source-credit;fresh-token-delivery-before-death;full-recipient-unsupported`。商品の232/Aura231/MP武器58/杵94/復活208等のabilityを売却payloadの構成時に実行・継承せず、単一対象・power0・属性無し・商品priceだけを用いる。配送後208の独立した救済hookは別に扱う。商品fakeも既存unsupportedのまま。構造体は同じABI2でも、clientは新opcode5の認識とruleset20のloader照合が必要である。

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

詳細は[限定実行器v20記録](official-execution-subset.md#v20の検証と売却集計)。
