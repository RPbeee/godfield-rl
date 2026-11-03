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

The ordinary `sample_action` never selects207 Fate use and refuses positive-confusion main and grant phase4. The existing Python `random_rollout` therefore stops at those phases; a driver must use the dedicated confusion/grant helpers and steps, and route a player-selected207 token through `sample_fate`/`fate_step`. Full237 gift membership does not make the old ordinary sampler a complete policy driver. No learner is included.

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
