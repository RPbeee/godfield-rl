# 倍化奇跡と順序付き攻撃 v21

`legacy-doubling-miracles-subset-v21` は追加専用の倍化奇跡214を扱う限定profile。
CLIの `doubling-miracles` と `gq_*` は全拡張を有効にし、198種・旧重み合計388で動く。
現行公式の完全再現ではなく、旧非公式Flashサーバーを参考にした部分実装である。

[一次処理の調査](legacy-additional-magic-rules.md)、[台帳・出典・hash](../data/legacy-doubling-miracles-v1/README.md)、
[前段の追加ATK奇跡profile](legacy-additive-miracles-profile.md)を参照。
既存20台帳のhashを固定し、対象プール内で旧重みを正規化する。公式の配布確率とは扱わない。

## 214の使用と保持

| 旧ID | 属性 | 効果 | 毎回のMP | 旧価格 / 重み | 新ローカル保持slot |
| --- | --- | --- | --- | --- | --- |
| 214 | 無 | その時点のraw威力を2倍 | 6 | 10 / 1 | 0 |

原典はMAGIC、attack kindは空、attack extraはDOUBLE_ATK。単独ATK・普通防御・特殊防御・確率・病気/harmsはない。
単独使用やkind10の `cast_additive` では使えず、命中率0のWEAPONを基底とする `attack_ordered` の追加だけに使う。
％武器・MAGIC・SUNDRY基底への追加は拒否する。完成した攻撃はWEAPON由来のままで、MAGIC反射/210/魔法遮断へ分類替えしない。

初使用は手札を消費し、`Player::doubling_ability` の保持数を増やす。保持は別のscalarで従来の各配列/indexを変えない。
保持からは毎回MP6を払い、保持数を減らさず反復できる。手札214から初取得した後、同じ列で保持214を使うこともできる。
逆順の未保持参照、MP不足、重複した手札slot、未使用tailは、状態と乱数を変えず原子的に拒否する。
各使用は配布予定+1を加える。支払・学習・配布予定は後の防御や反射でやり直さない。

追加は到達時点のraw威力を2倍にし、その後に無属性を普通合成する。
火ATK2→214は威力4・無属性、火ATK2→214→火setter65は威力4・火となる。
普通boostと倍化の順序でも威力は変わるため、unordered maskだけには還元しない。

## MAGICAL57と上限

57を基底にすると、各追加MAGICはMP支払後に威力を残MP×2へ再設定し、その後で自身の追加効果を処理する。
開始MP10の `57 → 214` は支払後MP4、再設定8、倍化16、57自身の最終支払4でMP0になる。
214を2回使う列でも、2回目は再び残MPから再設定する。以前の倍化結果をそのまま倍化し続ける計算にはならない。

MP99では214を最大16回使える。従来の追加197/217と同じ64参照上限を維持する。
普通武器の保守的raw上界は `480 << 16`（31,457,280）、32反撃までの保守的上界は
`480 << 48`（135,107,988,821,114,880）で、後者はint64へ収まる。
57自身の上界は597。57→214で残MP93を2倍して372、その後15枚の+15を足すと597になる。
MP費用は追加ATK・倍化・57自身の最終消費を別集計し、その合計が実際の消費と一致する。

## 状態・観測・API

`Config::doubling_miracles` はデフォルトfalseで、miraclesとadditive_miraclesを必要とする。
`State::attack_composition.initial_doubling` に合成開始時の保持数も記録し、復元時に合成を再実行する。
共通Snapshotはversion37。同じビルド・Config内のfixture用であり、旧形式の永続checkpoint移行はしない。

共通C++観測は1368次元、`gq_*` は先頭924次元。v21では末尾444特徴が0。旧 `gp_*` は903、gw以前も従来の出力幅を維持する。
v20では末尾465特徴が0。相手の未使用手札・非公開保持数・未来の抽選・乱数状態・fixture prefixは含めない。

