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
