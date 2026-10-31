# God Field 仕様差分表

作成日: 2026-10-30。比較対象: ローカルの `synthetic-neutral-duel-v1`、commit `ddc627f6225fabe93c77ab296e9ee85860ea67ab`。

**これは公式対応に向けた作業表であり、現行公式仕様を完全に確定した仕様書ではない。**
比較基準のC++実装は独自の簡略ルール。最初の対応範囲は1対1とし、複数人戦は後続に分ける。

追記: 比較後に `legacy-neutral-subset-v1` を追加した。以下の差分表の「現在のC++」欄は
上記commitの比較基準として保持する。新しい実装の進捗と制限は次節に記載。

## 現行公式の一次対応範囲（2026-11-01）

[公式公開データ/クライアント調査](official-current-rules.md)、
[fresh state/viewの契約](official-state-view.md)と
[公式適合計画](official-conformance-plan.md)を新しい根拠とする。
[data/official-v1](../data/official-v1/README.md)に現行公開台帳296モデルを独立固定し、
通常配布の正のgiftRateを持つ237モデル・重み合計500を確認した。
旧版37台帳・対戦Env・全C ABI/Pythonは維持し、旧IDを現行へ流用しない。
公式JSONのSHA256は `03a4782dc6899aeedd9b7986a49bd3c0217b96ac38c0779deb588bc94cca3e7c`。
client JSのhashが以前のB記録と一致しても、別取得のJSON台帳が更新されていない証拠にはならない。
Bの291神器と今回の全296モデルの差5は、Bが除外したtrade操作5件を含む集計範囲の差である。
件数差だけから5件の追加更新が起きたとは判断しない。

| 現在実装済みの範囲 | 根拠と限界 |
| --- | --- |
| 公式296モデルのtyped C++台帳 | 公式JSON全item fieldとJSの欠損default/配列index+1を固定。説明・画像・client全文はrepoに複製しない |
| `godfield::official` pure composition | 順序付き威力/属性/MP表示、反応/属性防御などの公開client関数を移植。選択列を受け取る計算であり、ターンやHPを更新しない |
| visible追加predicate | 見かけのモデル列に対するmain/response追加可否を再現。夢の真値、所有/容量/MPを含むserver全体の合法性判定ではない |
| 最大9人のデコード済みスナップショット | Team ID0〜4・dead・資源・災い・守護神・token/model/fake/usedを区別。9人の対局を進行する実行器ではない |
| fresh状態欄・自己手札・使用済み奇跡の部分view | 参加者の別名、霧の生存敵statusと別のusedパネル、見かけfaceと死亡/終局後tooltipを分離。他者unusedの枚数、user ID、transport更新番号を出さない。攻撃/rating/全イベント画面は未投影 |
| 本人のinstance選択照会 | token重複・表示モデルのappend・自己MP・祈り・取引最低枚数・本人の閃光を確認。真値を見た先行拒否はしない。actor/phase/手番/target/server受理の全合法性ではない |
| 実clientのオフラインoracle | 認証・DOM・network起動を止めた実JSの関数を実行し、50,270件のgolden vectorでC++と一致。旧serverテストを公式照合と数えない |
| 状態helper/表示分岐のoracle | 実client関数と抽出した霧の条件等22,140件が一致。tooltip/視覚部品のstubを含み、全UI操作やserver状態遷移のoracleではない |
| `gfo_*` ABI1 / Python `OfficialData` | read-only台帳と合成preview。ゲームEnv、step/reset、対局の実行・報酬は提供しない |
| `gfo_state_*` / Python `OfficialState` | C APIは型付き配列の所有コピー、PythonはJSON/dictのimportとdeepcopyしたrawの保持、viewと選択照会。入力フレームの状態を保持するが、対局を進めるstep/resetや乱数生成は提供しない |

公式完全シミュレーターは未達である。decoded frameの9人・チームと、freshな状態欄/手札の表示は対応したが、
多人数/チーム対局のターン進行・effect queue、配布/命中/状態の抽選、守護神、終末/悪魔/超常現象、死亡/勝敗を含むserver実行器が残る。
active攻撃/購入表示・モード依存rating欄、増分イベントとプレイヤーの記憶も今回のviewへは投影していない。
公開説明・client関数・実対局/権威的serverの結果を別の証拠として管理し、未観測の順序を旧実装で埋めない。
学習器と学習済みモデルは削除済みのまま、この対応範囲へ含めない。

全体検証: 通常CTest125件（20.03秒）・ASan/UBSan86件（50.83秒）が通過した。
その後の不可能な文字列長（SIZE_MAX）の拒否追加後は、影響する通常4件（0.86秒）・SAN3件（1.13秒）を再検証した。
公式296 factsと旧37台帳hash、generator、候補crosswalk、実client由来の計算/選択50,270件、
追加の状態helper/抽出分岐22,140件のgolden vectorが一致した。
Pythonは台帳15件・状態9件、C ABI合成4,878ケース、native C ABI境界と非干渉を検証した。
[状態oracle metadata](../data/official-v1/state-vectors.manifest.json)のとおり、
実関数に加え抽出分岐・tooltip等の描画stubを含む。実アプリの全画面操作を完走したという意味ではない。
今回公式対局traceは取得しておらず、公開clientから権威的server engineの全体を取得できたとは扱わない。
したがってこの検証数を、公式ゲーム全体の完全対応やserver状態遷移の一致の証明にしない。

### 状態・観測対応で区別する境界

`official_state.hpp` の投影は `official-fresh-view-v1`。霧は生存viewerが霧を持ち、未終局である場合の生存敵statusを覆う。
使用済み奇跡の別パネルでは名前/teamとfake優先のモデル列が見えるため、全情報を一律に隠さない。
観戦者には本人手札を与えず、終局でも他者unusedの自動公開は行わない。
夢はfakeの存在印と表示モデルを可視とし、真モデルのtooltipは本人dead/終局時に限って開示する。
本人の手札はused後方・見かけモデル順で安定ソートし、fresh描画が扱える18個までを照会・投影の範囲とする。
18個はserverの物理手札容量の主張ではなく、他者usedパネルへ一律に課す制限でもない。

選択照会は本人のtokenを使い、見かけモデルと公開自己MPを参照する。閃光はcallerが迂回できないよう本人の災いから判定する。
祈り・捨てる/ささげる・売るの選択数まで検査しても、現在その人が動けるか、どのtargetを選べるか、
夢の実体をserverがどう解決するかは別の未実装条件である。詳細は[状態と表示の一次根拠・API契約](official-state-view.md)を参照。

