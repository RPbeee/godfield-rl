# 物理品による魔法跳ね返し v34

`legacy-magic-bounce-subset-v34` はFLICK_MAGICの正重量6種を追加する限定profile。
CLIの `magic-bounce` と `g34_*` は全拡張を有効にし、224種・旧重み合計477で動く。
現行公式の完全再現ではなく、旧非公式Flashサーバーを参考にした部分実装である。

[一次処理の調査](legacy-magic-bounce-rules.md)、[台帳・出典・hash](../data/legacy-magic-bounce-v1/README.md)、
[前段のHP増減賭けprofile](legacy-hp-gamble-profile.md)を参照。
既存33台帳のhashと保持番号を固定し、対象プール内で旧重みを正規化する。公式の配布確率とは扱わない。

## 6物理品の役割

| 旧ID / local index | type | 普通ATK / boost / DEF | 価格 / 重み | 通常DISCARD |
| --- | --- | --- | --- | --- |
| 21 / 0 | WEAPON | 5 / 5 / 0 | 10 / 1 | 不可 |
| 119 / 1 | PROTECTOR | 0 / 0 / 0 | 5 / 1 | 可 |
| 121 / 2 | PROTECTOR | 0 / 0 / 1 | 5 / 1 | 可 |
| 127 / 3 | PROTECTOR | 0 / 0 / 2 | 5 / 1 | 可 |
| 134 / 4 | PROTECTOR | 0 / 0 / 3 | 5 / 1 | 可 |
| 140 / 5 | PROTECTOR | 0 / 0 / 4 | 5 / 1 | 可 |

すべて無属性・MP費用0・命中率0 sentinel・HP/MP回復0・病気/harmsなし。
`MagicBounceCard{Card card;}` の所属でMAGIC向けFLICK_MAGICを保つ。
21は普通base攻撃ATK5と、別baseへ追加するboost5の両方を残す。単独でATK10にはしない。
残る5防具も非MAGICへの普通DEF0〜4を残し、跳ね返し専用の効果へ役割を剥がさない。
保持能力や新しいcast slotは増やさない。

## MAGIC応答と現在source

入ってくる元typeがMAGICのとき、到達した物理品は通常DEFを加える代わりに必ず跳ね返す。
攻撃奇跡・回復/所持金奇跡・追加ATK奇跡の単独使用など、対応済みMAGICのpayloadを対象にする。
WEAPON・COUNTER・SUNDRYの効果をMAGICと分類替えせず、Noneの237/売買にも発火しない。

応答した物理1枚を消費して配布予定+1を得る。MP費用・保持化・学習・保持再使用はない。
現在sourceを応答者へ替え、自己も含む生存座席から現在targetを選び直す。
自己を選べばその場で元payloadを解決し、他者ならその人物のresponseへ続く。
[210の防御奇跡](legacy-defensive-miracles-profile.md)と動作の骨格は共通でも、消費/費用/保持由来は異なる。

元のamount・属性・命中済み結果・harm/病気・奇跡ID・保持/FREE由来などは保つ。
追加MPの再支払い・再学習・命中再抽選をせず、最初の術者へ適用先を固定しない。
吸収221も現在sourceを使い、selfなら先行回復→被害の順を保つ。

特殊前の普通DEF減算は元amountへ戻る。一方でprefix消費・状態付与・195の属性消去は残る。
元typeがMAGICなので既存の最終属性検査の扱いを使い、Dark/Lightや不適合色prefixだけを理由にMAGIC跳ね返しを拒否しない。
特殊後の未使用FREE suffixは既存の順序付き防御規則に従い、到達した消費品と分ける。

## 共用抽選・価格・予算

既存 `script_bounce_targets(span<uint8_t>)` とgame RNGを、20/210と共用する。
新しい抽選tapeを増やさない。各到達したbounceで生存対象を1回選び、反射のたびに命中を引き直すものではない。
fixture prefix/位置・未来の対象はpolicy観測へ含めず、旧Pythonとのbit単位seed一致は主張しない。

旧216価格表/hashは固定し、magic_bounce有効時だけ6種の価格10/5をprivate `trade_price()` へ補う。
既存BUY4の5・237の1分岐も維持する。全224商品が売買でき、最大価格30は変わらない。
新boost5は既存boost15上限を越えず、普通DEF4も既存上限内なのでraw/counterの上界は増やさない。

`Config::magic_bounce` はデフォルトfalseでmiraclesとreflectionを必要とする。
最小構成ではdefensive_miraclesを必須にせず、216/220の保持castを物理品で返せる。
この場合もheld castの配布予定をretained_dealsとして区別し、満杯手札での保存/復元予算を検証する。

新Action kind・Effect・Pending markerは作らない。
`StepResult::magic_card_bounced` と `magic_bounce_card` が今回到達した物理品を表し、
既存bounced/magic_bouncedもtrueになる。210はmagic_card_bounced=falseのまま。
`State::last_reaction` は実物ID・現在source/target・bounce・from_ability=falseを記録する。
過去記録が後の普通行動にも残るので、件数は永続記録を毎step足さずStepResultから数える。

