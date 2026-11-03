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

`SnapshotState` / `SnapshotView` は引き続きboard-onlyの型であり、raw attacks、events、rating、ratingGain、ゲームモードを含まない。v48で別型・別queryの先頭攻撃投影を追加した（末尾参照）。それ以外の以下は本実装の外である。

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