[OfficialState](../python/official_state.py) はdict/JSON文字列または `from_file` で読み込み、
`view(player_id=... / user_id=... / spectator=True)` と `can_append_hand` / `selection_ready` を提供する。
`raw_snapshot` は未知フィールドや未投影のattacks/eventsも保持した元JSONのdeepcopyであり、
それ自体を秘匿条件に従うviewとみなしてポリシーへ渡してはならない。
読み込みで検査する既知fieldの型・数値域は [state-schema.json](../data/official-v1/state-schema.json) に固定した。
応答の `ResponseContext` は既知の公開情報として明示し、未投影のraw攻撃から自動でturn/targetを推定しない。

## 差分表作成後の実装進捗

新profileの詳細は[旧版中立ルールセット](legacy-neutral-profile.md)と
[奇跡を有効にしたv2](legacy-miracles-profile.md)、[基本4属性を有効にしたv3](legacy-elements-profile.md)、
[確率攻撃を有効にしたv4](legacy-chance-profile.md)、[無属性武器の反応を有効にしたv5](legacy-reflection-profile.md)、
[病気を有効にしたv6](legacy-disease-profile.md)、[Glory・DarkCloudを加えたv7](legacy-harms-profile.md)、
[光・闇とGlory付与源を加えたv8](legacy-celestial-profile.md)、
[攻撃奇跡を加えたv9](legacy-offensive-miracles-profile.md)、
[防御奇跡を加えたv10](legacy-defensive-miracles-profile.md)、
[攻撃属性付け替えを加えたv11](legacy-attribute-change-profile.md)、
[防御時の属性消去を加えたv12](legacy-attribute-removal-profile.md)、
[土の反撃を加えたv13](legacy-counter-profile.md)、
[異種反撃と順序を加えたv14](legacy-counter-extensions-profile.md)、
[所持金反撃を加えたv15](legacy-money-counter-profile.md)、
[魔法反射を加えたv16](legacy-magic-reflection-profile.md)、
[万能反射を加えたv17](legacy-any-reflection-profile.md)、
[MP武器を加えたv18](legacy-mana-weapon-profile.md)、
[魔法遮断を加えたv19](legacy-magic-block-profile.md)、
[追加ATK奇跡と順序付き攻撃を加えたv20](legacy-additive-miracles-profile.md)、
[倍化奇跡を加えたv21](legacy-doubling-miracles-profile.md)、
[全体化奇跡を加えたv22](legacy-wide-miracles-profile.md)、
[MP無料化を加えたv23](legacy-magic-free-profile.md)、
[保持の忘却を加えたv24](legacy-forgetting-profile.md)、
[自己資源交換を加えたv25](legacy-exchange-profile.md)、
[死亡確定前の自動蘇生を加えたv26](legacy-revival-profile.md)、
[手札除去を加えたv27](legacy-item-removal-profile.md)、
[HP吸収を加えたv28](legacy-hp-absorption-profile.md)、
[自己被害武器を加えたv29](legacy-self-damage-profile.md)、
[杵/臼を加えたv30](legacy-pestle-mortar-profile.md)、
[強制売却を加えたv31](legacy-selling-profile.md)、
[購入確認を加えたv32](legacy-buying-profile.md)、
[HP増減賭けを加えたv33](legacy-hp-gamble-profile.md)、
[物理品の魔法跳ね返しを加えたv34](legacy-magic-bounce-profile.md)、
[物理追加ATK238を加えたv35](legacy-sundry-boost-profile.md)、
[状態・解除奇跡を加えたv36](legacy-status-miracles-profile.md)、
[死亡時効果付き武器の通常使用を加えたv37](legacy-dying-weapon-profile.md)を参照。
現行公式との直接照合は上節の公開台帳・client計算/visible選択関数まで完了し、対局全体の遷移は未完了である。
各行の「残る条件」は、その行のprofile時点での範囲を示す。後続profileで実装した機能も含むため、
最新のlegacy範囲はv37の行と各profileの仕様、現行公式の範囲は上節と公式適合計画を参照する。

