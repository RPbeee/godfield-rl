# 現在sourceの自己被害 v29

`legacy-self-damage-subset-v29` はDAMAGE_TO_SELF武器53だけを追加する限定profile。
CLIの `self-damage` と `g29_*` は全拡張を有効にし、213種・旧重み合計427で動く。
現行公式の完全再現ではなく、旧非公式Flashサーバーを参考にした部分実装である。

[一次処理の調査](legacy-self-damage-rules.md)、[台帳・出典・hash](../data/legacy-self-damage-v1/README.md)、
[前段のHP吸収profile](legacy-hp-absorption-profile.md)を参照。
既存28台帳のhashと保持番号を固定し、対象プール内で旧重みを正規化する。公式の配布確率とは扱わない。

## 武器53と二段の被害

| 旧ID / 新ローカルindex | type | 属性 / ATK | MP / 命中率 | 旧価格 / 重み |
| --- | --- | --- | --- | --- |
| 53 / 0 | WEAPON | 無 / 14 | 0 / 0 sentinel | 1 / 1 |

追加ATK役割・DEF・HP/MP回復・病気/harms・特殊防御はない。武器なので捨てられない。
専用台帳の所属でDAMAGE_TO_SELFを保持し、普通のATK14武器へ効果を剥がして登録しない。

防御後raw残余をrとして、現在sourceのHPを先にr減らし、続いて現在targetのHPをr減らす。
各減算は独立にHP0で止まる。targetの残HPや実損失に合わせてsourceへの請求を制限しない。

| sourceHP / targetHP / r | source先行被害 | target被害 |
| --- | --- | --- |
| 40 / 2 / 14 | 26へ。実損失14 | 0へ。実損失2 |
| 3 / 40 / 14 | 0へ。実損失3 | 26へ。実損失14 |
| 同じ座席HP20 / r14 | 6へ。実損失14 | 続いて0へ。実損失6 |

自己対象では1回分に相殺せず、同じHPへ2回適用する。
sourceが最初の減算で暫定HP0になってもtargetへの減算まで完了する。
死亡確定・244自動蘇生・残counter・ownerの自然病気tick・配布は、その後の境界で処理する。
自己被害の記録はこの2回の適用直後で、後続の244回復や自然tickを含めない。

完全防御r0でも解決診断eventを記録し、両実損失は0。
ミス・遮断・継続中の反射では未解決なので自己被害eventを出さない。
DARKは原典の別分岐でこのextraを飛ばすが、現在合法な53の合成からDARKへは到達しない。

## 追加・属性・反応

53は普通boost、65/85、197/217、214、209、MAGIC後のFREEを既存の順序どおり使える。
完成した攻撃全体へ自己被害を保ち、209を使えば生存敵へ正規化して命中を抽選する。
元typeはWEAPONのまま、MAGIC向け反射・遮断は対象にしない。
53自体を他baseの追加boostとして使うこともできない。

20/39・215・196・195の既存反応を使う。反射や跳ね返しでは追加MPの再支払いや命中の再抽選をしない。
自己被害を受けるのは元の使用者に固定せず、解決時の現在sourceである。
prefix消費・病気・195の属性消去は残し、特殊反応時の普通DEF減算は捨てる。
派生counterへ自己被害の来歴を継承しない。

## 状態・観測

`Config::self_damage` はデフォルトfalseで、他の拡張を必須にしない。
最小構成でも両HP0を勝者なし終局として認める。deadの確定追跡は従来どおりcountersまたはrevival有効時に行う。
台帳は `SelfDamageCard{Card card;}`、local0=53。保持配列や奇跡indexを増やさない。
`Pending::self_damage_card` は元53の来歴、`LastSelfDamage` は現在source/target・r・両実損失を表す。
共通Snapshotはversion36。同じビルド・Config内のfixture用であり、旧形式の永続checkpoint移行はしない。

共通C++観測は1345次元、`g29_*` は先頭1135次元。v29では末尾210特徴が0。旧 `g28_*` は1110、g27以前も従来の出力幅を維持する。
v28では末尾235特徴が0。相手の手札・非公開保持数・乱数やfixture prefixは追加しない。

