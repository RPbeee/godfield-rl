# 現在sourceへのHP吸収 v28

`legacy-hp-absorption-subset-v28` はABSORB_HPの正重量4種を扱う限定profile。
CLIの `hp-absorption` と `g28_*` は全拡張を有効にし、212種・旧重み合計426で動く。
現行公式の完全再現ではなく、旧非公式Flashサーバーを参考にした部分実装である。

[一次処理の調査](legacy-hp-absorption-rules.md)、[台帳・出典・hash](../data/legacy-hp-absorption-v1/README.md)、
[前段の手札除去profile](legacy-item-removal-profile.md)を参照。
既存27台帳のhashと保持番号を固定し、対象プール内で旧重みを正規化する。公式の配布確率とは扱わない。

## 3武器と魔法221

| 旧ID / 新ローカルindex | type | 属性 / ATK | MP | 命中率 | 旧価格 / 重み |
| --- | --- | --- | --- | --- | --- |
| 26 / 武器0 | WEAPON | 無 / 7 | 0 | 0 sentinel | 10 / 1 |
| 42 / 武器1 | WEAPON | 無 / 11 | 0 | 0 sentinel | 15 / 1 |
| 103 / 武器2 | WEAPON | 木 / 3 | 0 | 75 | 10 / 1 |
| 221 / 魔法0 | MAGIC | 光 / 10 | 10 | 0 sentinel | 15 / 1 |

追加ATK役割・DEF・病気/harms・他の特殊防御はない。武器3種は捨てられない。
0は従来の必中sentinelで、103は既存の確率武器と同様に生存敵へ1回ずつ展開する。
1対1では指定selfも相手へ正規化する。最初の命中後、反射で再抽選しない。
221は独立した奇跡台帳で、旧offensive13を変更しない。価格15はsource/normalized CSVに固定する。

## 残余全量の回復と自己対象

普通防御後のraw残余をrとして、現在sourceを先に `min(99, HP + r)` へ回復し、その後現在targetへrの被害を与える。
targetの残HPや実際のHP損失、sourceが実際に回復した量でrを制限しない。

| sourceHP / targetHP / r | source先行回復 | target被害 |
| --- | --- | --- |
| 40 / 2 / 11 | 51へ | 0へ。実損失2でもsourceは11回復 |
| 95 / 40 / 11 | 99へ。実回復4 | 29へ |
| 同じ座席HP95 / r11 | 99へ | 続いて88へ。相殺してHP95のままにはしない |

r0の完全防御では実回復0。ミス・遮断・継続中の反射ではまだ吸収を解決しない。
診断用の吸収解決eventはr0も記録するので、「解決数」と「正の回復があった回数」は区別する。
DARKの場合は原典の別分岐で回復せず999被害となるが、現在4種の合法な合成からDARK吸収へは到達しない。
この一般則を、到達不能なfixtureを通常の行動例として説明する根拠にはしない。

反射・跳ね返し後も効果を保ち、受益者は元の使用者ではなく現在sourceとなる。
特殊反応前の普通DEF減算は捨てるが、prefix消費・病気・195の属性消去は残す。
吸収自体はharmではなく、Heavenを落としたり病気を解除したりしない。
暫定HP0・dead=falseのsourceも、回復が先なら死亡確定前に救われ得る。
target被害まで完了してから244/死亡走査、残counter、owner病気tick、配布へ進む。
派生counterには吸収来歴を継承しない。

## 追加・反応と221の保持

26/42は普通boost、65/85、197/217、214、209、MAGIC後のFREEを順序付きで使え、完成した武器全体へ吸収を保つ。
103は元の率75が正なので追加品を拒否する。吸収武器自体を別baseのboostとして使うこともできない。
221は単独MAGICで、FREE carrier以外の追加を拒否する。221を武器へ追加したり、57と吸収を同じ攻撃へ混ぜたりしない。

26/42/103はWEAPON、221はMAGICのまま、それぞれ従来の属性/％/195/反射/遮断の規則を使う。
例えば木103を195で消して跳ね返し、自分をtargetにすれば自己回復→被害の順で解決できる。
221は通常防御、210、魔法反射/遮断、196の対象となり、物理20/39/215へ分類替えしない。

221初使用は手札1枚を消費し、保持を1個増やしてMP10を払う。
保持からの再使用は保持を減らさずMP10を払い、各使用は配布予定+1を得る。
FREE付き初使用/保持再使用はMP0でも可能だが、未保持の再使用を許可するものではない。
240で未使用221を除去しても保持化せず、241の忘却は取得済み221も抽選対象にする。
最後のコピーを失えば有料/無料保持castの両方を拒否し、別の手札221を使えば取り戻せる。

## 保持番号・状態・観測

`Config::hp_absorption` はデフォルトfalseでmiraclesとcelestialを必要とする。offensive_miraclesは必須ではない。
互換用の `total_miracle_count=15` と `total_retained_miracle_count=17` は固定し、物理配列だけ `main_ability_capacity=16` にする。
221は `hp_absorption_ability_index=15`。従来0〜14、防御/追加ATKの各local indexを動かさない。
有料CASTのbase15、FREE保持CASTのbase47（32+15）を追加し、uint16のbit15を使う。
旧offensiveが無効でHP吸収が有効ならmain保持の有効集合は0/1/15で、単純な先頭3件ではない。

旧 `retained_type_ids` 21種は凍結し、新headerの22種配列へ221を220と222の間に挿入する。
新flagがfalseなら221を0に固定し、旧ID順・checksumの旧15/21件loop・旧saltと抽選対応を維持する。
`Pending::hp_absorption_card` は元神器の来歴、`LastHPAbsorption` は現在source/target・r・実回復を表す。
共通Snapshotはversion36。同じビルド・Config内のfixture用であり、旧形式の永続checkpoint移行はしない。