| 対象ID | 追加した実装 | 残る条件 |
| --- | --- | --- |
| D01・D04 | 出典・hash付き60種台帳、旧ID・価格・重み、未対応IDの拒否 | 全神器の台帳と現行ID対応は未完了。60種内で重みを正規化する独自プール |
| D02・D03 | 旧版profileのHP40/MP10/所持金20、初期手札10、容量16 | 現行初期値と衝突C03は未確定 |
| D05〜D08 | 先頭と追加ATK、攻防兼用、武器の捨て禁止、複数捨て、条件付き祈り | 中立・単純効果の対象神器のみ。特殊な組み合わせは未対応 |
| D09・D10・D14 | 通常手番と応答の分離、周期ごとの順序、配布予定、手札の詰め、0防御と過剰防御。後続profileでは対応済み属性・病気・反応連鎖も扱う | 未対応の固有効果と複数人戦における処理順の照合が必要 |
| D11 | opt-inのv2で単純奇跡2種の初使用・保持・重複数・再使用・MP消費を追加。再使用時も配布予定+1 | 旧版の数値であり現在の公式値とは未一致。属性付き奇跡、防御用奇跡、忘却などは未対応 |
| D12・D15 | 中立のHP/MP回復、相手を対象にした応答、自分への攻撃。v2は奇跡によるHP・所持金増加も扱う | 売買・資源交換は未対応 |
| D13・D14 | v3で基本4属性54種を追加。無属性を含む攻防の合成、相克防御、不適合な混合防御の拒否を実装 | 光・闇の衝突C01は未解消。ADD_ATTRIBUTEなど順序依存効果、特殊防御、属性付き奇跡は未対応 |
| D18 | v4で単純な確率攻撃12種、命中済み応答、miss時の無応答と補充、指定命中値を追加 | 旧版の全敵攻撃を1対1の相手1人へ限定。同一対象への連撃・複数敵への解決・特殊効果付き確率攻撃は未対応 |
| D17 | v5で無属性武器への反射・跳ね返し2種、再防御の連鎖、先行通常防具と特殊の順序、まとめた配布を追加。後続profileの病気・DarkCloud付き攻撃も反射後の対象へ解決 | 万能反射・魔法反射・防御奇跡・属性消去、未対応の特殊攻撃、複数人の対象抽選は未対応 |
| D19・D16 | v6で病気と解除8種、重複付与、自然悪化、元turn_ownerの継続効果、Heaven転落、連鎖中の暫定HP0と両者死亡終局を追加 | 自然悪化は旧版増加方式でC02未解消。0生存をdraw/報酬0にするのは学習規約。守護神・蘇生等は未対応 |
| D19・D18 | v7で旧ID43のDarkCloud付与、確率武器の必中・乱数非消費、薬の解除、Heaven転落、Gloryの1枚防御制限を追加 | v7単独ではGloryはfixture専用。制限は旧bot由来で旧サーバー入力受理との差を明記。現行Flashとの名称対応、霧・幻覚は未確定 |
| D13・D14・D18・D19 | v8で光・闇18種を追加。光の代用合成と通常防御、闇の正残余で即死、Gloryの旧ID70通常付与、光闇の確率武器3種 | 通常闇防具は台帳に存在せず、光への闇防御という旧版表の分岐は今回到達しない。衝突C01は未解消。反撃・属性変更・光闇奇跡等は未対応 |
| D11・D13・D18・D19 | v9で攻撃奇跡13種を追加し保持15種へ。MP支払・初使用保持・再使用・配布、通常防具・武器反射禁止、確率7種、Glory200と闇207の解決を追加 | 現行抽出と旧版の威力/MPの差を残す。追加魔法・防御奇跡・忘却・他の状態変化奇跡は未対応。複数敵の同時解決は1対1へ限定 |
| D11・D14・D17 | v10で防御奇跡210/215を追加し保持17種へ。MP付き保持応答、全MAGIC跳ね返し、NE武器遮断、非damageへの通常防具、保持由来の配布予定と長い連鎖を追加 | 防御2種は別配列/maskに分離。元奇跡の使用者と現在sourceを区別。その他の万能/魔法反応・反撃・忘却・追加魔法は未対応。暫定HP0から回復するserver経路のUI可否は未検証 |
| D05・D13 | v11でADD_ATTRIBUTEの武器65/85を追加。普通boostとsetterの順序による全到達結果を火/水/NEの3kindへ圧縮し、6byte Actionを維持。元payload・反応・snapshot来歴も扱う | 追加使用のATK非加算は旧実装の挙動で、現行表示候補との差をC04へ記録。属性消去195・追加魔法・他の順序依存効果は未対応 |
| D13・D14・D17・D18 | v12でREMOVE_ATTRIBUTE防具195を追加。通常防具/20/39/215/210と消去の実選択を検証し、有色・闇・命中済み%武器をNE化して反応。奇跡・状態payload・setterと元命中記録を維持 | 元の神器typeは変更しない。特殊後の未使用195は状態遷移として省略し、旧XMLトレース完全一致は主張しない。その他万能/魔法反応・反撃等は未対応 |
| D14・D16・D17 | v13で反撃190を追加。raw残余×2、同種複数のFIFO、項目ごとの死亡確定、dead source実行/target skip、全キュー後のtick・補充・勝敗、int64威力と公開キューを実装 | 反撃への防御はserver受理を採りbot意図との差をC05へ。未対応7種は除外し、異種反撃の順序・守護神/蘇生/死亡時効果は未対応 |
| D14・D17・D18・D19・D24 | v14で187火75%・191直接Glory・192直接DarkCloud・193自己MP回復を追加し180種へ。16byte行動で反撃のFIFO順を指定、死亡対象のhit結果を観測へ漏らさずRNG進行だけ維持 | 188霧・189幻覚・194所持金反撃は除外。192と光防具の合成でC01の闇防御が到達可能になるが、旧版規則を採用したまま現行公式未照合。C05も未解消 |
| D14・D16・D17 | v15で所持金反撃194を追加し181種へ。raw残余を先にcounter元へ加算し、相手へYEN→MP→HPの順で全額請求する。DEFで請求を減らさず、支払いによる死亡でも再counterは生成しない | source死亡済みでも予約を実行する旧server順序を維持。188霧・189幻覚は秘匿観測と合法手の設計を含め未対応。現行公式との動的照合は未完了 |
| D11・D14・D17 | v16で魔法反射40/171/174/178を追加し185種へ。全MAGICのsource/targetを交換し、prefix消費・状態効果を残して元payloadを返す。通常武器への3防具のDEF、MAGICへの強制反射、保持cast・210との連鎖も検証 | 196万能反射は先頭選択順・counter反射の来歴設計が別途必要。188霧のUI秘匿対象は未確定のため配布しない。[霧の調査](legacy-fog-rules.md)を参照。現行公式との動的照合は未完了 |
| D14・D16・D17 | v17で万能反射196を追加し186種へ。先頭使用とprefix付きの制限を分け、反撃の元所有者・現在source・元命中記録を保持する。194受益者の交替と死亡先返却の取消、キューを増やさない反射連鎖を実装 | 193自己回復は応答なしを維持。意味的行動では特殊後の未使用suffixと旧XML表示を省略。霧・幻覚や現行公式との動的照合は未完了 |
| D05・D12・D17 | v18でMP武器57を追加し187種へ。全MP消費・2倍威力・MP0の0威力応答・通常boost/setterを実装し、WEAPON由来と支払MPを反射後も保持する。counterには来歴を引き継がない | 有料追加MAGIC、MAGIC_FREE、MAGICALの追加魔法後の威力再設定は未対応。[調査](legacy-additional-magic-rules.md)を参照。現poolで最大423、既存上限480の範囲内。現行公式との動的照合は未完了 |
| D14・D17 | v19で魔法遮断8種を追加し195種へ。MAGICへの強制遮断とprefix属性免除、元MP支払・保持を戻さない効果取消、武器/55boost/非MAGIC通常DEFの役割を維持する | COUNTER・MP武器57は遮断対象にしない。weight0群・追加魔法・MP無料化は対象外。現行公式との動的照合は未完了 |
| D05・D11・D13・D14・D24 | v20で197/217を追加し197種へ。単独MAGIC・手札初使用/保持取得・保持反復・保証命中WEAPONへの順序付き追加を実装。57は追加MP支払後の残MPで威力を再設定し、同列取得→再使用、64参照、属性setter、元item typeと来歴を保持する | MAGIC_FREE・214倍化・209全体化は未対応。新gpだけ行動88byte・13maskで、旧19profileのRNG/ABI/保持indexを維持。現行公式との動的照合は未完了 |
| D05・D11・D13・D14・D24 | v21で追加専用DOUBLE_ATK214を加え198種へ。MP6ごとの倍化・初使用保持化・同列取得と反復・NE属性合成・57の残MP再設定を順序どおり処理。raw/counterの上界を拡張し履歴から復元 | MAGIC_FREE・209全体化は未対応。gqは88byte/15mask/924特徴、旧20台帳・ABI・保持index・RNGを固定。現行公式との動的照合は未完了 |
| D05・D11・D13・D14・D24 | v22で追加専用WIDE_ATK209を加え199種へ。MP5・初使用保持化・同列取得と反復・NE再設定・57の残MP再設定を保持。1対1ではalive敵へ正規化し、100%でも命中乱数を消費、DarkCloud時は省略。武器由来と公開wide来歴を分ける | MAGIC_FREE・複数人戦は未対応。gtは88byte/17mask/945特徴、旧21台帳・ABI・保持index・RNGを固定。現行公式との動的照合は未完了 |
| D05・D11・D13・D14・D24 | v23でMAGIC_FREE5種を加え204種へ。最初の後置FREEの直前1品だけを無料化し、MP免除・実消費・保持化・配布予定を区別。防御の途中終了ではFREE suffixを未消費で残し、決定上限までの無料保持連鎖を扱う | 旧serverの受理経路を採用。公式UIでの無料連鎖受理は未照合。giは88byte/23mask/993特徴、旧22台帳・ABI・保持index・RNGを固定。現行公式との動的照合は未完了 |
| D11・D13・D14・D24 | v24でREMOVE_ABILITIES241を加え205種へ。保持コピーの重複数に比例して1個を忘却。属性None、通常防御/195不可、先頭196反射、空ならRNG無しを実装。旧21保持typeのindexを保ち、学習累積と在庫減少を区別 | 原典の取得順リストではなくID昇順の加重rankで抽選するため、分布は一致しても旧Pythonのseed→ID/個体index対応は再現しない。gjは88byte/24mask/1017特徴、旧23台帳・ABI・RNGは固定。現行公式との動的照合は未完了 |
| D13・D14・D24 | v25でTRADE EXCHANGE2を加え206種へ。HP/MP/YENを総和保存で自己再配分し、反応無し・MP費用無し・手札消費と配布予定を処理。配分の差分と自然病気tickを分離し、HP0の自殺も扱う | 各0〜99はcanonicalな入力範囲であり、合計一致しか調べない旧serverの負値/99超を再現しない。公式UIの範囲は未照合。売買は未対応。ggは88byte/25mask/1043特徴、旧24台帳・ABI・RNGは固定 |
| D14・D17・D18・D24 | v26でREVIVE244を加え207種へ。固定seat順でHP0かつdead未確定・not.dead人数2以上なら最初の244を消費してHP10へ。補充無し、病気/harms保持。queue前・owner病気後の境界を区別し、HP0交換と複数回発動を扱う | 既deadの死者を戻す機能ではない。117死亡時攻撃は244併存後もduel発火不能で除外。g26は88byte/25mask/1062特徴、旧25台帳・ABI・RNGは固定。現行公式との動的照合は未完了 |
| D13・D14・D24 | v27でREMOVE_ITEMS240を加え208種へ。解決時targetの手札コピー1枚を一様抽選で除去し、対象の補充/保持化は無し。自己使用では元240を先に消費。属性None・先頭196反射・空なら選択RNG無しを扱う | 削除slot/population/ticketは内部値で観測しない。MORTAR245/PESTLE99とHP吸収は未対応。g27は88byte/26mask/1085特徴、旧26台帳・ABI・RNGは固定。現行公式との動的照合は未完了 |
| D11・D13・D14・D24 | v28でABSORB_HP武器26/42/103と魔法221を加え212種へ。防御後raw残余を現在sourceへ先に99上限回復し、その後targetへ被害。103の75%、反射/自己順序、221保持slot15/FREE/忘却22種を扱い、tick/244回復を分離 | targetの実HP損失に制限する移転ではない。旧main15/17定数・旧21忘却順・台帳は固定し、物理capacity16と新22順を分離。g28は88byte/27mask/1110特徴。旧27 ABI/RNG/212項目を固定、現行公式との動的照合は未完了 |
| D05・D13・D14・D24 | v29でDAMAGE_TO_SELF武器53を加え213種へ。防御後raw残余を現在sourceへ先に、続いてtargetへ与える。自己対象の二段減算、反射後のsource交替、r0診断・244/自然tickとの境界を扱う | 実target HP損失に合わせてsource被害を制限しない。派生counterへ来歴を継承しない。PESTLE99/MORTAR245は除外。g29は88byte/28mask/1135特徴、旧28台帳/保持番号/ABI/RNG/229項目を固定。現行公式との動的照合は未完了 |
| D05・D13・D14・D24 | v30でPESTLE99/MORTAR245を加え215種へ。最初not.dead臼所持者へ光raw999、無臼時は自己を含むランダム生存座席へraw30。単独列・元選択履歴・反射後来歴・通常/特殊応答・反撃を保持し、245の捨て実消費skipと240強制除去を区別 | 旧server受理を基準とし、ログだけで防御禁止とは解釈しない。相手の未選択臼/手札は非公開。g30は88byte/30mask/1175特徴、旧29台帳/保持番号/ABI/RNG/241項目を固定。現行公式との動的照合は未完了 |
| D05・D12・D13・D14・D24 | v31でSELL3を加え216種へ。商品を先に手札から直接消費し、現在targetへYEN→MP→HP請求、容量16内で物理配送、現在sourceへ99上限credit。全216商品の価格を独立source表に固定し、自己取引・先頭196反射・245/244/未使用MAGICも扱う | 満杯配送失敗でも料金/credit/商品消費を戻さない。保持移転や商品使用ではなく、BUY4は後続。g31は88byte/32mask/1221特徴・新SELL kind15、旧30台帳/保持番号/ABI/RNG/257項目を固定。現行公式との動的照合は未完了 |
| D05・D12・D13・D14・D24 | v32でBUY4を加え217種へ。現在target手札1コピーを一様抽選し、empty/self/現金不足は即終了、購入可能な他者品は現在buyerの新purchase phaseでYES/NOを選ぶ。YESは対象削除→seller上限credit→buyer物理配送→現金支払。buyerは4/196消費済みで最大15枚、満杯offer/配送false履歴を拒否。4の価格5だけ新helperへ補う | 母数/slot/ticketは非公開。旧31の216価格表・台帳hash・保持番号を固定。g32は88byte/34mask/1251特徴・kind16〜18、旧31 ABI/RNG/275項目を維持。現行公式との動的照合は未完了 |
| D12・D13・D14・D18・D24 | v33でINCREASE_OR_DECREASE_HP237を加え218種へ。launch時に±10を1回抽選/公開し、単独USE・属性None・196反射後も符号を保持。current targetへの0/99でclipした実変化を244/自然tickと分離し、商品価格1だけfallbackへ加える | raw武器/属性被害やcounterではなく、MYSTERY243等の複合乱数効果は未対応。g33は88byte/35mask/1278特徴で既存kind3を使用、旧32台帳/216価格表/ABI/RNG/297項目を固定。現行公式との動的照合は未完了 |
| D05・D11・D13・D14・D17・D24 | v34でFLICK_MAGIC21/119/121/127/134/140を加え224種へ。MAGIC応答で物理品を消費し、MP/保持化なしに生存対象を選び直す。元payload/命中/保持由来/MP支払を維持しselfは即解決。21のbase/boost5とarmor DEF0〜4、最小構成でのheld gift予算も扱う | 210保持castと物理6種を別eventで数える。共用bounce tape/既存Action・Effectを使い、g34は88byte/36mask/1300特徴。旧33台帳/216価格表/ABI/RNG/315項目を固定。現行公式との動的照合は未完了 |
| D05・D11・D13・D14・D24 | v35でSUNDRY INCREASE_ATK238を加え225種へ。baseATK0/boost10/MP0・保持無しで通常/順序付き武器へ追加し、57有料MAGICのreset/DOUBLE/WIDE/属性/FREEの位置を保持。価格15をfallbackに追加し、消費枚数と最終rawを分離 | canonical防御はDEF無し238を除外。旧server弱いguardはNE/特殊prefixのzeroDEF消費を受理し、応答時手札交換に戦略的意味があり得るが、公式UI未照合で今回は採らない。g35は88byte/37mask/1317特徴、旧34台帳/216価格表/ABI/RNG/333項目を固定 |
| D11・D13・D14・D19・D24 | v36で解除202/211・DarkCloud208・Cold212・Heaven224を加え230種へ。独立5保持、CAST_STATUS19/FREE48〜52、忘却27種、純statusの反応/Heaven転落/解除・prefix→効果→244/tick順を実装。固定commitの全364行と旧225factを新証拠で照合 | 非防御MAGICのzeroDEF応答でMP支払/保持取得する旧weakguard入力は、手札交換/学習の戦略性があり得るが従来canonical同様に除外。g36は88byte/40mask/1345特徴、旧35台帳/21・22保持順/216価格表/ABI/RNG/358項目を固定。現行公式との動的照合は未完了 |
| D05・D13・D14・D16・D24 | v37でDYING_ATTACK117を加え231種へ。通常は光ATK1・75%・敵正規化・追加不可。死亡時威力30も台帳へ保持し、dead設定後のnot.dead人数2以上という条件からduel発火不能を固定commitのroom/turnで確認。244救済はdead設定前continueなので結論は変わらない | 死亡時queueを実装した主張ではなく、3人以上戦は未対応。g37は88byte/41mask/1368特徴で新Action/Effect/保持/tapeなし。旧36台帳/216価格表/ABI/RNG/387項目を固定。併せてv36の208手札属性観測をNEから闇へ修正（効果payloadとゲーム/乱数/checksumは不変） |
| D24〜D27 | 意味的Action、128/gl・164/gm・186/ge・210/gc・236/gr・258/gd・283/gh・301/gs・334/go・356/gb・375/ga・393/gn・515/gk・661/gx・679/gy・696/gz・713/gu・732/gv・750/gw・903/gp・924/gq・945/gt・993/gi・1017/gj・1043/gg・1062/g26・1085/g27・1110/g28・1135/g29・1175/g30・1221/g31・1251/g32・1278/g33・1300/g34・1317/g35・1345/g36・1368/g37のC ABI/Python。v10で応答kind6/8番目mask、v11で属性kind7〜9/9番目mask、v12で消去の10番目mask、v13で反撃の11番目mask、v14で順序count+4bit列を追加。gl〜gkは6byteを維持し、gx/gy/gz/gu/gv/gwは順序付き16byte、gpは保持参照64件付き88byte・13mask、gqは同じ88byte・15maskと保持214参照18、gtは17maskと保持209参照19、giは23maskと65番目tail/無料cast・防御列、gjは24maskと忘却ticket fixture、ggは25maskと資源配分kind14、g26は同mask/actionと自動蘇生、g27は26maskと手札除去ticket fixture、g28は27maskと221の末尾保持bit15、g29は28maskと自己被害来歴、g30は30maskと杵/臼の選択履歴、g31は32maskと売却kind15・商品価格/配送履歴、g32は34maskと購入kind16〜18・確認phase、g33は35maskと公開HP符号/実変化、g34は36maskと物理MAGIC跳ね返し、g35は37maskと物理SUNDRY boost、g36は40maskとstatus保持5種/cast19、g37は41maskと117通常武器/命中履歴。共通Snapshot37、旧ABI幅は固定。snapshotと各抽選prefix、出典由来の局面テスト | 対局途中Envの永続checkpoint、全効果の行動表現、旧版Pythonとの自動差分実行、現行公式との照合は未完了 |
| D16〜D23・D28 | 各限定profileで終局と学習用打ち切りを扱い、病気・反撃による0生存終局も区別 | 旧版の未対応効果（霧/幻覚等のharms・守護神・終末）と複数人戦、現行公式の蘇生との差異は未照合。旧244自動蘇生はv26で対応済み。衝突C01/C02も未解消 |

