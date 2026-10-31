# 状態・解除奇跡 v36

`legacy-status-miracles-subset-v36` は状態付与/解除MAGIC5種を追加する限定profile。
CLIの `status-miracles` と `g36_*` は全拡張を有効にし、230種・旧重み合計484で動く。
現行公式の完全再現ではなく、旧非公式Flashサーバーを参考にした部分実装である。

[一次処理の調査](legacy-status-miracles-rules.md)、[台帳・出典・hash](../data/legacy-status-miracles-v1/README.md)、
[前段の物理boost profile](legacy-sundry-boost-profile.md)を参照。
既存35台帳のhashと保持番号を固定し、対象プール内で旧重みを正規化する。公式の配布確率とは扱わない。

## 5種の状態/解除と費用

| 旧ID / local index | 属性 | 効果 | MP | 価格 / 重み |
| --- | --- | --- | --- | --- |
| 202 / 0 | 無 | lower解除 | 3 | 10 / 1 |
| 208 / 1 | 闇 | DarkCloud直接付与 | 5 | 10 / 1 |
| 211 / 2 | 無 | all解除 | 5 | 15 / 1 |
| 212 / 3 | 無 | Cold直接付与 | 6 | 10 / 1 |
| 224 / 4 | 無 | Heaven直接付与 | 15 | 15 / 1 |

全てMAGIC、value/命中率0、普通DEF/防御特殊roleなし。
`StatusMiracle{id,name,Element,Cure,Disease,Harm,cost,price,weight}` は各行1種類だけ非noneの効果を持つ。
武器の追加魔法やraw攻撃ではなく、208の闇属性もDARK999被害へ変換しない。
Fog/Illusion付与・助手・MYSTERY・117死亡時攻撃は追加しない。

手札は既存USE kind3で、自己または相手を対象にする。初使用は物理1枚消費・保持1コピー取得・MP支払・配布予定+1。
同IDの重複初取得は保持数を増やす。保持castは数を減らさずMPを払い、同じく配布予定+1を得る。
病気が無い人物への解除など無変化の使用も合法。相手に反射/遮断されても元の支払/取得を戻さない。
自己は即解決、他者は応答へ進む。保証命中なので命中乱数は使わない。

FREEだけを後置でき、最初の後置FREEの直前1品を無料化する従来規則を使う。
無料でも初取得・消費・配布予定は発生する。初期MP10で224の有料使用は不可だが、224+FREEは使える。
通常boost・setter・追加197/217/214/209を付けたり、新5種をWEAPONへ追加したりしない。

## 付与・解除・Heavenと応答

202はCold/FeverとGloryを取り除く。Cold/Feverを除去したときだけ悪化率0へ戻し、Hell/Heaven/DarkCloudは保持する。
211は全病気・対応済みharmsと悪化率を消す。解除はaddHarmではないため、Heavenへ使っても転落しない。
Fog/Illusionは付与源/UIとも未対応のままで、その実装を今回追加しない。

Heaven以外への208はDarkCloudを追加し、重複は増やさず、病気/悪化率を変えない。
212/224は既存病気よりincoming rankが高ければそれを採用し、同じ/低ければ現在病気を1段進め、悪化率0へ戻す。
Cold反復はCold→Fever→Hell→Heavenとなる。224はHeaven以外をHeavenへする。

既にHeavenへ208/212/224を使うと、HP0・悪化率0へ転落してHeavenを保持し、incoming状態を新追加しない。
これが208による即死の条件であり、闇属性自体によるraw999攻撃ではない。
効果直後の死亡/244を確認してから、元turn_ownerの自然tickへ進む。
自己Cold初付与ならその末に1被害、自己Heaven初付与なら5回復が起きるが、純付与の直接HP変化と自然tickは分ける。
202で自分のColdを消せばそのtickを止め、Heavenなら保持して5回復し得る。211でHeavenを消せばその5回復もなくなる。

普通防具のDEFはstatusを打ち消さず、通常応答後も付与/解除が届く。195も208をNE化するだけでDarkCloudを消さない。
MAGICなので210・物理6種の跳ね返し・魔法反射/遮断・196を既存規則で使える。
MP・元奇跡ID・保持/FREE由来・payloadを保ち、現在source/targetへ返す。self bounceはその場で解決し、再支払/再学習しない。
prefix病気や消費は残るため、175でHeaven転落した後に211が病気を解除しても、先のHP0を回復はしない。
244があれば解除後の状態で救われ、owner tickにも解除済みの病気を再適用しない。