共通C++観測は1345次元、`g28_*` は先頭1110次元。v28では末尾235特徴が0。旧 `g27_*` は1085、g26以前も従来の出力幅を維持する。
v27では末尾260特徴が0。相手の手札・非公開保持数・乱数やfixture prefixは追加しない。

| 位置 | 内容 |
| --- | --- |
| 1085 | hp_absorption有効なら1 |
| 1086〜1101 | 自分の手札の武器26/42/103・魔法221をローカル種別1〜4 / 4で表す。その他0 |
| 1102 | 自分の221保持数 / max_decisions |
| 1103 | 現在の保留吸収来歴ID / 363 |
| 1104 | 最後の吸収解決記録があれば1。r0も含む |
| 1105 | 最後のsourceが自分なら1 |
| 1106 | 最後のtargetが自分なら1 |
| 1107 | 最後の吸収神器ID / 363 |
| 1108 | 最後の防御後r / magic_free_damage_limit |
| 1109 | 最後の実回復量 / 99 |

旧13攻撃奇跡の個別保持座標305〜317は固定し、221の個別値は1102だけへ追加する。
**忘却も有効な場合、既存aggregate1016は221も含む現在の総保持数**を表す。
新profile内で221を持てばこの古い座標の値も増えるため、tail以外が常に同値とは扱わない。
旧flag=falseでは221が0なので、旧27profileの観測・乱数・統計は従来どおりである。

## C ABI・Python・統計

新C ABI [hp_absorption_c_api.h](../include/godfield/hp_absorption_c_api.h) は `g28_*` version28。
行動は88byte・kind0〜14・最大65参照・tail@17を保ち、新しいaction kindは増やさない。
mask幅は27。旧26語を保ち、word26はmain時の吸収4種の手札membership。
通常weapon/utility/castとFREEのmaskを組み合わせ、費用・対象・順序は `legal()` で検証する。
予約byte・未使用参照・他kindの追加fieldは0にする。

[Pythonラッパー](../python/hp_absorption_env.py)は `HPAbsorptionBatchEnv` を提供する。
batchは全active行を検証してから更新し、1行でも不正なら原子的に拒否する。
終了行は固定・報酬0で、不正手を無視し自動resetしない。報酬は固定座席順。
Python返却バッファーを保存するときはコピーし、同一ハンドルを同時操作しない。

```sh
python3 python/hp_absorption_env.py
./build/godfield_legacy_bench 100000 1 42 hp-absorption
./build/godfield_legacy_bench 100000 4 42 hp-absorption
```

ベンチはStepResultから吸収解決・実回復・自己対象・完全防御・99上限を集計する。
raw残余は最大値だけを記録し、巨大な理論値を総和へ加えない。
221の初使用/再使用/実MP/学習/HP被害/忘却を別記録し、旧offensive13統計へ混ぜない。
全奇跡集計には221も含め、自己魔法のtargetHP差からsource吸収回復・owner tick・244回復を分離する。

MORTAR245/PESTLE99・117死亡時攻撃・売買・HP増減賭け・霧/幻覚・守護神・終末・複数人戦、
旧serverの負値/99超交換や取得順保持リスト、現行公式との動的照合、学習器・学習済みモデルは未対応。
衝突C01〜C05も残す。受入検証はraw全量/HPclip非依存/99cap/self順序、反射と現在source、r0・miss・block、
221保持/FREE/忘却22種、追加と属性閉包、244/tick分離、復元・秘匿・旧ABI境界・旧27profile互換を対象とする。

## 検証結果（2026-10-31）

通常CTest86件、ASan/UBSan58件、28種類のPython suite計290テストが通過。
正残余全量・targetHPclip非依存・99上限・self非相殺、完全防御0・103のhit/missと195後selfbounce、
221の初使用/重複/保持/FREE/忘却22種、Heaven暫定HP0からの210selfbounceと244有無、
counterへの非継承、属性/追加の閉包、snapshot・秘匿・旧ABI境界を検証した。
既存27profileは各10万試合で、従来の全212項目の決定的出力が基準と一致した。

全機能有効・10万試合・seed42・意思決定上限512で、1/4スレッドの全229項目の決定的出力が一致した。
ASan/UBSanのRelWithDebInfoビルドでも1万試合の全229項目がReleaseと一致した
（checksum version28: `15910157591344157653`）。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 2,903,299 |
| 座席0 / 座席1勝利 | 49,644 / 49,805 |
| 勝者なし / 打ち切り | 551 / 0 |
| 吸収解決 / 武器 / 魔法 | 20,392 / 15,676 / 4,716 |
| 自己対象の吸収解決 | 231 |
| 残余0 / 実回復0 | 2,062 / 2,067 |
| 実回復合計 / 上限で制限された解決 | 143,457 / 17 |
| DARK解決 / 観測したraw最大値 | 0 / 224 |
| 221初使用 / 保持再使用 | 3,327 / 1,636 |
| 221実MP支払 / 新規保持 | 44,720 / 3,327 |
| 221攻撃 / target実HP損失合計 | 4,963 / 44,590 |
| 忘却した221コピー | 97 |
| checksum version28 | 335783823664705205 |

自己吸収の先行回復、自然tick、244をtarget差分から分離して、実被害と実回復を混同しない。
巨大rawは総和に加えず最大値だけを記録し、221を含む累積学習・在庫・忘却の収支も検査した。
旧main15とold21-IDのloop/saltは無効flag側で固定し、array16の末尾0を追加mixして旧checksumを変えない。
