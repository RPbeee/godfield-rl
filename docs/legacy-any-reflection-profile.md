# 万能反射196と反撃の返却 v17

`legacy-any-reflection-subset-v17` は万能反射196を追加する限定profile。
CLIの `any-reflection` と `gu_*` は既存拡張も有効にし、186種・旧重み合計376で動く。
旧非公式エミュレーターを参考にしており、現行公式との完全一致は確認していない。

[処理順の調査](legacy-any-reflection-rules.md)、[台帳・出典・hash](../data/legacy-any-reflection-v1/README.md)、
[前段の魔法反射profile](legacy-magic-reflection-profile.md)を参照。
既存16台帳を変更せず、追加1種を含むプール内で旧重みを正規化する。公式の配布率とは扱わない。

## 先頭と前置防具ありの違い

196は無属性の防具。ATK・追加ATK・通常DEF・HP/MP増加・MP費用はいずれも0、価格10・重み1。
保持化せず、通常手番では捨てることだけが可能。応答で選べば必ず反射が発火する。

| 使用する順序 | 反射できる対象 |
| --- | --- |
| 196を先頭に使う | 現在対応する全属性の武器、命中済み％攻撃、奇跡、通常回復・解除、反撃 |
| 普通防具・195などの後に196 | MAGICは属性を問わず可能。それ以外は、実際の195処理後に無属性で、counter由来でない場合のみ |

`defend.base` に196の手札slotを指定し、`slots=0` なら先頭使用。
`slots` に前置防具を含めれば、後者の制限を検証する。
闇武器を195→196で返すことはできるが、counterを195→196で返すことはできない。
闇・有色のcounterを返したい場合も196を先頭に使う。
Gloryの既存単枚制限では196も1枚に数え、前置防具との2枚選択を拒否する。

196を普通防具のslotsへ入れて反射を省略したり、別の特殊応答より前に通常DEFとして消費したりする行動は拒否する。
196の後に旧UIで選んだ未使用suffixは、実際の消費と状態遷移に影響しないため意味的行動から省略する。
元XMLに未使用選択が載る情報公開まで含めた完全なwire trace再現は対象外。

前置防具は消費・配布予定・病気等の副作用を残すが、返す威力は元payloadの値を使う。
195による属性変更は元payloadにも適用されるので反射後も残る。
反射自体は現在sourceとtargetを交換し、別の攻撃を再生成しない。
元の属性・威力・奇跡・保持使用・付随状態・命中記録を保持し、MP支払・保持取得・命中抽選をやり直さない。

## 反撃の元所有者と現在source

190土・187火75％・191直接Glory・192直接DarkCloud・194所持金の反撃は、196先頭で返せる。
193自己MP回復は自己対象で自動解決し、196を選ぶ応答がない。通常の自己対象奇跡等も同様。
武器用20/39・魔法反射4種・210・215の型条件は維持し、counterをそれらで返せるようにはしない。

`Pending::origin_source` は消費したcounterの元所有者として固定し、
`source/target` は反射ごとに交換する。元の手札消費・配布予算と、現在の効果の受益者を分ける。
194は**反射後のcurrent source**へ所持金を加算し、current targetへ同額を請求する。
191/192も反射後のtargetへ付与し、187は起動時に命中済みのまま返す。
起動時のDarkCloudによる必中も記録のままで、新targetのDarkCloudを見て再判定しない。

同じcounter項目を返し合っても、生成数・pop数・queue件数を増やさない。
新たに普通防御して再counterを予約する場合は、現在の相手sourceをtargetとする新項目をFIFO末尾へ加える。
元のturn_ownerは変えず、全項目が終わるまで病気tick・補充・勝敗判定を待つ。

## 死亡済み対象への返却と取り消し

死者sourceからの予約済みcounterは従来どおり生者へ実行される。
そのcounterを196で返し、新targetが既にdeadなら、効果を適用せず現在項目を終了する。
196の消費・配布予定は残し、次の予約項目へ進む。194の入金・請求も起こらない。
HP0でもdead=falseの暫定状態なら、死亡先取り消しを適用せず応答を続ける。

`StepResult::any_reflected` と汎用 `reflected` がtrueになる。
既に起動したcounterの死亡先返却には `counter_cancelled` を立て、`counter_resolved` はfalse。
未起動queueの死亡target skipとは区別する。
取り消し後に次の187が命中処理を行ったり、193が自動解決/skipされたりする場合は、その同じstepへ別の件数を返す。

ベンチは次の収支を検証する。

```text
counter予約 = 起動 + 未起動死亡skip + miss + queue残数
counter起動 = 外部効果解決 + 自動解決 + 死亡先返却取消 + current残数
```

反射で応答回数が増えるため、counter防御決定数とcounter効果解決数を同一視しない。

## 固定容量・復元

`Config::any_reflection` はデフォルトfalseで、reflectionを必要とする。
奇跡・属性・counter等の他拡張は、それぞれの依存関係に従い個別に選べる。
保持castを追跡するretained_dealsは本flagでも有効にし、防御奇跡なしでは座席ごとの上限1、
防御奇跡ありでは既存のMPに基づく保守上限20を維持する。
193は反射の応答へ出ず、元MAGIC連鎖の途中で保持防御奇跡のMPを補給する経路はない。

