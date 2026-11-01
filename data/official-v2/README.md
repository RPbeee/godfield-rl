# official-v2: public catalog facts, 2026-11-02

The active `godfield::official` catalog uses this independently pinned generation of the ordinary public [Japanese asset](https://godfield.net/i18n/ja.json) and [client JavaScript](https://godfield.net/main.dart.js). No account, authentication or match connection was used. Whole localized prose, client implementation and image assets are not bundled; source URLs, HTTP validators and hashes identify the cached observations.

| Artifact | Bytes | SHA256 |
| --- | ---: | --- |
| Original JA JSON | 49,240 | `91b82c2600eac771c1a94dec80f249758496e2bbcb7befe6db52dacec1b55b02` |
| Original client JS | 440,420 | `5ece06c224b6a7a5d877eb45d22ea18b82a03aaf1b7161cbbdf6a1e23ceab2cd` |
| Normalized cards.json | See manifest | `8e465e2d296a2dd039d6e3b6e22956447c1fc4a5572a31e2597cdf20c551dc2a` |

[manifest.json](manifest.json) records the source observation, loader defaults, typed enums and previous fingerprints. [cards.json](cards.json) preserves all 296 item metadata records and omission presence, adding `id=array_index+1`. IDs belong to this official namespace, independently of legacy numeric IDs. The positive gift pool remains 237 models with total weight500; the experimental Match uses229/490. Category counts, values, abilities, curses, rates, ID order and source omission presence are unchanged from v1.

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

`catalog_version` is `official-v2`; `catalog_sha256` identifies normalized cards, while `client_sha256` identifies the complete source client. The pure C API ruleset is `official-catalog-client-rules-v2`, retaining ABI1. Match ruleset v27 retains ABI2 and229/490 gameplay rules; Snapshot27 rejects earlier snapshots, and execution view-v25 adds these three static provenance fields. [The execution contract](../../docs/official-execution-subset.md) separates client facts from experimental server schedules. State/C/Python interfaces and legacy profiles remain separate.

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