## 比較資料と確度

| 出典 | 内容 | 扱い |
| --- | --- | --- |
| [B: Bible抽出記録][B] | 第三者が公式画面から抽出した2026-09-20の神器・参照説明 | 当時の公式表示の記録。現在の画面とは未照合。説明文だけで処理順までは確定しない |
| [現行公式JSON](https://godfield.net/i18n/ja.json) / [現行client](https://godfield.net/main.dart.js) | 2026-11-01取得の296モデル・教典・計算・表示・選択処理 | hashを固定して台帳、pure関数、freshな部分viewを照合。server engineや全画面・時系列の一致は主張しない |
| [S: 調査記録][S] | 同プロジェクトによる公式クライアントの観察 | 観察した場面の証拠。全条件で成り立つ規則とは限らない |
| [F0: godfield-flash README][F0] | 旧Flash版の非公式エミュレーターの説明 | 作者が神器分布と霧の不完全さを明記。動作の参考にするが正解と同一視しない |
| [F1: player.py][F1] | 初期値、所持品、奇跡、状態変化 | 旧版実装の根拠 |
| [F2: room.py][F2] | 手番順、配布、死亡処理、効果の進行 | 同上 |
| [F3: turn.py][F3] | 行動、連鎖、反応の解決 | 同上 |
| [F4: attackData.py][F4] | 攻撃の構造と組み合わせ検証 | 同上 |
| [F5: item.py][F5] | 神器データ構造、属性判定、抽選 | 同上 |

Bに記録されたクライアントSHA-256:
`764a50524e4b6b3f510415da7128abd8ad99dcb87b45b8572d98ddd96889cabd`。
これはB内では抽出者の記録値である。今回は別途公式URLから取得して同じSHA-256を確認した。
ただしBの観測内容全体や、別配信JSONの版まで同一と証明する値ではない。
F系の参照リンクには可変の `master` が残る。v36では全CSV、v37ではroom.py/turn.pyを
固定commit `fcc83740a3d3cd9968e6e5cd2b7ed781f53abe80` から取得し、新manifestへSHA256を記録した。
過去manifestの当時の取得記録は変更せず、未取得の処理まで固定内容と照合済みとは扱わない。

「大きな仕様改定はない」という前提でF系を実装調査の中心に使う。
ただし、B/Sとの衝突やF0の既知不具合は個別に残し、変更なしと推測して埋めない。

## 表の読み方

- **相違**: B/Sの記録と現在の表現・処理が異なる。
- **旧版差**: F系との違いをコードから確認。現行版にそのまま採用できるかは未確定。
- **不足**: 資料に存在する機能を現在の実装が表現できない。
- **要確認**: 資料不足、資料間の衝突、処理順の未検証がある。
- **学習側**: シミュレーター・学習器の設計。公式ルールの再現度とは別に管理する。
- **対象外**: 最初の1対1対応に含めず、後続の範囲として管理する。

P0は公式対応用コアの土台を阻む項目、P1は1対1のルールを埋める項目、
P2は初期対象外のモードなど。P2を省いた環境を「全仕様対応」とは呼ばない。
優先順位は実装順であって、P1の未対応効果を無視してよいという意味ではない。

## 基本状態・合法手・進行

| ID | 優先 | 項目・判定 | 現在のC++ | 比較資料との差・未確定点 | 対応と受入条件 |
| --- | --- | --- | --- | --- | --- |
| D01 | P0 | 神器台帳 / 不足 | `cards`の独自10種。公式IDなし | Bには291の表示レコード。F系IDとの同一性は未確認 [B] [F5] | 種別・属性・価格・効果・出典を持つ台帳を作る。表示レコード数と抽選対象数を区別し、旧IDの流用を検証する |
| D02 | P0 | 初期資源 / 旧版差 | HP40、MP20。所持金なし | F1の`reset`はHP40、MP10、所持金20。現行値は別途確認 [F1] | モード別初期値を固定し、開始直後の公式画面と照合する |
| D03 | P0 | 手札枚数・上限 / 要確認 | 常に9スロットへ補充 | F1は初期配布数のカウンター10、手札上限16。一方Sの観測は手札9枚 [F1] [S] | 開始配布・初回行動・上限を別々に検証。容量と開始枚数を別定数にする。衝突C03を解消する |
| D04 | P0 | 神器分布 / 相違 | 10種を等確率・復元抽出 | Bの配布率は神器別に異なる。F0は自実装の分布に不確かさを明記 [B] [F0] | 現行の配布率・対象プール・抽出条件を整理。F系の重みを無検証で採用しない。確定後に分布検査する |
| D05 | P0 | 攻撃の組み合わせ / 旧版差 | 武器の任意部分集合を合算 | F4は先頭と追加分を区別して検証。Bにも通常ATKと追加ATKの区別がある [F4] [B] | 基底の神器と追加効果を分離。合法・非合法の組み合わせ、順番の影響をケースで検証する |
| D06 | P1 | 攻防兼用 / 不足 | 種類は武器か防具の片方 | F5は武器にも防御値を持たせる [F5] | 使用可能な役割を種別とは別に表現。攻撃・防御それぞれの合法手と消費を検証する |
| D07 | P0 | 捨てる制限 / 相違 | 全ての非空スロットを捨てられる | Bの参照説明には武器等の捨て禁止がある [B] | 神器別・局面別の可否を合法手生成に反映。禁止対象が候補に出ないことを確認する |
| D08 | P0 | パス / 旧版差 | 通常フェーズでは無条件に可能 | F3の`DO_NOTHING`は武器を所持しないことを要求し、配布数も増やす [F3] | 現行のパス条件と配布を照合。パスが可能な手札・不可能な手札を用意する |
| D09 | P0 | 通常手番の順序 / 旧版差 | 最初だけ乱数。以後は相手、防御後は防御側 | F2は巡回用の順序リストを持ち、周期の境界で並べ替える [F2] | 通常手番の順番と反応の順番を分離。防御者を無条件に次の通常行動者にしない。2人の周期境界も確認する |
| D10 | P0 | 消費・補充 / 要確認 | 攻撃の解決後に双方の空欄を埋める。回復・捨ては即補充 | F系は空欄補充ではなく配布数を管理し、手番終了処理で配る [F1] [F2] [F3] | 行動ごとの配布増分、反応中の消費、配布順、手札満杯時を追跡する。死亡・連鎖中に先に配らないことも照合する |
| D11 | P0 | 奇跡の保持・再使用 / 旧版差 | 回復魔法も毎回手札から消える | F1では手札から使う奇跡を`magics`へ移し、再使用は別処理 [F1] | 手札と保持した奇跡を分離。初回取得・再使用・MP消費・忘却を検証する |
| D12 | P1 | 回復・数値 / 相違 | 独自のHP回復、MP回復、MP6でHP+10 | BのSpringはHP+10、MP7。独自カードID10を公式カードとみなせない [B] | 回復を公式IDと数値へ置換。対象指定、上限、コスト、手番終了条件も照合する |
| D13 | P1 | 属性 / 不足 | 属性フィールドなし | Bの属性説明とF5に判定があるが、一部が衝突 [B] [F5] | 属性の合成と防御可否を実装。光への防御は衝突C01を解消してから確定する |
| D14 | P0 | 防御の条件・消費 / 要確認 | 防具なら何枚でも合算し全て消費 | 現行の一般則を未確定。F3の`defenderCommand`が調査対象 [F3] | 使用順、必要量を超えた防具、兼用武器、特殊防御、状態による制限を分けて確認する |
| D15 | P1 | 対象・取引 / 不足 | 攻撃は相手、回復は自分。所持金なし | F3には対象・売買応答・資源交換の分岐がある [F3] | 対象、追加パラメーター、承諾などの反応を行動に追加。売買の支払・移動・不成立を検証する |
| D16 | P0 | 死亡確定 / 旧版差 | ダメージ後HP0なら即終了 | F2は死亡処理中に蘇生・死亡時攻撃を扱う [F2] | HP0と脱落確定を別状態にする。蘇生、同時死亡、死亡時効果を処理してから勝者を決める |

## 特殊効果・モード

| ID | 優先 | 項目・判定 | 現在のC++ | 比較資料との差・未確定点 | 対応と受入条件 |
| --- | --- | --- | --- | --- | --- |
| D17 | P1 | 反射・跳ね返し / 不足 | 未解決状態は攻撃力1個だけ | F3は攻撃キューと継続する解決処理を持つ [F3] | 元の通常行動者、現在の攻撃元・対象、反応待ちを分離。連鎖後の手番と終了条件を検証する |
| D18 | P1 | 確率・連続攻撃 / 不足 | 全攻撃が確定命中・1回 | F3は命中判定と複数の攻撃を処理する [F3] | 抽選結果を注入して命中・失敗・連撃途中の死亡を再現する。乱数の呼び出し順も記録する |
| D19 | P1 | 状態異常・悪化 / 不足 | 状態異常の状態なし | Bに説明があり、F1/F2に進行処理がある。ただし悪化確率に衝突 [B] [F1] [F2] | 発生・重複・悪化・解除・処理タイミングを分ける。確率はC02を解消してから採用する |
| D20 | P1 | 霧・見かけの手札 / 不足 | 常に自分の真の手札と両者の資源を観測 | F1は真の神器と見かけを区別する。F0は霧が不完全と記載 [F1] [F0] | 内部状態とプレイヤーの知識を分離。状態異常中の観測・合法手マスクが隠れた真値を漏らさないよう確認する |
| D21 | P1 | 守護神 / 不足 | 所有者・HP・行動状態なし | F1/F2/F3に守護神への参照と進行処理がある [F1] [F2] [F3] | 個体・所有者・HP・発動を持つ。召喚・消滅・被ダメージ・発動順・特殊個体は個別照合する |
| D22 | P2 | 終末・特殊現象 / 不足 | 状態なし | Bには終末に関する説明と現象の記録がある [B] | 通常1対1の対応範囲に終末を含めるかを明記。含める際は発生条件・ルール切替を検証する |
| D23 | P2 | 複数人・チーム / 対象外 | 2人固定。相手は`1-actor` | Sは最大9人に言及。F2はチームと生存者を扱う [S] [F2] | 初期目標とは別段階。人数、対象、生存者巡回、チーム勝敗、観測・報酬を拡張する |

## 学習接口・照合基盤

ここからは互換性を支える設計上の差分であり、公式ゲームに同じAPIや報酬があるという意味ではない。

| ID | 優先 | 項目・判定 | 現在のC++ | 必要な変更・確認 | 受入条件 |
| --- | --- | --- | --- | --- | --- |
| D24 | P0 | 行動表現 / 学習側 | 9bitの選択と捨てるを合わせた521種類。順序・対象なし | 基底、追加効果、対象、保持奇跡、売買応答等を表せる行動へ。D03/D05/D11/D15に依存 | 同じ選択集合でも順番等で意味が違うケースを区別。新ABIと旧チェックポイントの非互換を明示する |
| D25 | P1 | 観測 / 学習側 | 73特徴。資源・5種別・威力等 | 神器識別、保持奇跡、所持金、状態異常、解決中の文脈、既知の履歴を検討。D20の秘匿条件を守る | 同じ公開情報から隠れた内部状態が特徴量・マスク経由で漏れない。観測schemaを版管理する |
| D26 | P0 | 乱数の照合 / 学習側 | SplitMix64のseedでローカル再現 | 公式サーバーと同じseedや乱数列とは仮定しない。抽選結果を指定できる検証用入口を設ける | 同じ事前状態・意味的行動・指定抽選結果でC++の結果を比較できる。分布の検証は別に行う |
| D27 | P0 | テストの根拠 / 学習側 | 簡略ルールの遷移、1万試合、不正手、ABI等 | 既存テストは公式との一致を証明しない。F系の局面と公式で観測した局面を、出典付きfixtureへ追加 | 各差分に局面・操作・観測できた結果・未観測部分を記録。F系との一致と公式との一致を別に報告する |
| D28 | P2 | 打ち切り・報酬 / 学習側 | 512意思決定で打ち切り。終局±1 | 512手を公式の終了条件として扱わない。D16/D23の勝敗を解決した上で学習用報酬を付ける | 自然終局と学習用打ち切りを区別。防御後にactorが同じ場合も座席別に報酬を対応させる |

## 資料間の衝突

旧版を調べたことで判明した衝突。大きな仕様改定がなくても、非公式実装の誤り・表示説明の省略・
観察時点やモードの違いがあり得る。原因は現時点で断定しない。

| ID | 記録A | 記録B | 解消のための確認 |
| --- | --- | --- | --- |
| C01 | Bの属性説明は光攻撃を属性では防御できないとしている [B] | F5の`checkDefense`では光攻撃に闇防御を許す [F5] | 実際に用意できる神器と特殊効果を区別して公式局面を確認。v14では192闇counterと光DEF158を混ぜると光攻撃へ闇防御できる旧規則を採用しており、現行一致は未確認。[追加調査](legacy-counter-extensions-rules.md)を参照 |
| C02 | Bの病気説明は自然悪化の確率を5%としている [B] | F2は`worseChance`を用い、悪化しなければ増加。F1では初期値0 [F1] [F2] | 現行表示と効果の発生履歴を照合。旧版の増加方式を固定5%と同じ扱いにしない |
| C03 | Sには開始時の手札9枚という観測がある [S] | F1の初期配布数は10、F2は開始時に配布を処理 [F1] [F2] | モード・最初の配布・最初の行動・観察タイミングを合わせる。手札上限16という別の条件とは分ける |
| C04 | Bの対応候補Wand of Ignition/Mystic Waterは+ATK2/5と火/水への付替を表示 [B] | 旧65/85は単独ATK2/5だが、F3のADD_ATTRIBUTE追加処理は威力を加算しない [F3] | ID対応自体を確定せず、現行の単独/追加時の威力を動的照合。旧版profileでは非加算を維持。[調査と検算](legacy-attribute-change-rules.md)を参照 |
| C05 | [旧bot](https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/bot.py)はcounter攻撃へ防御を返さない | F3のdefenderCommandはcounterにも普通防具・195・再counterを受理する [F3] | 現行公式の反撃応答を動的照合。v13/v14は旧server受理を選び、bot意図や公式一致を確定しない。[調査](legacy-counter-rules.md)を参照 |

### C01〜C05の今回の一次確認

上表は旧資料間の衝突の記録として保持する。今回の対応状態は次のとおり。

| 衝突 | 今回直接確認・実装した範囲 | 残る確認 |
| --- | --- | --- |
| C01 | 公式教典とclientの属性防御predicateは光攻撃へfalse。`official::attribute_defense_allowed`と実JS oracleも一致 | 特殊反応/混合防御を含む権威的serverの受理・解決。legacyの光→闇防御は変更しない |
| C02 | 公式配信の教典説明は自然悪化5%と記載 | serverでのtick対象・抽選の位置/端点・再付与・死亡順。今回のpure kernelは病気turnを進めない |
| C03 | 公式clientの初期表示は9枠、資源表示の初期値はHP40/MP10/CP20。freshな自己手札描画は使用済みを含む18個までを今回の対応域にする | 初回実配布数、server側の保持/未使用物理手札上限と補充の動的契約。表示の18をserver容量とは断定しない |
| C04 | 現行66/83の属性ワンドはisPlusAtkかつATK2/5。実clientの順序付き合成でも加算し、official kernelが一致 | serverの消費/HP遷移との照合。旧65/85の非加算仕様はlegacyへ保存 |
| C05 | 現行clientのvisible応答predicateについてattack分類・反応・追加可否を実JSで照合 | counter由来分類の完全な意味、server受理と多段queue/死亡/補充順 |

## 実装箇所との対応

| ローカル箇所 | 主な差分 |
| --- | --- |
| [official_state.hpp](../include/godfield/official_state.hpp)、[official_state.cpp](../src/official_state.cpp) | 現行公式decoded frameとfresh部分view、本人の表示モデル/token選択照会。D20/D23/D25の表示側を部分対応し、対局進行は未実装 |
| [official_c_api.h](../include/godfield/official_c_api.h)、[official_state_c_api.h](../include/godfield/official_state_c_api.h)、[official_state.py](../python/official_state.py) | 現行公式の台帳・計算APIと入力stateのimport/view照会。旧版Envとは別の部分対応接口 |
| [env.hpp: Card / cards / Player / State](../include/godfield/env.hpp) | D01〜D03、D06、D11、D15〜D23。神器・プレイヤー・解決中状態を拡張 |
| [env.cpp: reset / refill](../src/env.cpp) | D02〜D04、D10。初期化と神器獲得を区別 |
| [env.cpp: legal / legal_actions](../src/env.cpp) | D05〜D08、D13〜D15、D20、D24。合法手を仕様から生成 |
| [env.cpp: step / next_turn](../src/env.cpp) | D09〜D19、D21〜D22。通常手番と効果解決を分離 |
| [env.cpp: observe](../src/env.cpp) | D20、D25。見える情報を別に構成 |
| [c_api.h](../include/godfield/c_api.h)、[c_api.cpp](../src/c_api.cpp)、[Pythonラッパー](../python/godfield_env.py) | D24〜D26、D28。次期ABI・schemaを定義 |
| [env_test.cpp](../tests/env_test.cpp) | D27。独自ルール用テストを公式照合テストと区別 |

## 着手順と完了条件

現行対応の次の本体作業は、turn/effect queue・消費/補充/死亡順を持つ状態遷移実装である。
その上で9人/チーム対局、守護神、終末/悪魔/超常現象と勝敗へ拡大する。
9人分のstate/viewがある段階を9人playableの完了とせず、学習器は現在の対象に追加しない。
以下の番号付き手順は、差分表作成時の計画記録として読む。

1. **資料の固定と台帳**: Bの元データ、F系の参照commit、意味的な神器対応表を固定する。現行未照合の項目を残す。
2. **土台の置換**: D02〜D11・D16・D24・D26。初期状態、容量、順番、配布、合法手、保持奇跡、死亡待ちを扱える状態機械を作る。
3. **1対1の効果を拡張**: D12〜D21。衝突C01〜C05を解消しながら、効果ごとの照合fixtureを追加する。
4. **学習へ接続**: D25・D27・D28。未対応効果を黙って通常攻撃として処理しない。制限した神器プールを使う場合は独自rulesetとして記録する。
5. **対象範囲を拡大**: D22〜D23。終末を除くなら対象名にも明示し、複数人戦は別の完了判定を持つ。

各項目の完了には、根拠、実装、対応する照合ケースの3点が必要。
差分表を最初に作成した時点ではゲームの挙動を変更していない。
その後の実装範囲は冒頭の進捗表に記載し、公式対応済みとは区別する。

[B]: https://github.com/BrandenXia/godfield-bot/blob/main/data/snapshots/2026-09-20/bible.json
[S]: https://github.com/BrandenXia/godfield-bot/blob/main/docs/research/2026-09-06-godfield.md
[F0]: https://github.com/Igoorx/godfield-flash#known-issues
[F1]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py
[F2]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py
[F3]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py
[F4]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py
[F5]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/item.py
