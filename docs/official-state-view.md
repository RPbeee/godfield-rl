# 公式スナップショットからの表示投影

対象は公式 [main.dart.js](https://godfield.net/main.dart.js) のSHA-256 `764a50524e4b6b3f510415da7128abd8ad99dcb87b45b8572d98ddd96889cabd`。2026-11-01取得の [ja.json](https://godfield.net/i18n/ja.json) / [en.json](https://godfield.net/i18n/en.json) と合わせて調査した。取得物の全ハッシュは[一次調査](official-current-rules.md)を参照する。以下の行番号はこの固定JSに限る。

前半の行番号と出典は初回v1調査の固定記録である。v48の先頭攻撃APIは別世代v2の固定clientで追加照合した（末尾のv48節を参照）。

ここでいうviewは、既存のイベント記憶や過去に見た値を補完せず、新規に状態を構成したときの表示・閲覧可能情報である。サーバーのゲーム遷移や乱数生成の再現ではない。wireに存在する値が、そのまま本人へ表示されるとは限らない。

## wireの形と本人の特定

`tk`（3752–3786行）はゲーム全体の `players`、`gf`、`tiebreakGF`、`attackTurnPlayerId`、`attacks`、`isOver`、`events`、`updateCount` を読む。表示再構成は `ft.fs`（12334–12376行）。増分イベントを使う経路もあるが、本契約ではfreshな再構成経路だけを扱う。

`tL`（4021–4067行）が読むプレイヤーの値は、数値 `id`、文字列またはnullの `userId`、`name`、`alias`、`team`、`isBot`、HP/MP/CP、災い、守護神、`confusionCount`、`dead`、`items`、rating/ratingGain。`pQ`（4206–4209行）はteamを0〜4の列挙型に変換する。

実クライアントは `h7.J`（13140–13143行）で現在の認証userIdに一致するプレイヤーを本人として探す。数値playerIdとuserIdは別の識別子である。オフラインAPIがviewerを数値playerIdで受け取る場合は、userIdから本人を解決済みの明示パラメーターとして扱い、wireの任意userIdを表示データへ混入させない。一致する本人が無ければ観戦者の経路になる。

## 敵関係と霧の範囲

`c5.ce`（13231–13233行）は次の敵関係を使う。

- 個人戦team0では、自分以外を敵とする。
- チーム戦では、自分と異なるteamを敵とする。

`h7.aG` は `dead=false` の人だけを集め、`dR` はそこから敵を絞る（13145–13151行）。HP0とdeadを同一視する処理ではない。

freshな `ft.fs` の霧条件（12352–12357行）は次の全てが成立するときである。

1. viewer本人が存在する。
2. viewerが霧を持つ。
3. viewerのdeadがfalse。
4. ゲームのisOverがfalse。

この条件下で、生存している敵の通常表示コンテナにだけ `em(true)` を適用する。味方、自分、既にdeadの敵には適用しない。viewer本人がdead、観戦者、終局後であれば、このfresh経路は霧を掛けない。霧の災い自体を削除するわけではない。

`em`（13257–13273行）は情報コンテナ `z` を外し、半透明の霧で覆う。`c5` のフィールド割当（4154–4176行）、`ox`（4078–4098行）、`iT`（13173–13211行）、`dA`、`sd1` をたどると、通常コンテナ内にある名前・チーム記号・HP/MP/CP・災い・守護神・混乱の表示が対象になる。座席そのものの存在を削除する処理ではない。

ただし、これを「霧の間はその人について全てが非公開」と拡張してはならない。以下の使用済み奇跡パネルや、公開されている攻撃表示は別経路である。

## 通常パネルと使用済み奇跡パネルは別

`c5.fs`（13221–13229行）は、各プレイヤーの `items` のうち `used=true` のものだけを別のパネル `db` へ追加する。表示するモデルは `fakeModelId>0` なら偽モデル、そうでなければ真のmodelId。`j8.eQ`（13383行以降）は表示モデルID順に並べる。同じモデルの複数使用済みコピーは重複のまま表示される。

`A.nh`（13475–13500行）の「使用済み奇跡」ボタンは、全プレイヤーの通常パネル `y` を隠して、`db` をその人の外側コンテナへ直接追加する。この操作に霧の有無・本人の生死によるガードはない。`db` は霧の `z` の子ではない。

したがって、霧中の生存敵についても、その別パネルにある表示名・チーム記号・使用済み奇跡の表示モデルは閲覧可能な情報である。通常パネルの `pane_hidden` と、あらゆるUI操作を含めた情報の公開範囲を区別する必要がある。名前やチームをview全体から消すだけでは、現クライアントより情報を落とすことになる。

使用済み奇跡の小アイコンは `A.qd` / `A.bc`（4111行付近、13446–13457行）が表示モデルだけから作る。そのアイコンに対応する元のinstance ID、真のモデル、偽表示の有無の専用印は、この別パネルに表示されない。

## 名前・別名の例外

`h7.is`（13065–13100行）は、viewer本人が存在すれば本人のaliasを表示用にnullへ置き換え、他者のaliasが非nullなら通常パネルと使用済みパネルの両方の名前をaliasへ置き換える。本人がいない観戦者経路ではこの置換ループを実行しない。

| 閲覧経路 | 通常/使用済みパネルの名前 |
| --- | --- |
| 参加者が自分を見る | name |
| 参加者が他者を見る | aliasが非nullならalias、それ以外はname |
| 本人に一致しない観戦者 | name |

空文字列aliasも非nullとして扱われる。`qj`（3687行付近）の攻撃パネルの表示名も、置換後のaliasを優先する。

終局かつduelモードでは `ft.fs` が `h7.lS`（13105行以降）を呼び、ratingパネルを別に構成する。こちらはrawプレイヤーを `ox` へ渡すのでnameを表示し、rating/gainも表示する。ratingボタン `A.lE`（13508行以降）で閲覧可能である。したがって、終局duelでは別名と別に元のnameへアクセスできる。これはモード依存の例外であり、modeを持たない投影に対して「isOverなら常に全本名公開」と推測してはならない。

## 自分の手札と他人の手札

`kY`（3855–3865行）はカードインスタンスを `id`、`modelId`、`fakeModelId`、`used` として読む。modelIdは公式台帳IDであり、固有idや旧版IDとは異なる。

freshな `ft.fs` は本人が存在するときに限って、その本人の `items` を手札UIの `fG.ir` へ渡す（12357行、`kw` 12476行、`ir` 12641行）。他者の未使用手札はこの経路では描画しない。本人に一致しない観戦者に、自動的に誰かの手札を与える処理もない。isOverでも全員の未使用手札を公開するfresh処理は見つからない。

他人のwireにitemsが含まれていても、それをviewの未使用手札・件数・真偽情報へ転記する根拠にはならない。`tL` に他人の未使用手札枚数を専用表示するフィールドもない。表示から手札枚数を独自に推定して補完しない。

自分の手札ではコマンド送信に固有instance IDが必要であり、`submit-command` の `itemIds` は選んだinstanceのIDから作る（9651–9679行）。UI上の台帳モデルをコマンドの物理IDへ取り違えない。

`fG.iv` / `hs`（12750–12782行）は見かけモデル等で表示を並べ替え、使用済みを後ろに配置する。wireの配列順、見かけモデル順、固定16slotという旧環境の順序は同じではない。instanceベースの選択検証は配列添字やモデルIDの一意性ではなく、実instance IDを用いる。

## 夢：見かけ、偽表示の印、真のモデルのtooltip

`aC.hn`（12859–12881行）は真のモデルを `e`、fakeModelIdが非0なら偽モデルを `f` として保持する。`gL`（12856–12858行）は常に `f` を優先する。通常の絵、追加可能性の判定、MP計算、選択中の説明はこの見かけモデルを使う。

偽モデルがある場合、`hn` は偽モデルの絵に加えて `items/fake.webp` の半透明オーバーレイを付ける。`od`（5261–5263行）はその画像の透明度を0.5にしている。したがって **自分の手札のfake存在フラグは可視**であり、真のモデルIDとは別の情報である。fakeModelIdが真のmodelIdと偶然同じでも、正のfakeModelIdなら同じ分岐になる。

`A.l_`（12981–12989行）のtooltipは例外を持つ。本人がdead、または攻撃画面のendingフラグ `fq.as` がtrueのときは真のモデル `e` を示し、それ以外は見かけモデルを示す。`fq.kI`（11941行）はending表示でこのフラグをtrueにする。本人の死亡・終局によって真のモデルを閲覧できるが、絵そのものがこのtooltip分岐で自動的に真の絵へ交換されるわけではない。

この例外は自己手札のtooltipについてのものであり、他人の全真手札を公開する規則ではない。他者の使用済み奇跡パネルは、その時点でも `fakeModelId` を優先して構成する。

投影には、少なくとも `display_model`、`has_fake`、`true_model_if_revealed`、`used`、選択対象のinstance IDを別に持たせる。夢の災いが無いからfakeが存在しない、usedだから真のモデルである、といった補完はしない。fresh wireのインスタンス情報をそのまま表示規則へ通す。

## active attackの公開フィールドと表示用分類

`p3`（3653–3680行）は以下を読む。

| wire key | 意味 | 省略時のデコード |
| --- | --- | --- |
| playerId | 現在の攻撃表示の発信者 | 0 |
| guardian | 表示する守護神の種類 | null |
| itemModelIds | 順序付きの公開モデル列 | 空列 |
| atk | 動的な基底ATK用の表示値 | 0 |
| hitRate | 動的な命中率用の表示値 | 0 |
| mp / cp | 表示用MP/CP値 | 0 |
| targetPlayerId | 既に設定された表示対象 | 0 |
| reversed | 攻撃側の表示方向 | false |
| buyingItemModelId | 購入待ちの表示商品 | 0 |

`tk` はattacksリストの先頭だけをactive attackとして渡す。残りのキュー全体を学習viewへ公開する根拠にはならない。`fv`（12115–12143行）はモデル列からカード表示を構成し、MP由来ATK、死に際攻撃、反撃MP/CP等に応じて値を渡す。これはfreshに届いている情報の表示であり、未来のヒット抽選や秘密のキューを計算しない。

`gf3`（12293–12295行）の `incoming_is_attack` 相当は「先頭モデルのatkが正、または先頭categoryがweapons、または動的atkが正」。命中率の有無そのものではない。`categoryWeapons` という能力の場合に、反応判定へ渡す分類をweaponsへ読み替える処理もある（`ev` 9483–9488行）。守護神由来というだけで一律MAGICやWEAPONへ置き換えない。

instance選択のappend可否は、真のモデルではなく表示モデルとused状態を `VisibleSelection` へ渡す。現在MPも本人の表示値から検査する。追加可能性、全コマンドの有効性、サーバーの受理は別契約であり、古いエンジンで真値に基づき拒否してからappend可否を返してはならない。

## 終局の表示上の勝者

`h7.lV`（13153–13162行）は最初にdead=falseの人を集める。0人なら勝者は空。先頭が個人戦team0ならその1人、先頭がチーム所属なら、deadの人を含めて同じチームの全プレイヤーを返す。

この関数は終局条件や同時死亡順を決めていない。2人以上の個人戦生存者を持つ任意フレームでも先頭1人を返す関数であり、そのことから当該フレームを合法な終局と断定できない。view APIではisOverを入力の事実として維持し、勝者の表示関数と権威的勝敗判定を区別する。

## fresh viewの最低契約

| 情報 | 通常参加者 | 霧の生存敵 | dead viewer / 観戦者 / 終局 |
| --- | --- | --- | --- |
| 座席の存在 | 可視 | 可視 | 可視 |
| 通常パネルの資源・災い・守護神・混乱 | 可視 | fresh表示では不可視 | 霧による覆いなし |
| 表示名とteam | 別名規則で可視 | 通常paneは覆うが使用済みpaneで閲覧可 | 別名/モード規則に従う |
| 他者の使用済み奇跡モデル | fake優先の小アイコン | 別paneで同じく閲覧可 | 同じ規則 |
| 他者の未使用手札・枚数 | この経路では表示しない | 表示しない | 自動公開しない |
| 自分の手札 | 表示モデル・fake印・used | 自分の霧でも手札を覆わない | dead/終局なら真モデルtooltip可、観戦者の本人手札は無し |
| active attack | 受信済み先頭項目を表示 | 別の公開攻撃表示を保持 | 終局後は通常のactive表示再構成に進まない |

過去イベントからの記憶、霧の途中の演出による一時的な露出、フレームに無い手札・未解決乱数・残りの攻撃キュー、サーバー側の夢の効果処理はこの契約の外である。欠落値を0や真値で埋める代わりに、非公開・欠落・表示モデルを区別する。

## 実装された部分投影API

[official_state.hpp](../include/godfield/official_state.hpp) と [official_state.cpp](../src/official_state.cpp) は、上記のうち **状態欄・自己手札・使用済み奇跡一覧** を扱う。すべての画面、時系列イベント、権威的なゲーム状態遷移を再現するAPIではない。

- `SnapshotState` はデコード済みのプレイヤー列、GF、任意の終末GF、終局フラグ、transport更新番号を保持する。`validate_state` は最大9人、JSの安全整数域、プレイヤーIDと本人内のinstance tokenの重複、台帳IDと列挙値などを検査する。これはサーバー側の到達可能状態を証明する関数ではない。
- `resolve_viewer` はuserIdから最初に一致する本人を数値playerIdへ解決する。`project_view(state, nullopt)` は観戦者、既存playerIdはその本人の投影。存在しない明示playerIdは観戦者に黙って変換せず拒否する。
- `SnapshotView` / `view_json` のschemaは `official-fresh-view-v1`。userId、raw alias、transportのupdate_countを含まない。表示名、team、dead、fogged、任意のstatus、公開used_models、自己hand等を返す。raw nameを表示名とは別の抜け道として渡さない。
- `status=null` は霧でその通常欄が覆われたことを示す。HP0や空の災いと同義でない。公開済みの名前・team・used_modelsは別欄で維持する。
- 災いはwireのSet相当で最初に出た重複だけを残し、その後に現れた異なる病気が前の病気を置き換える。例えば `cold,fever,cold` は重複coldを先に除くので最終表示はfever。出力は表示アイコンの集合として列挙値順に正規化し、挿入履歴を観測へ出さない。
- 混乱は残り回数を返さず `confused` の真偽だけ。死亡済みならfalseとする。`hP`（13348–13355行）が混乱の枠を消すためであり、rawのconfusionCountを公開する規則ではない。
- 自己handはusedでないものを先、次にfaceのモデルID順で安定ソートする。同じused/faceなら入力の相対順を保つ。`face` は常に見かけ、`reference` はtooltipで閲覧できるモデル、`disguised` は偽オーバーレイの有無。本人がdeadまたは終局ならreferenceを真のモデルへ変えるが、faceは変更しない。
- 他者used_modelsは見かけモデルIDのソート済み列であり、重複数を保持する。他者の固有instance token・未使用手札枚数・真のモデル・偽表示マーカーは返さない。

`alive_players` / `enemy_players` / `ending_display_candidates` は固定スナップショット内の表示集合の補助関数であり、ターン進行やサーバーの勝敗判定ではない。HPではなくdeadフラグを用いる。

### instance選択の照会

`can_append_hand` は本人のinstance token列と次のtokenを受け取り、モデルを見かけへ変換して公開クライアントのappend判定を呼ぶ。存在しないtoken、同じtokenの再選択、本人の死亡、終局を拒否する。同じモデルの別tokenを同一カードと扱わない。

応答の閃光制限はcallerのcontextフラグを信頼して迂回できるようにはせず、本人の災いから判定し直す。低レベル `can_append_response` の明示context計算と、スナップショットに本人の災いがあるinstance照会の違いである。

`selection_ready` は与えたtoken列の各prefixがappend可能かを検証し、表示された自己MPで費用を支払えるかを調べる。主手番の空列は、見かけが武器の神器を自己手札に持たないときだけ祈りとして準備済みとする。捨てる/ささげるは対象を含む2枚以上、売るは2枚が必要。応答の空列は防御せず受ける選択として扱う。

これらは対戦の `step` や完全な合法手判定ではない。actor/phase、現在の主動作手番、targetの指定、交換の配分、購入待ち、実体が夢で別物だった場合のサーバー処理は検証しない。応答では公開済み攻撃から作った `ResponseContext` が別途必要であり、未知の相手状態からそれを推測してはならない。低レベルなappend可能性と、実コマンドを送信してよい状態は区別する。

選択のモデル照会や祈り・MP判定に他者の真手札を使わず、本人の見かけと公開情報だけを使う。本人が生存中の偽装カードについても、真のカード型で先に拒否してから可否を返すことはしない。typedなraw state自体をポリシーの観測として渡さず、投影結果を使用する。

### 未投影の項目と範囲

`SnapshotState` / `SnapshotView` は引き続きboard-onlyの型であり、raw attacks、events、rating、ratingGain、ゲームモードを含まない。v48で別型・別queryの先頭攻撃投影を追加した（末尾参照）。v50では3種類の受信pane/GF更新も別のActiveDisplay型で扱うが、全event historyをSnapshotStateへreplayするものではない。それ以外の以下は本実装の外である。

- 攻撃画面全体のlayout・アニメーション、現在の応答者や購入承認phaseのserver上の認定。
- 終局duelのratingパネルによる本名・ratingの公開。
- 通知、ログ、霧中の一時アニメーション、過去に覚えた資源値。
- 終局判定、病気tick、補充、反撃順、抽選、夢の実効果、守護神・終末の遷移。

`is_over` は真モデルtooltip等のfresh表示条件として使うが、このboolだけからゲームモード依存のrating公開を補わない。先頭攻撃と表示商品はv48の独立APIで扱うが、受信攻撃から対局phase・合法actor・未確定のhit結果を推定しない。

### 表示上の18枠

`fG.ir`（12641–12646行）は手札欄を空にして、受信instanceを順に `bb` へ入れる。`bb`（12678–12694行）は表示数が既に18なら `lm` で空placeholderを末尾から探して取り除く。`l2`（12830–12832行）の条件は真のモデル参照がnullであること。`lm→ln`（5443–5451行）は該当が無ければ例外を投げる。

今回のtyped `ItemInstance` は有効modelIdを必要とし、placeholderのmodel0を表さない。このような実instanceだけを19個freshな自己手札へ入れると、公式クライアントは19個目で空placeholderを見つけられず例外になる。よって18個以下をクライアント表示互換の投影範囲として制限する根拠はあるが、これを **未使用物理手札のサーバー容量18** と言い換えてはならない。使用済み奇跡もこの18個の描画対象に含まれる。他者の使用済み奇跡小アイコンの一覧 `j8` には、同じ18枠のチェックはない。

実装の `visible_hand_capacity=18` は、viewer本人の `project_view` / `can_append_hand` / `selection_ready` に適用する。19個以上なら描画互換の範囲外として拒否する。raw stateの他者itemsへ一律18個のサーバー容量制約を課すものではなく、観戦者の投影や他者の公開used_modelsはこの自己手札境界と分ける。


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