**新5種を防御用のzeroDEF MAGICとして消費する入力は採らない。**
原典weakguardはMP支払・保持取得・配布予定だけを行い、mainの付与/解除は実行しない応答を受理し得る。
応答時の手札交換や学習に戦略的意味があり得るが、公式UI未照合で、従来の非防御MAGICと同じcanonical除外を維持する。

## 独立5保持・忘却27種・価格

`Config::status_miracles` はデフォルトfalseでmiracles/disease/celestialを必要とする。
旧main/防御/追加/倍化/全体化の配列とindex、旧21/22の忘却ID定数は固定する。
新 `Player::status_abilities[5]` はuint32、local0〜4が表の5IDに対応する。
通常main16・defensive2・status5の保持合計をmax_decisions以内に検証し、追加群の15×max_decisions上界は変えない。

新 `ActionKind::cast_status=19` はbase0〜4とtarget0/1を指定し、slots/extra/counter等を0にする。
FREE保持castは既存CAST_ORDEREDのbase48〜52。旧32〜47や他の参照を動かさない。
通常のcast kind5へ新5indexを詰め込まない。

新flag時だけ `status_retained_type_ids[27]` のID昇順で保持コピー数に比例して忘却する。

`197,198,199,200,202,203,204,205,206,207,208,209,210,211,212,214,215,216,217,218,219,220,221,222,223,224,226`

非空T1も1回・空T0は0回という既存ticket消費を保つ。取得順listのindex/旧Pythonのseed→ID一致は主張しない。
新flag=falseでは新5数は0なので旧21/22の抽選分布・RNG/台帳対応を維持する。
売買/240除去は物理品だけを動かし、学習や状態効果を発火しない。旧216価格表は固定して5価格だけfallbackへ加え、最大30を保つ。

新Effect.statusを末尾13へ追加し、Pending.miracle/amount0/cure/disease/harmを正確な5種の組で保持する。
新Pending marker・history・tapeは追加しない。既存通常病気用status_cardは新MAGICの識別に流用しない。
StepResultのstatus_first_use/recast/resolvedとid/現在source/targetを、全奇跡の共通取得/MPeventとともに記録する。
208は既存harm_*にも計上し、212/224は既存apply_disease同様にgeneric harm_*を増やさず、新status検算で扱う。

## 観測とC ABI

共通Snapshotはversion36。同じビルド・Config内のfixture用であり、旧形式の永続checkpoint移行はしない。
共通C++観測と `g36_*` は1345次元。旧 `g35_*` は1317、g34以前も従来の出力幅を維持する。
v35では末尾28特徴が0。

| 位置 | 内容 |
| --- | --- |
| 1317 | status_miracles有効なら1 |
| 1318〜1333 | 自分の手札の新5種を(local+1)/5で表す。その他0 |
| 1334〜1338 | 自分の新5種の保持数 / max_decisions |
| 1339 | 現在の保留が新statusなら1 |
| 1340 | 保留localを(local+1)/5で表す |
| 1341 | 保留Disease / 4 |
| 1342 | 保留Cure / 2 |
| 1343 | 保留Harm / 2 |
| 1344 | 新status保留かつ保持由来なら1 |

既存aggregate1016は新5も含む現在の総保持数になる。旧13個別・221個別などの座標は動かさない。
新profileでtail以外が常に同値とは主張せず、旧flag=falseの各profileの値/幅を維持する。
相手の手札/保持数、game RNG、未使用ticketやprefixは観測しない。

新C ABI [status_miracles_c_api.h](../include/godfield/status_miracles_c_api.h) は `g36_*` version36。
88byte・kind0〜19・最大65参照・tail@17、mask幅40。旧37語を保ち、word37はmain手札5種の所属、
word38はMP/所持を満たすstatus_ability_slots、word39はFREE用free_status_ability_slots。
物理使用はutility/free cast maskと組み合わせ、参照/順序/対象は `legal()` で検証する。
旧g35以前はkind19を拒否し、出力幅・行動配置・従来のRNG/検証統計を保つ。

[Pythonラッパー](../python/status_miracles_env.py)は `StatusMiraclesBatchEnv` を提供する。
batchは全active行を検証してから更新し、1行でも不正なら原子的に拒否する。
終了行は固定・報酬0で、不正手を無視し自動resetしない。報酬は固定座席順。
返却バッファーの保存時はコピーし、同一ハンドルを同時操作しない。

```sh
python3 python/status_miracles_env.py
./build/godfield_legacy_bench 100000 1 42 status-miracles
./build/godfield_legacy_bench 100000 4 42 status-miracles
```