## 共通観測とC ABI

共通Snapshotはversion36。同じビルド・Config内のfixture用であり、旧形式の永続checkpoint移行はしない。
共通C++観測は1345次元、`g34_*` は先頭1300次元。v34では末尾45特徴が0。旧 `g33_*` は1278、g32以前も従来の出力幅を維持する。
v33では末尾67特徴が0。

| 位置 | 内容 |
| --- | --- |
| 1278 | magic_bounce有効なら1 |
| 1279〜1294 | 自分の手札slotが追加6種のどれかなら1 |
| 1295 | 最後のreaction IDが追加6種なら1 |
| 1296 | その物理品ID / 363 |
| 1297 | そのsourceが自分なら1 |
| 1298 | そのtargetが自分なら1 |
| 1299 | 1295がtrueでsource==targetなら1 |

新C ABI [magic_bounce_c_api.h](../include/godfield/magic_bounce_c_api.h) は `g34_*` version34。
88byte・kind0〜18・最大65参照・tail@17、mask幅36。旧35語を保ち、word35はmagic_bounce_slots。
main/response時の手札所属maskで、purchase/doneでは0。発火可能な候補はreaction_slotsとの共通部分と `legal()` で確認する。
mainでは21のweapon_bases/boost_slotsや防具のdiscard_slotsも既存規則と組み合わせる。
旧g33以前の出力幅・行動配置・従来のRNG/検証統計を保つ。

[Pythonラッパー](../python/magic_bounce_env.py)は `MagicBounceBatchEnv` を提供する。
batchは全active行を検証してから更新し、1行でも不正なら原子的に拒否する。
終了行は固定・報酬0で、不正手を無視し自動resetしない。報酬は固定座席順。
返却バッファーの保存時はコピーし、同一ハンドルを同時操作しない。

```sh
python3 python/magic_bounce_env.py
./build/godfield_legacy_bench 100000 1 42 magic-bounce
./build/godfield_legacy_bench 100000 4 42 magic-bounce
```

ベンチは物理6種の到達eventを210から分け、ID別・self・damage/utility・保持/FREE由来・195使用を集計する。
21のbase/boost使用・非MAGICへの普通消費・売買も別記録する。
全magic_bounces=210使用数+物理6種bounce数、weapon_blocks=215使用数という内訳を検査し、
非selfのbounceを未解決として扱って元奇跡のHP/所持金効果へ混ぜない。
checksum version34だけへ新event/LastReaction・共用bounce tape・retained予算を混ぜ、旧33までの式と保持loopを固定する。

117死亡時攻撃・238追加ATK・MYSTERY243・霧/幻覚・未対応状態奇跡・守護神・終末・複数人戦、
旧serverの負値/99超交換や取得順保持リスト、現行公式との動的照合、学習器・学習済みモデルは未対応。
衝突C01〜C05も残す。受入検証は6物理品/21普通攻撃・boost/armor普通DEF、MAGIC対象と非MAGIC非発火、
self/他者・prefix/195/FREE・保持/FREE元payload・再命中なし、最小構成のheld gift予算、
売買価格・復元/秘匿・旧ABI境界・旧33profile互換を対象とする。

## 検証結果（2026-11-01）

通常CTest104件、ASan/UBSan70件、34種類のPython suite計391テストが通過。
6物理品のMAGIC-only発火、21のbase/boostと5防具の普通DEF、self/他者・prefix/195/FREE、
元保持/FREE由来・命中結果の保持、最小構成のfull-hand held gift予算、価格fallback・復元/秘匿・旧ABI境界を固定fixtureで確認した。
既存33profileは各10万試合で、従来の全315項目の決定的出力が基準と一致した。
全機能有効・10万試合・seed42・意思決定上限512で、1/4スレッドの全333項目の決定的出力が一致した。
ASan/UBSanのRelWithDebInfoビルドでも1万試合の全333項目がReleaseと一致した
（checksum version34: `8258821872869757785`）。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 3,317,281 |
| 座席0 / 座席1勝利 | 49,644 / 49,553 |
| 勝者なし / 打ち切り | 803 / 0 |
| 物理品によるMAGIC bounce / self | 1,966 / 1,000 |
| damage / utility payload | 1,966 / 0 |
| 保持cast / FREE由来のbounce | 681 / 179 |
| 195使用と同stepのbounce | 34 |
| 21の普通base攻撃 / boost使用 | 2,669 / 4,941 |
| 非MAGICへの6種の普通消費枚数 | 15,590 |
| SELLの商品 / 購入品となった6種 | 931 / 464 |
| 21 / 119 / 121のbounce | 265 / 354 / 311 |
| 127 / 134 / 140のbounce | 351 / 337 / 348 |
| checksum version34 | 14052509873477545836 |

到達eventをStepResultから集計し、普通行動の後も残るLastReactionを重複計上しなかった。
自然runのutility0は非対応を意味せず、対応済みutility MAGICや最小構成の保持castを固定fixtureで検証した。
元のMP/学習と物理応答の消費を分け、210由来と新6種の内訳・6ID別合計・self/他者を検査した。
