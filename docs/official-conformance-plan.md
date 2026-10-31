# 現行公式への適合計画

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
| 除去 | 説明は神器3個、使用済み奇跡2個 | 抽選順・重複・個体公開・empty時の乱数は未検証 |
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
取引配分・サーバー受理、一般effect queue・病気tick・守護神・終末等は引き続き実装と照合対象とする。
9人のstate/view対応、限定対局の実行、公式サーバー同等性を同じ完了項目として数えない。

## 限定実行器の段階

`official_match.hpp` / `official_match.cpp` はv1の132モデル・配布重み308、
物理反射6種と属性消去189を加えたv2の139モデル・配布重み319を経て、
遮断9種を加えたv3の148モデル・配布重み328を経て、FREE5種・setter2種・倍化231を加えた
156モデル・配布重み337を経て、跳ね返し8種を加えた164モデル・配布重み351の `official-execution-subset-v5` へ拡張した。
2〜9人/チームで通常行動から応答、反射/跳ね返し連鎖・遮断、効果適用、
死亡、補充、次の生存者へ進む実験用対局を実行する。固定座席の報酬、自然終局/打ち切り、
別policy乱数、プログラム内snapshot再開、C/Python実行APIを持つ。

資源更新は公式client由来7,898ケースで照合する一方、ATK−DEFの残余算式、
補充枚数・発行順、即時脱落・GF/次手番の順はまだ公式server traceで確定していない。
それらを明示したローカル規約は[限定実行器の契約](official-execution-subset.md)へ固定する。
未対応モデル/状態は拒否し、通常攻撃へのfallbackや237全配布での実行は行わない。

反射は現在source/targetを交換し元のpayloadとownerを保ち、消去後の無属性を維持する。
原発奇跡を再課金・新sourceへ取得させず、応答者ごとの消費と補充予定をledgerへ記録する。
Snapshot version5は特権的token/抽選来歴を検査し、旧version1/2/3/4を拒否する。公開observeへtoken・乱数anchor・未来RNGを出さない。
自己反射もresponseを継続するのは明示したローカル規約であり、clientの自己target表示だけからserver規則を確定しない。

v3の魔法遮断8種は現行の回復奇跡HP235/CP236も止め、武器遮断233は無属性武器にmatchingする。
MP増加奇跡は現行台帳に無く、categoryで判定する一般predicateからMP回復雑貨の遮断を導かない。
＜壁＞単品は初回/再使用ともMP6、初回はused保持し、補充は空き次第で1または0。v4は直後FREEによる費用0も許可する。
blockは現在payload全取消の終端応答とし、reflection ledgerへ保持しない。各選択の消費・費用・予約を処理後、
未終局/未打切りの場合だけまとめて配布して元ownerから巡回する。この全体順はclientのblock演出だけで証明できず、実験規約である。
v3/v4の公開JSONはofficial-execution-view-v2だったが、v5は跳ね返し件数と抽選規約を加えるofficial-execution-view-v3とする。C ABI1は維持する。

v4の無料化は各MCの即後FREEペア全てを免除し、MP不足prefixではなく完成列の最終費用を検証する。
壁→FREEは全選択を消費/used保持する契約で、旧版の未消費suffixは採らない。
追加FREEのATK/攻撃属性はskipし、通常DEFと先頭46のATK12は保つ。setterはATK加算と色上書き、倍化は累積×2/NE合成で順序を維持する。
samplerもMC+FREE pairをatomicに提案し、途中のMP不足だけで完成可能な無料列を除外しない。

v5のbounceでは応答者が新sourceになり、対象は全aliveから自己・味方・旧sourceを含めて一様に選ぶ。入力seat順・SplitMix64有界抽選、候補1人でも1draw、反射0draw、自己response継続は公式server未検証の実験規約として公開する。元power/category/abilityとfilter結果、owner、原発MCの既払い状態は保つ。
保持234は5MP、直後FREEなら0MPで再使用できる。旧physical-only `n*18` や固定32ではなく、`decisions-1` と `n*(hand_limit+99/5)`、各seatのMP/gift/消費token会計でredirect履歴を検証する。主動作の乱数anchorから全履歴を再生してtargetと最終RNGを一致させる。
view-v3の `bounce_target_rule` は `experimental:uniform-all-alive-in-input-order;includes-self-and-teammates`、公開pendingは反射件数と跳ね返し件数を分離する。未解決中の死亡・補充は無く、これもローカル規約として扱う。

全効果の一般キュー、counter・売買・確率・霧/夢の実効果・守護神・終末と、
公式の消費/補充/死亡/進行の実対局照合は引き続き未完了である。
限定対局が動くことを、公式サーバー全体との同等性の証明として扱わない。
v1の132モデル、v2の139モデル、v3の148モデル、v4の156モデルの計測・checksumは過去の証拠として保持し、現行v5の164モデルの結果とは分ける。