ベンチは新5を旧offensive/raw攻撃へ混ぜず、初取得/再使用/実MP/FREE/忘却/保持在庫の収支を全奇跡合計と照合する。
純効果はprefix175を先にreplayし、効果直後の病気/harmsを求めてlast_tick.before/afterへ照合、244回復と自然tick実差を分離する。
Heaven転落の直接HP損失、lower/all解除、病気変化・Glory/DarkCloud除去/追加、反射/bounce/block、売買/除去も別記録する。
checksum version36だけへ新5counts・27-ID忘却順・status event/payloadを混ぜ、旧35までの式/loopを固定する。

## 新しいimmutable source証拠

固定commit `fcc83740a3d3cd9968e6e5cd2b7ed781f53abe80` のCSV全364行・10,915bytesをgh APIで取得した。
[raw archive](../data/legacy-status-miracles-v1/immutable-item-data.csv) のSHA256は
`a6b6892d6234656c40d5171b042ce7aeefcd309ae1abcd6c3fa61e55c552f8b1`。
[照合証拠](../data/legacy-status-miracles-v1/immutable-comparison.json)により、新5行と旧225種の1531 numeric facts・4169 source-column checksが一致した。
cached web masterの全364行から再構成したbytesも一致する。generatorはこのraw hashと照合を再検証する。
過去35台帳の取得時点のimmutable未検証flagは変更せず、新v36manifestだけに新証拠を記録した。
固定旧commitの再現性を確かめる証拠であり、現行公式の挙動/名称や将来masterの同一性を確定するものではない。

117死亡時攻撃・MYSTERY243・霧/幻覚付与・助手/守護神・終末・複数人戦、
旧serverの負値/99超交換や取得順保持リスト、現行公式との動的照合、学習器・学習済みモデルは未対応。
衝突C01〜C05とcanonical応答の受理差も残す。受入検証は5種MP/初取得/FREE/cast19/48〜52・忘却27、
Heaven/解除/prefix/244/tick順、全MAGIC反応・秘匿/復元・旧ABI境界・旧35profile互換を対象とする。

## 検証結果（2026-11-01）

通常CTest111件、ASan/UBSan75件、36種類のPython suite計437テストが通過。
5種MP/初取得/再使用/FREE/cast19・48〜52、旧22保持固定と忘却27、純statusのMAGIC反応、
既Heaven→244→owner tick・202のHell/Heaven保持・211解除・175prefix、zeroDEF応答除外、
価格/在庫予算・復元/秘匿・旧ABI境界を固定fixtureで確認した。
既存35profileは各10万試合で、従来の全358項目の決定的出力が基準と一致した。
全機能有効・10万試合・seed42・意思決定上限512で、1/4スレッドの全387項目の決定的出力が一致した。
ASan/UBSanのRelWithDebInfoビルドでも1万試合の全387項目がReleaseと一致した
（checksum version36: `16648244832006702901`）。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 3,295,778 |
| 座席0 / 座席1勝利 | 49,380 / 49,788 |
| 勝者なし / 打ち切り | 832 / 0 |
| status初使用 / 保持再使用 | 14,199 / 15,311 |
| 実MP支払 / 新規保持 | 138,458 / 14,199 |
| 使用 / 解決 / 現在selfで解決 | 29,510 / 28,943 / 16,235 |
| lower / all解除 | 8,689 / 7,310 |
| DarkCloud / Cold / Heaven適用 | 6,721 / 4,641 / 1,582 |
| 効果直後のHeaven転落 / その実HP損失 | 436 / 17,585 |
| 病気変化 / 病気除去 | 6,406 / 561 |
| Glory除去 / DarkCloud除去 / DarkCloud新追加 | 221 / 145 / 3,405 |
| FREE使用 | 2,012 |
| 反射 / bounce / 遮断 | 342 / 470 / 567 |
| 保持由来の解決 | 15,124 |
| 忘却した新5コピー / 終端の新5保持在庫 | 418 / 13,781 |
| 売却商品 / 購入品 / 240除去の新5 | 871 / 365 / 96 |
| checksum version36 | 14937203584757521763 |

新5の保持在庫13,781と忘却418が新規保持14,199に一致し、全奇跡の収支にも含まれた。
prefix175・純効果・244・自然tickを分け、last_tick.before/afterと実HP差で順序を検算した。
新statusをraw攻撃へ混ぜず、5種類の解決内訳・既Heavenの転落・解除後のGlory/DarkCloudを確認した。