| 位置 | 内容 |
| --- | --- |
| 903 | doubling_miracles有効なら1 |
| 904〜919 | 自分の手札が214なら1、その他0 |
| 920 | 自分の214保持数 / `15 * max_decisions` |
| 921 | 公開合成記録内の214使用回数 / 16 |
| 922 | 現在の保留amount / `480 << 16` |
| 923 | 保留counterならamount / `480 << 48`、その他0 |

先頭903特徴も継承する。位置773の合成MP費用には197/217に加え214の支払も含む。
旧特徴の位置は変えず、既存のraw表示には元の尺度も残す。大きなraw値を旧特徴から読むと1を超え得る。
新C ABI [doubling_miracles_c_api.h](../include/godfield/doubling_miracles_c_api.h) は `gq_*` version21。
`gq_action` は `gp_action` と同じ88byteで、kind/base/slots/targetとcounter順序、extra_count/extrasの位置を維持する。
保持参照16/17は197/217のまま、新しい18だけが214を表す。手札slot0〜15は消費前の位置、同一実体は1回だけ使える。
kind10は従来どおり197/217のローカルindex0/1だけを受け付け、214を単独castできない。

mask幅は15。従来13語の位置を維持し、word13は手札214候補、word14は保持214のbit0。
maskは構成要素であり、MP・順序・同列取得・Glory等は `legal()` で全列を検証する。
予約byte・未使用extras・他kindの追加fieldは0にする。旧APIへref18を渡さない。

[Pythonラッパー](../python/doubling_miracles_env.py)は `DoublingMiraclesBatchEnv` と `Action.extras` を提供する。
batchは全active行を検証してから更新し、1行でも不正なら原子的に拒否する。
終了行は固定・報酬0で、不正手を無視し自動resetしない。報酬は固定座席順。
Python返却バッファーを保存するときはコピーし、同一ハンドルを同時操作しない。

```sh
python3 python/doubling_miracles_env.py
./build/godfield_legacy_bench 100000 1 42 doubling-miracles
./build/godfield_legacy_bench 100000 4 42 doubling-miracles
```

MAGIC_FREE・209全体化・霧/幻覚・忘却/守護神/蘇生・死亡時効果・終末・複数人戦、
現行公式との動的照合、学習器・学習済みモデルは未対応。衝突C01〜C05も残す。
受入検証は台帳、倍化順序、保持反復/同列取得、MP不足拒否、57再設定、属性/反応、上限、
復元・秘匿・旧ABI出力境界・batch原子性と旧20profile互換を対象とする。

## 検証結果（2026-10-31）

通常CTest64件、ASan/UBSan43件、21種類のPython suite計202テストが通過。
倍化の位置・保持反復/同列取得・単独拒否・費用不足の原子的拒否・属性合成、
57の再設定/上界597、raw威力16,842,752とその反撃33,685,504、
復元時の初期保持trace・秘匿・旧ABI境界を検証した。

全機能有効・10万試合・seed42・意思決定上限512で、1/4スレッドの全145項目の決定的出力が一致した。
ASan/UBSanのRelWithDebInfoビルドでも1万試合の全145項目がReleaseと一致した
（checksum version21: `13200587834190989706`）。
既存20profileも各10万試合で、従来の全139項目の決定的出力が基準と一致した。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 3,260,908 |
| 座席0 / 座席1勝利 | 49,853 / 49,484 |
| 勝者なし / 打ち切り | 663 / 0 |
| 倍化の初使用 / 保持再使用 | 5,339 / 3,888 |
| 倍化のMP支払 / 新規保持 | 55,362 / 5,339 |
| 倍化を含む攻撃 / 倍化使用数 | 7,561 / 9,227 |
| 順序付き攻撃 / 追加参照数 | 182,414 / 212,075 |
| checksum version21 | 4021232563286823743 |

新しい倍化統計を追加ATK奇跡の統計と分け、奇跡全体の費用・保持数には両方を含める。
raw威力やcounter理論最大額を統計へ加算せず、実資源差・用途別の使用数と上界内の57威力を集計する。

共通観測の新しい21特徴と保持参照19は[HP吸収v28](legacy-hp-absorption-profile.md)を参照。v21のAPIと機能範囲は維持する。