| 位置 | 内容 |
| --- | --- |
| 1110 | self_damage有効なら1 |
| 1111〜1126 | 自分の手札slotが53なら1 |
| 1127 | 現在の保留自己被害来歴ID / 363 |
| 1128 | 最後の自己被害解決記録があれば1。r0も含む |
| 1129 | 最後のsourceが自分なら1 |
| 1130 | 最後のtargetが自分なら1 |
| 1131 | 最後の自己被害神器ID / 363 |
| 1132 | 最後の防御後r / magic_free_damage_limit |
| 1133 | source先行実HP損失 / 99 |
| 1134 | targetの続く実HP損失 / 99 |

## C ABI・Python・統計

新C ABI [self_damage_c_api.h](../include/godfield/self_damage_c_api.h) は `g29_*` version29。
行動は88byte・kind0〜14・最大65参照・tail@17を保ち、新しいaction kindは増やさない。
mask幅は28。旧27語を保ち、word27はmain時の53手札membership。
普通の武器/追加/FREEのmaskを組み合わせ、費用・対象・順序は `legal()` で検証する。
予約byte・未使用参照・他kindの追加fieldは0にする。

[Pythonラッパー](../python/self_damage_env.py)は `SelfDamageBatchEnv` を提供する。
batchは全active行を検証してから更新し、1行でも不正なら原子的に拒否する。
終了行は固定・報酬0で、不正手を無視し自動resetしない。報酬は固定座席順。
返却バッファーの保存時はコピーし、同一ハンドルを同時操作しない。

```sh
python3 python/self_damage_env.py
./build/godfield_legacy_bench 100000 1 42 self-damage
./build/godfield_legacy_bench 100000 4 42 self-damage
```

ベンチはStepResultから解決・自己対象・完全防御・両実損失・HPclip・反応・244同stepを集計する。
raw残余は最大値だけを記録し、巨大な理論値を総和へ加えない。
checksum version29だけへ新event履歴・pending/last自己被害を混ぜ、旧28までの式と保持loopを固定する。

MORTAR245/PESTLE99・117死亡時攻撃・売買・HP増減賭け・霧/幻覚・守護神・終末・複数人戦、
旧serverの負値/99超交換や取得順保持リスト、現行公式との動的照合、学習器・学習済みモデルは未対応。
衝突C01〜C05も残す。受入検証はsource先行/targetHPclip非依存/self二段減算、反射後の現在source、
r0・miss・block・属性閉包、244/tick分離、復元・秘匿・旧ABI境界・旧28profile互換を対象とする。

## 検証結果（2026-11-01）

通常CTest89件、ASan/UBSan60件、29種類のPython suite計304テストが通過。
source先行・target HP clip非依存・self二段減算・反射後source交替、r0・miss・block、
WIDE/FREE/倍化・属性閉包、244と病気tickの境界、190予約後の死亡target skip、
snapshot・秘匿・旧ABI境界を固定fixtureで検証した。
既存28profileは各10万試合で、従来の全229項目の決定的出力が基準と一致した。
全機能有効・10万試合・seed42・意思決定上限512で、1/4スレッドの全241項目の決定的出力が一致した。
ASan/UBSanのRelWithDebInfoビルドでも1万試合の全241項目がReleaseと一致した
（checksum version29: `18374025819091112568`）。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 2,878,133 |
| 座席0 / 座席1勝利 | 49,763 / 49,476 |
| 勝者なし / 打ち切り | 761 / 0 |
| 53攻撃 / 自己被害解決 | 5,566 / 5,531 |
| 自己対象の解決 | 109 |
| 残余0 / source実損失0 | 387 / 387 |
| source実損失合計 / target実損失合計 | 58,937 / 58,869 |
| sourceのHPclip / targetのHPclip | 881 / 861 |
| 観測したraw最大値 | 112 |
| 自己被害payloadの反射/跳ね返し回数 | 371 |
| 解決と244発動が同じstep | 20 |
| checksum version29 | 10526022366423848945 |

両実損失はStepResultから直接集計し、自然tick・244回復を含む最終HP差へ置き換えない。
自己対象の合計損失は元HP以下で、source減算でHP0へclipした後のtarget実損失は0であることも検査した。
rawは総和に加えず最大値だけを記録し、新event履歴と来歴のchecksum混入はv29だけに限定した。