counterの生成は両者の手札消費以内なので、queue32枠とint64の威力上限を維持する。
反射そのものは倍率を増やさない。新しいcounter予約だけが既存の威力倍率を適用する。

Snapshot共通versionは36。同じビルド・Config内のfixture用で、旧形式を自動移行する永続checkpointではない。
未起動queueはsource=originのまま、current counterはoriginと反射回数の偶奇からsourceを検証する。
currentの187命中記録は現在sourceでなく、起動時のorigin/元targetと照合する。
current193を復元可能にはしない。

counterの消費予算をoriginへ課金し、現在項目の196使用枚数を各座席へ別に課金する。
生成数とcurrent反射数の合計が物理消費枚数を超えるsnapshot、反射された状態を未起動queueへ入れたsnapshot、
型・属性・奇跡・状態・命中来歴や死亡先応答の不整合を拒否する。

## 観測・API

共通C++観測は1345次元、gu出力は先頭713次元。v17では末尾632特徴が0。gzは先頭696、gy以前も従来の幅を維持する。
既存の反応履歴、反射回数、current counterのorigin、命中記録の位置を維持する。

| 位置 | 内容 |
| --- | --- |
| 696 | any_reflection有効なら1 |
| 697〜712 | 自分の各手札位置が196なら1 |

追加17特徴は旧profileでは0。相手の未使用手札・完全な保持数・未来の抽選値・乱数状態は含めない。
既存の威力・反応回数の正規化には1を超える値があり、全特徴が0〜1とは仮定しない。

新C ABI [any_reflection_c_api.h](../include/godfield/any_reflection_c_api.h) は `gu_*` version17。
行動はgx/gy/gzと同じ16byte、mask幅11、手札容量16。
候補は既存reaction_slotsへ入れ、反射の先頭条件・195・Glory・counter順序を `legal()` で同時に検証する。

[Pythonラッパー](../python/any_reflection_env.py)では `AnyReflectionBatchEnv` を使う。

```sh
python3 python/any_reflection_env.py
./build/godfield_legacy_bench 100000 1 42 any-reflection
./build/godfield_legacy_bench 100000 4 42 any-reflection
```

全active行の事前検証で原子的に拒否する。終了行は固定・報酬0で不正手を無視し、自動resetしない。
報酬は固定座席順。Pythonの返却バッファーは再使用するため、履歴へ保存するときはコピーする。
同一ハンドルの同時操作はしない。既存gl〜gkは6byte、gx/gy/gzは16byteで各観測幅も維持する。

霧188・幻覚189、追加魔法・忘却・守護神・蘇生・死亡時効果・終末・複数人戦、
旧版Pythonとの動的差分実行と現行公式照合、学習器・学習済みモデルは未対応。衝突C01〜C05も残す。

## 検証結果（2026-10-31）

通常CTest52件、ASan/UBSan35件、17種類のPython suite計154テストが通過。
全7属性・命中済み％・通常回復/MP/解除・奇跡・counterの返却、prefixとGlory、
元所有者と現在sourceの区別、194受益者交替、187の無再抽選、死者への返却取消とFIFO継続を検証した。
193の自己auto/死亡skip、Heaven暫定HP0からの回復、保持210との混合連鎖、
最小Configの32反射・配布予定17、counter予算と不正復元、旧ABI境界、1000試合replayも確認した。

全機能有効・10万試合・seed42・意思決定上限512で、1/4スレッドの全統計とchecksumが一致した。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 3,399,965 |
| 座席0 / 座席1勝利 | 49,343 / 50,054 |
| 勝者なし / 打ち切り | 603 / 0 |
| 万能反射 | 5,326 |
| 武器 / 奇跡 / counter / 通常utilityの万能反射 | 4,892 / 360 / 74 / 0 |
| 195前置を伴う万能反射 | 143 |
| 反撃予約 / 起動 / 未起動死亡skip / miss | 17,541 / 16,556 / 302 / 683 |
| 外部反撃効果解決 / 自動自己MP解決 / 死亡先返却取消 | 13,945 / 2,609 / 2 |
| 反撃への防御決定 | 14,019 |
| 未処理キュー / 未解決反撃 | 0 / 0 |
| checksum v17 | 12144784059662735881 |

万能反射の内訳はcounter、MAGIC、それ以外のdamage武器、それ以外の通常utilityの排他的分類。
回復奇跡の反射はMAGICへ数える。通常utilityの返却はfixtureで検証し、このランダム方策の10万試合では発生しなかった。
196を既存の武器反射・魔法反射4種へ重ねて数えず、無効になるprefix減算を実ダメージ軽減として集計しない。
取消後の次queue処理を同stepへ計上しつつ、取り消した194の入金/請求を計上しない。

checksum v17は万能反射回数・元payload・元所有者・現在source/target・prefix・取消の履歴を含める。
旧16profileのchecksum式を維持し、各10万試合で意思決定数・勝敗・draw・打ち切り・checksumが以前と一致した。
ASan/UBSanのベンチ1万試合でもReleaseの全統計とchecksumが一致した。
速度出力は行動生成・合法性検証・遷移・統計・resetを含み、観測転送・モデル推論を含まない。

共通観測の新しい追加部分は[HP吸収v28](legacy-hp-absorption-profile.md)を参照。旧ABIの幅・行動形式は維持する。
