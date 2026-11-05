# official-v2: public catalog facts, 2026-11-02

The active `godfield::official` catalog uses this independently pinned generation of the ordinary public [Japanese asset](https://godfield.net/i18n/ja.json) and [client JavaScript](https://godfield.net/main.dart.js). No account, authentication or match connection was used. Whole localized prose, client implementation and image assets are not bundled; source URLs, HTTP validators and hashes identify the cached observations.

| Artifact | Bytes | SHA256 |
| --- | ---: | --- |
| Original JA JSON | 49,240 | `91b82c2600eac771c1a94dec80f249758496e2bbcb7befe6db52dacec1b55b02` |
| Original client JS | 440,420 | `5ece06c224b6a7a5d877eb45d22ea18b82a03aaf1b7161cbbdf6a1e23ceab2cd` |
| Normalized cards.json | See manifest | `8e465e2d296a2dd039d6e3b6e22956447c1fc4a5572a31e2597cdf20c551dc2a` |

[manifest.json](manifest.json) records the source observation, loader defaults, typed enums and previous fingerprints. [cards.json](cards.json) preserves all 296 item metadata records and omission presence, adding `id=array_index+1`. IDs belong to this official namespace, independently of legacy numeric IDs. The positive gift pool contains237 models with total weight500. The experimental Match defaults to236/499 and enables all237/500 ordinary gifts with `Config.include_fate=True`; owned207 still uses the dedicated Fate API rather than an ordinary Command. Category counts, values, abilities, curses, rates, ID order and source omission presence are unchanged from v1.

## Versioned changes and archive

[upgrade-review.json](upgrade-review.json) compares every field of all296 records. Three rows contain four display changes:

| ID | Field | v1 | v2 |
| --- | --- | --- | --- |
| 19 | imageName | chain-sickle | kusarigama |
| 26 | imageName | glaive-classic | naginata-classic |
| 117 | imageName | slate | stone-tablet |
| 117 | name | 石版 | 石板 |

The [official-v1 archive](../official-v1/README.md), its immutable manifests/cards/source pins and previous crosswalk remain unchanged. The v2 [legacy-v37-crosswalk.json](legacy-v37-crosswalk.json) is regenerated for the new image metadata; mappings remain provisional rather than an assertion of matching game behavior. All37 legacy catalog fingerprints remain frozen. The locale also changes text outside items, including Spanish-language entries and resource labels; item-field comparison does not establish complete locale or JavaScript equivalence.

## Active interfaces and conformance boundary

`catalog_version` is `official-v2`; `catalog_sha256` identifies normalized cards, while `client_sha256` identifies the complete source client. The pure C API ruleset is `official-catalog-client-rules-v2`, retaining ABI1. Match ruleset v47 retains ABI2, uses Snapshot47 (rejecting1..46) and execution view-v45, and preserves these three static provenance fields. Config24/Command184/Status248 byte layouts and phases0..4 are version-gated; Config bytes6/7/20 are the independent strict-bool `external_grants`/`include_fate`/`local_guardian_payloads` flags, with bytes21..23 reserved zero. [The execution contract](../../docs/official-execution-subset.md) separates client facts from experimental server schedules. State/C/Python interfaces and legacy profiles remain separate. Match has C/Python `load_scene` for a board-only quiescent starting state with owner/seed; it does not import an active execution checkpoint, RNG history or pending journal. Native checkpoint/restore remains a separate privileged API.

The executable scope includes nine generic external phenomena287..294/296 plus the dedicated295 shuffle API, explicit devils240..244, threshold sacrifice and reserved-gift phase4. Local chance helpers are separate from player choices. `local_guardian_payloads=True` connects the narrow Earth120/Moon26 sampler to automatic opportunities; it does not reproduce all Earth/Moon roles or the official selector. Incompatible Dream masks,94+232, complete devil coverage of all gift paths and authoritative server schedules remain gaps.

The ordinary `sample_action` never selects207 Fate use and refuses positive-confusion main and grant phase4. The existing Python `random_rollout` therefore stops at those phases; a driver must use the dedicated confusion/grant helpers and steps, and route a player-selected207 token through `sample_fate`/`fate_step`. Full237 gift membership does not make the old ordinary sampler a complete policy driver. The additive v49 choice/driver API connects these phases; the old entry points retain their behavior. No learner is included.

Direct offline execution of the latest client matched50,270 calculation/selection,22,140 state branch and7,898 resource cases (80,308 total). Outputs from archived v1, new v2 and re-execution of the old client also match byte-for-byte. All three extraction scripts accept known generation/hash pairs, reject unknown or mismatched pairs, and extract helpers without executing client startup or network access. State cases include extracted branches and visual stubs. Even successful helper comparison does not prove all UI behavior, server legal actions, event distribution or full engine equivalence.

## Checks

```sh
python3 data/official-v2/generate.py --check
python3 data/official-v2/crosswalk.py --check
python3 data/official-v1/generate.py --check --archive-only
```

To independently reproduce the current reference outputs outside the repository:

```sh
python3 data/official-v2/fetch.py --require-match
node scripts/official_client_oracle.cjs --client /tmp/godfield-official-v2-cache/main.dart.js --out-dir /tmp/godfield-official-v2-oracle
node scripts/official_state_oracle.cjs --client /tmp/godfield-official-v2-cache/main.dart.js --out-dir /tmp/godfield-official-v2-oracle
node scripts/official_resource_oracle.cjs --client /tmp/godfield-official-v2-cache/main.dart.js --out-dir /tmp/godfield-official-v2-oracle
```

All three scripts also accept `--catalog-dir data/official-v1` with an original cached v1 bundle and an external output directory. They reject archive destinations and linked output files before writing either output. A regular existing output can be regenerated. A client/catalog hash mismatch is rejected rather than selecting bindings by filename.

The v2 generator checks the active C++ catalog. The v1 archive mode checks its historical evidence without requiring it to generate the current catalog. Source fetches must match the intended generation: a changed URL response is reviewed into a new version rather than overwriting immutable pins. [The original upstream review](../official-v1/upstream-review-2026-11-02.json) remains historical evidence of the difference detected before v2 adoption.

## v27 verification and gameplay regression

Focused Release16/16 tests passed in3.68s and ASan/UBSan13/13 in5.07s. Match retains88 native groups and63 Python methods; the pure OfficialData suite adds one method checking the three updated rows. V1 source pins, archived facts/crosswalk and legacy37 fingerprints remain preserved.

At seed42, v27 worker4 runs of100,000 two-player games and10,000 nine-player games match the corresponding v26 worker4 baseline in every gameplay/counter field, including checksums `11173370200104368305` / `10091894475017822334` and decisions3,988,255 /2,409,699. Ruleset/static provenance/time are excluded from this comparison. This is a same-worker cross-version regression, not a new v27 worker1-versus-worker4 comparison. Release/SAN1,000-game runs also match, with checksums `760147037809871428` / `4393572629372685365`. No quiet speed measurement or speedup claim is added.

The final full suites passed Release130/130 (54.34s) and ASan/UBSan90/90 (87.17s), with no build warnings or errors. These concurrent suite durations are not engine performance measurements. Independent reruns of all three latest-client oracles reproduced all80,308 TSV cases and all three v2 manifests byte-for-byte. Six file-symlink and six hardlink destinations were rejected before writing, while archive hashes and companion sentinels stayed unchanged; regular output regeneration succeeded. Unknown or mismatched input pairs were rechecked in nine cases.


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

[実クライアントoracle](../../scripts/official_active_attack_oracle.cjs)、[保存report](../../data/official-v2/active-attack-client-v48.json)、[native用TSV](../../data/official-v2/active-attack-vectors-v48.tsv) はp6/tn decode、fu、iI/gf2 summary、iP cardラベル分岐、fq/aT fresh callerとroster lookupを実行する。400 frame例（296単体＋順序付き16＋動的26＋override36＋card境界26）、fresh46例（expected lookup error22）、decoder10例、人工null source2例を区別した。nativeは443行（expected error22を含む）で公開JSONが全て一致した。

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

[LOCAL reference生成器](../../scripts/official_driver_oracle.cjs) / [保存report](../../data/official-v2/driver-local-v49.json) は独立BigIntでenvelope23例、別区分のFate chance stage6例、dispatch契約4行を記録する。reportは55,335bytes、SHA256 `6f8ba9de99f5c6ee3f6440b27faa1c64f4fb80a6729a038a42126e42c381b438`。新しい一次クライアント選択器の発見・実行は0であり、既存v44等の固定証拠を継承する。N0やresponse/purchaseのreferenceは旧samplerへ委譲する直前で止まり、旧sampler自体のdrawまで0と呼ばない。

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

[実client oracle](../../scripts/official_active_display_oracle.cjs)、[保存report](../../data/official-v2/active-display-client-v50.json)、[native TSV](../../data/official-v2/active-display-vectors-v50.tsv) は実ks.kの3受信branchとaa/fu/fq/aT・roster lookupを実行し、47受信シーケンス・53event callを記録する。full handler error8／core error7を分け、native ERROR10にはtyped adapterのstrict未描画field拒否・safeGF上限も含める。native用47vectorはJSON/core結果または指定されたERRORと全て一致した。

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

## v51：現行公開アセットの再取得

別途[ブラウザでの教典・CPU修行の観測](../../docs/official-live-observations.md)と[機械可読記録](live-ui-observations-v51.json)を保存した。初期資源・本人手札の2例と選択previewまでであり、完了済み遷移goldenは0件。以下の取得記録とは証拠の範囲を分ける。

[取得検証record](live-review-v51.json) は2026年11月4日05:39 JSTに、公式HTML・main.dart.js・日本語/英語JSONを公開GETで再取得した証跡である。全4件HTTP200。JS・JA・ENの本文SHA256とbyte列は既存証拠に完全一致し、JAの全296行も省略fieldを保持したまま `id=index+1` で `cards.json` と一致した。正重量237モデル・配布重み500、公開client/台帳で観測されたルール差は0。公式backendの抽選・進行まで照合した結果ではない。

Last-Modifiedは11月3日へ進んだが、JS/JAのETagと本文は同じだった。HTMLだけは初期取得cache比でFirebase import3本が12.17.1から13.1.0へ変わっていた。この比較元は版付きv2のHTML pinではなく、外部Firebase本文の挙動も未検証である。recordにURL・取得UTC時刻・本文size/SHA・HTTP validators・比較範囲を保存し、rawソース全文・一時pathへの依存を含めていない。既存pins・生成物・ルール・保存oracleは更新していない。

## v52：完了した公開対局遷移の照合

[対局証跡](live-transition-witnesses-v52.json) は、公式サイトの2人CPU修行で確認した**完了済み公開遷移3件、取得した完全なserver event列0件**を記録する。モデルIDは表示名・値から本台帳へ対応付け、未知の手札・補充品・乱数は補完していない。v51の部分観測はその時点の履歴として保持する。操作できなかった問題は `A.ut` のheight300とtop55の取り違えを解消して先へ進めたもので、公式の送信拒否を示す事実ではない。

光ATK3の75へ空応答して本人HP40→37/GF2、本人の32自己攻撃でHP37→28/GF5（追加の本人応答操作なし）、CPUの `[45,124,34]` のATK26へ `[150]` のDEF7で応答して本人HP28→9・CPU40→59/GF6を確認した。最後の差19は台帳の合成・防御・吸収から計算する値であり、server damage値を実行器へ入力した結果ではない。

公開clientのfu/aT/fFで自己targetに独立した反対側response paneを作らない経路と整合するため、実行器をruleset48/Snapshot48/view-v46へ更新した。新 `self_attack_rule` の範囲は通常mainの明示自己target・武器base・abilityなし・非確率・非Wideだけで、DEF0即時解決とする。特殊base・反射・bounce等は既存経路を維持する。公式server内部の即時解決と自動空応答は観測から区別できず、致死・特殊効果の順序や公式完全同等性をこの3件から主張しない。詳細は[現行ルールのv52節](../../docs/official-current-rules.md#v52公開対局の完了遷移と通常自己攻撃)を参照。

## v53：Dark自己攻撃の完了例と反射剣のrouting証拠

[追加の対局証跡](live-transition-witnesses-v53.json) は、81さよならの剣の自己使用による**完了遷移1件**と、38反射剣の自己使用についての**routingのみ1件**を分けて保存する。v52と合計した完了公開遷移は4件、routingのみは1件、完全なserver event列の取得は0件。v51/v52の記録・旧pinsは変更しない。

81は闇ATK4を本人へ1回使用し、追加の本人応答なしにHP9→0、CPU勝利でGF6のまま終局した。本人手札9→8、81消費・補充なしを確認した（本人208/112なし）。38は終末GF1の別対局で本人が1回自己使用し、追加defendなしに次の本人mainへ進んだ。ただし補充・悪魔・CPUの途中行動は未取得で、後のGF4/本人HP30/CPU55を38単独の資源goldenにはしない。

実行器をruleset49/Snapshot49/`official-execution-view-v47`へ更新し、`self_attack_rule` のbase条件を `ability-none-or-reflect-weapon` に限定して拡張した。通常main・明示自己target・武器・hitRate0・WideなしをDEF0で即時解決し、他のspecial baseや反射後frame・bounce・counterは既存経路を維持する。Match ABI2は固定。内部即時処理と自動空応答、未観測grantや死亡特殊順序、公式完全同等性をこれらの画面証拠から主張しない。


## v54：能力なし攻撃奇跡の自己使用

[追加記録](live-transition-witnesses-v54.json)に、大木213の初回自己使用、鎖ガマ19への空防御、使用済み大木の後の自己使用という3完了遷移を保存した。累計は7完了遷移と別枠のrouting-only 1件、full server event traceは0件。途中のG.F.3自己使用は次のCPU行動が欠けるため不完全区間として分離する。新しい補充品の未同定modelやCPUの未観測行動は補わない。

対局ruleset50 / Snapshot50 / view-v48は、大木で確認した追加防御不要の経路を、能力なし・hitRate0・非Wideの通常main自己攻撃奇跡へ限定して一般化する。MP消費・同一使用済みコピーの保持・各使用時の補充は既存処理を使い、gift抽選を観測結果に合わせる変更はない。既存pins・旧witness・保存oracle・Match ABI2は保持する。

## v55：特殊自己攻撃のclient照合

[oracle](../../scripts/official_self_attack_oracle.cjs) / [report](self-attack-client-v55.json) は、選択・送信76ケースと独立した受信routing65ケースを実client関数で比較する。14/27/45/58/53/216の自己選択と、条件付きself pane/main activationを確認したが、submit→server→frameの連続取得ではなく、serverの特殊body順・二回攻撃の区切りは未証明である。実対局は完了7件・routing-only1件・full wire0件のまま。

本体ruleset51/Snapshot51/view-v49は、この6種・4familyを既存bodyのまま限定self経路へ追加する。14のfresh2body・消費/課金1回・自然終局/owner実死亡での打切り・cutoff/gift接合等は実験規約とし、ABI2は固定する。[契約の詳細](../../docs/official-execution-subset.md)と[現行ルールv55](../../docs/official-current-rules.md)を参照。過去の応答経路の記述は歴史として残す。

reportは79,743bytes、SHA256 `d0ab012b0724c9e7a93f6cc16e09f728f5abdb4e5a3e0b81ad567ae71e8e643c`、`--check`でbyte一致する。別公開GETのJS/JAは本文SHA・ETagがpinと同一で、Last-Modifiedのみ11月4日07:33:42 GMTへ進んでいた。取得metadataをreportに保存し、rawソース本文・旧pinsは更新していない。


## v56：精霊の杖自己使用と自己売却の公開遷移

[対局記録](live-transition-witnesses-v56.json)に、4＋26の自己売却、69つるぎ焼き（石2）への空防御、46精霊の杖（無12、cutCost）の自己使用という完了3件を追加した。累計は完了公開遷移10件・別枠routing-only1件・full server event列0件。各例の合成72変種によるnative720比較と、従来の反射剣72routing probeを実測件数から分ける。

自己売却は両者40/10/20と手札9を保ちG.F.1→2、空防御は本人40→38・G.F.2→3、46自己使用は追加defendなしで本人38→26・MP10/CP20・CPU40/10/20・手札9を保ちG.F.3→4まで記録した。自己売却後の補充46は後のtooltipで同定したが、engineのgift outcomeに指定しない。4/26の観測総数と、新giftの寄与を除く元コピー消費・配送のnative検査を分け、未知の攻13補充も補完しない。server token・隠れた在庫・中間CP順は未取得である。

本体ruleset52 / Snapshot52 / view-v50は、通常main・明示自己target・hitRate0・非Wideの武器baseへcut_costを追加する。現台帳で増えるのは46のみで、143/163/173の防具や204雑貨へbase条件を広げない。単品46実測から合成列へは、既存の完成列MP費用・隣接FREE・保持・消費を使う限定一般化である。Match ABI2と旧pins・旧witnessを維持し、v55特殊6種の未実測bodyや94＋Wide・178終末grantの未対応範囲は変更しない。[実行規約](../../docs/official-execution-subset.md)と[一次記録の説明](../../docs/official-current-rules.md)を参照。


## v57：終末中の夢・取引・悪魔後の再授与

[対局記録](live-transition-witnesses-v57.json)に、他者売却・無4への空防御・夢222・石けん206・46＋見かけ231と悪魔・CPU購入と悪魔・祈りと悪魔の7完了例を追加した。累計17完了例・独立routing-only1例・full wire trace0件。記録日は2026年11月5日、個々のcapture timestampは未保存のため観測日は11月4〜5日の範囲とする。

nativeには先の4例と祈りの5例を追加し、累計15例1080公開投影＋別枠72routingを検査する。46＋231とCPU購入の複合2例は、未取得のCPU在庫・補充を補わず観測記録のみとする。先4例の通常補充は合成completion、祈りの241→通常0は結果を外部入力した比較であり、実serverの配布や抽選の再現とは呼ばない。

祈り例ではHP50→30、手札8→9を確認し、悪魔後にも神器授与が続いた。石けんでused222を除去しても本人のDreamは残った。新しい品の真偽・model、受取人をまたぐ画面順とbackend順の対応は確定しない。本体ruleset52 / Snapshot52 / view-v50、Match ABI2・台帳pins・旧記録は維持し、178終末grant・94＋Wide・互換外Dreamの未対応範囲を広げない。

記録は26,716bytes、SHA256 `a68dcef535c1918888c104c6d4638e8047d83dfb0edda3743a05cb2d4b917a4a`。件数・native対象ID・ローカル参照リンクを照合し、未同定の品や日時を補完していないことを確認した。


## v58：夢見る帽子の一新を含む完了観測

[追加記録](live-transition-witnesses-v58.json)に、2026年11月5日の終末ありCPU修行で、201→CPUの271→本人178防御→一新と後続244→G.F.12を取得した1完了遷移を保存する。累計18完了例・独立routing-only1例・full wire trace0件。本人38/0/20から178で闇2を防ぎ、Dream・旧使用済み226/214を含む手札の置換、CP＋10・MP＋10、最終手札10を確認した。

8置換結果が全て通常品だったとは確定しない。途中の重なりとsortの後には通常品9と一時的な244が見え、一新と後続補充のどちらにその悪魔が含まれたか、二回の244が同じslotを使ったかは不明である。真モデル・fakeやserver乱数を補完せず、redrawへの悪魔適用を必須にも除外にも決めない。

本体ruleset52 / Snapshot52 / view-v50とABI2、旧pins・記録を維持する。nativeの15例1080公開投影＋72routingへ、別の72一新コンポーネント検証を加える。178の防御・Dream・旧usedコピーの消失を照合するが、原発201の予約枠が無い合成境界のため、今回の手札10・次手番までのfull goldenへ数えない。

記録は14,893bytes、SHA256 `cfbc441ee1b945c03dce65d8a1b26d14375000543c6f2c2fd10790c7431bfd90`。件数・native対象ID・相対リンク・別GET本文hashを照合した。Release・ASan/UBSanのlive test各1/1と、既存redraw/grant oracleのbyte一致を確認した。旧記録・reportを変更していない。

## v59：匿名化した連続受信状態と明示通常授与

[wire-game-updates-v59.json](wire-game-updates-v59.json)は同一CPU修行のupdateCount1〜4を保持する検証専用fixtureである。[live-transition-witnesses-v59.json](live-transition-witnesses-v59.json)に、23自己使用・62空防御のCold付与・199自己解除の3完了操作と証拠の範囲を記録する。累計21完了例・独立routing-only1例。取得した初期game1＋受信batch3は有限範囲で、全対局・全backendイベント列は0件である。

Firefoxの応答本文保存を使い、名前・userId・room path・updatedAtを除去した。受信slot ID・model0 placeholder・省略field・events順は保持する。認証header/HARを取得・格納せず、rawはpermission600の `/tmp` のみ。CPU真モデルを含むprivileged fixtureであり、policy向け公開観測へ流用しない。受信id6/1の再利用とnative fresh tokenは別に対応付け、CPU184は手札差分からの結果入力であってgiftイベントではない。

本体ruleset53 / Snapshot53 / view-v51は設定配布プール・非Dreamの通常神器を外部chanceで指定できる。通常0・悪魔・sample_grant・既定配布とABI2を維持する。新wire targetは8seed・4checkpoint＝32比較、既存live targetは15例1080投影＋72routing＋72一新部分検証のまま。旧pins・旧witness・旧reportを変更しない。

| v59 artifact | bytes | SHA256 |
| --- | ---: | --- |
| `wire-game-updates-v59.json` | 18,596 | `4da3062f4d14673aa1c04f3583e18fdc6aee95a84ae7f991e7307ab4784da649` |
| `live-transition-witnesses-v59.json` | 15,278 | `81048035378287ffd6d38528c5206864618b876eecc045ab13709d468ce08778` |

4受信gameは、private本文から名前置換・userId/updatedAt除去だけで全値一致した。全4本文が同一のtraining room文書を参照することと、rawのbytes/SHA・mode600も独立に確認した。rawのroom識別子や認証情報は出力・commitしない。

## v60：開始〜終局の受信列と補充中死亡

[wire-game-updates-v60.json](wire-game-updates-v60.json)に同一CPU修行のupdateCount1/2、[live-transition-witnesses-v60.json](live-transition-witnesses-v60.json)に1完了操作を保存する。累計22完了例・独立routing-only1例。startGame＋CPU起動からendGameまで連続する受信game列1対局分を得たが、CPU通常giftは省略され、全内部イベントtrace0とは区別する。

CPU32＋51のATK22へ本人118＋142のDEF8で応答し、14被害と補充中の241→242でHP26→6→0、GF1維持・CPU勝利を確認した。CPU243による191/206除去と新通常135/172は、受信イベントと手札差分を区別する。CPU通常2結果は両順を試す仮定付きcompletionで、新16再生・32checkpointを別に検査する。終局の本人は実物8＋empty1で、emptyをlive debtへ一般化しない。

匿名化は元name→cpu/viewer・userId/updatedAt除去だけで、game数値・省略field・event順・zero rowは保持する。rawはmode600の一時ファイルのみ。`ReceivedState` がemptyと物理stateを分離するが、入力raw・物理snapshot・empty metadataは検証専用であり方策viewではない。runtime/ABIと旧pins・旧witnessは変更しない。

| v60 artifact | bytes | SHA256 |
| --- | ---: | --- |
| `wire-game-updates-v60.json` | 12,552 | `890d98320f5f4dc7371e942ad01e395ad85f5ccb53e0472f99f8b05f67eec021` |
| `live-transition-witnesses-v60.json` | 11,395 | `78495d4fea76803d267639f1e20d95c852a8257a1dd604dc61e39e2ceebad726` |

初期本文27,143bytes/終局本文23,365bytesのSHAと同一room参照、指定した匿名化だけでの全game値一致を独立確認した。誤って初期応答を再保存した一時ファイルは出典に含めない。

## v61：Dream結果の明示入力（新witness追加なし）

ruleset54 / Snapshot54 / view-v52の新 `ArtifactGrantAction` は、現互換クラス内actual/fakeまたは明示unmaskedを外部chanceで受け取る。新C16 byte型を追加するが、既存GrantAction32 byteとABI2・通常0/samplerを維持する。新たな公式Dream pairを今回確定していないため、台帳pins・過去witness・件数は変更しない。合成入力テストは完了観測22例・native対象19例へ加算しない。詳しい入力契約は[実行規約](../../docs/official-execution-subset.md)を参照。

## v62：非終末の天国草・病気処理・通常授与

[wire-game-updates-v62.json](wire-game-updates-v62.json)へ同じ終末なしCPU修行のupdate1/2、[live-transition-witnesses-v62.json](live-transition-witnesses-v62.json)へ1複合完了操作を保存する。198自己使用のMP＋20・Heaven付与・disease/HP＋5・本人191補充、その後のCPU191自己回復を同じbatchで取得した。CPU207は最終在庫差分からの記録結果で、giftイベントや率の証明とは区別する。

runtimeはruleset55/Snapshot55/view-v53。新external_artifactsは全GFの予約通常補充へ入力できるが、悪魔は旧終末modeだけ、reset/即時redraw/取引配送は変えない。元識別情報とroom path・headerを含めず、game内の数値・field省略・events順・受信slot再利用は保持する。旧pins・witness・oracleは変更しない。

| v62 artifact | bytes | SHA256 |
| --- | ---: | --- |
| `wire-game-updates-v62.json` | 10,303 | `feb6b6f323e12ce51ecb558749477601c4c263154b02fe124b35716597d5c0d1` |
| `live-transition-witnesses-v62.json` | 8,621 | `74329bbddf358fae79ff25351fae285cf8144ebd4ad201156eaa7be2cef1bcb6` |

新1例で累計23完了観測・native対象20例。固定8seed×2checkpoint＝16比較を従来の集計と分ける。同一room参照と指定した匿名化だけでの全game値一致、raw mode600、元識別情報の不在、受信イベント列の一致を独立検査した。


## v63：Dream自己使用と同一番号の偽装

[wire-game-updates-v63.json](wire-game-updates-v63.json)は終末なしCPU修行のpost-only updateCount2、[live-transition-witnesses-v63.json](live-transition-witnesses-v63.json)は80自己使用の1完了例を保存する。本人HP40→36・Dream・通常補充135/fake135・次CPU60の攻撃開始を確認した。初期wireは未取得で、残在庫とuseイベントから作るpre-play sceneは再構成である。別対局の初期資料を混ぜず、原初期draw/orderや完全なbackend列を主張しない。

runtimeはruleset56/Snapshot56/view-v54。同番号fakeのpresenceを保ち、従来の異番号9key生成器・率・RNGを維持する。CPU真手札はprivileged検証専用で、policyへ渡さない。元識別情報・updatedAt・room path・headerを除き、受信ゲーム数値・省略field・events順・slot再利用を保持する。旧世代のpins/記録は変更しない。

| v63 artifact | bytes | SHA256 |
| --- | ---: | --- |
| `wire-game-updates-v63.json` | 8,525 | `ed52d185c30650490e657f7c0160d5451cccfd339ae2a3850a2cbb6c50826101` |
| `live-transition-witnesses-v63.json` | 7,455 | `9faafbc7ce558b2289c74bca809cf08b7c76c25623f9283b2c1ed32afae7fffb` |


## v64：Dream偽装の使用と異番号の回復量

[wire-game-updates-v64.json](wire-game-updates-v64.json)は同じ終末なしCPU修行の連続受信update4〜7、[live-transition-witnesses-v64.json](live-transition-witnesses-v64.json)は4完了操作と範囲を保存する。135/135防御・195/195自己使用、見かけ195/MP＋5から実197のMP＋15、CPU両替と花の後続を取得した。最初のGF3 sceneはUI資源と消費前copyを戻した再構成であり、game3や元の初期draw/orderは未取得である。

runtime56/Snapshot56/view-v54を変更せず固定8seed×4受信checkpointを比較する。CPU188/196/202は在庫差分の結果で、eventsに無いgift/retryやbackend順を作らない。旧pins/記録を保持し、CPU真手札はprivileged検証専用とする。GF9で退出しホーム復帰・Network Monitor終了を確認した。

| v64 artifact | bytes | SHA256 |
| --- | ---: | --- |
| `wire-game-updates-v64.json` | 19,912 | `c7714518c9b542062699740b6115dfb3facc6bb0e9daa18b116381b64f331fb0` |
| `live-transition-witnesses-v64.json` | 7,819 | `ce55a7a85db9a7e2c6850cc6e4460bcecc27b51e1dfbf8ae507269e53257ec78` |
