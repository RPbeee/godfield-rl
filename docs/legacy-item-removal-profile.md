# 補償なしの手札除去 v27

`legacy-item-removal-subset-v27` はSUNDRY240のREMOVE_ITEMSを扱う限定profile。
CLIの `item-removal` と `g27_*` は全拡張を有効にし、208種・旧重み合計422で動く。
現行公式の完全再現ではなく、旧非公式Flashサーバーを参考にした部分実装である。

[一次処理の調査](legacy-item-removal-rules.md)、[台帳・出典・hash](../data/legacy-item-removal-v1/README.md)、
[前段の自動蘇生profile](legacy-revival-profile.md)を参照。
既存26台帳のhashと保持indexを固定し、対象プール内で旧重みを正規化する。公式の配布確率とは扱わない。

## 240の使用と抽選

| 旧ID / ローカルindex | 原典type / 効果 | 値 | MP | 旧価格 / 重み |
| --- | --- | --- | --- | --- |
| 240 / 0 | SUNDRY / REMOVE_ITEMS | 手札コピー1枚を除去 | 0 | 10 / 3 |

普通ATK・追加ATK・DEF・HP/MP変化、病気/harms、他の特殊防御はない。捨てられる普通神器であり、保持奇跡ではない。
重み0のCREATURE249はプールへ入れない。既存use（kind3）で相手か自分を指定し、単独使用だけを認める。
手札240を消費して使用者の配布予定+1を加え、解決時のtarget手札から実コピー1枚を一様抽選して直接除去する。
同じIDが複数あれば、その種類の確率はコピー数に比例する。残った手札の順序は保つ。

除去された対象には**補充予定を与えない**。未使用MAGICが消えても保持化・MP支払はなく、既存の保持奇跡は減らさない。
普通の捨て可否にかかわらず、選ばれた武器や244も除去できる。値1を複数回抽選のループへ一般化しない。

自己使用は元240を先に消費するため、その元コピーは抽選対象に含まれない。
240だけの手札から自己使用すれば、選択時の手札は空となる。
空なら何も消さず選択RNGも消費しない。非空なら1枚だけでも選択RNGまたはticketを1件消費する。
通常の病気tickや配布によるRNG消費は、その選択処理とは別である。

`script_item_removal_tickets` はuint64 prefixを与え、その時点の手札数で剰余を取ったslotを選ぶ。
prefix終了後はseed付き抽選へ戻る。手札は実際のslot順を保持するので、忘却の取得順リストとは違い、
重複コピーを表すための別の保持履歴は不要。ただし旧Pythonと同じ乱数bit消費までは主張しない。

## 属性Noneと196反射

CSVの属性列は空だが、旧queueAttackは通常属性代入の前にこの効果で終了する。
実行中payloadは無属性ではなくNoneで、普通防具・195・WEAPON/MAGIC用の反射/遮断・210/215は使えない。
先頭196だけが属性検査の特例で反射できる。普通prefixを196より前へ置く入力は拒否する。
自分への使用は即解決し、相手への使用には空応答かこの反射がある。

反射時には196を消費してsource/targetを交換し、手札抽選はまだ行わない。
最終targetが受け入れた時点の手札から抽選するため、途中の反射で消費した196を選び直すことはない。
kind13の196→FREE suffixは従来どおり未消費で、最終targetの手札に残っていれば除去対象になり得る。
元240の費用や配布予定を反射のたびに再実行しない。

終端では既に蓄積した通常の配布予定を配るので、同じstepで手札が補充される場合がある。
それは除去への補償ではない。ベンチはstep前の手札を保存し、自己240を除外した抽選population/slot/IDを検証する。
補充後の手札枚数差だけから削除を推論しない。

## 状態・観測・API

`Config::item_removal` はデフォルトfalseで、他の拡張を必要としない。
`Pending::item_removal_card` が元240、`State::last_item_removal` が最後のsource/target・削除ID・削除前枚数/slotを表す。
削除前の手札枚数とslotは内部値で、公開イベントはsource/targetと削除IDまたは空結果だけである。
共通Snapshotはversion35。同じビルド・Config内のfixture用であり、旧形式の永続checkpoint移行はしない。

共通C++観測は1317次元、`g27_*` は先頭1085次元。v27では末尾232特徴が0。旧 `g26_*` は1062、gg以前も従来の出力幅を維持する。
v26では末尾255特徴が0。相手の残り手札内容・選択slot/ticket・内部population・乱数やfixture prefixは追加しない。

| 位置 | 内容 |
| --- | --- |
| 1062 | item_removal有効なら1 |
| 1063〜1078 | 自分の手札が240なら1、その他0 |
| 1079 | 現在の保留itemが手札除去なら1 |
| 1080 | 最後の手札除去試行があれば1 |
| 1081 | 最後のsourceが自分なら1 |
| 1082 | 最後のtargetが自分なら1 |
| 1083 | 最後に除去したID / 363、空なら0 |
| 1084 | 最後に1枚を除去したなら1 |

新C ABI [item_removal_c_api.h](../include/godfield/item_removal_c_api.h) は `g27_*` version27。
行動は既存88byte・kind0〜14・最大65参照・tail@17を保ち、新しいaction kindは増やさない。
mask幅は26。旧25語の位置を保ち、word25は手札240を表す。複合条件は `legal()` で検証する。
予約byte・未使用参照・他kindの追加fieldは0にする。

[Pythonラッパー](../python/item_removal_env.py)は `ItemRemovalBatchEnv` と手札除去ticket fixtureを提供する。
batchは全active行を検証してから更新し、1行でも不正なら原子的に拒否する。
終了行は固定・報酬0で、不正手を無視し自動resetしない。報酬は固定座席順。
Python返却バッファーを保存するときはコピーし、同一ハンドルを同時操作しない。

```sh
python3 python/item_removal_env.py
./build/godfield_legacy_bench 100000 1 42 item-removal
./build/godfield_legacy_bench 100000 4 42 item-removal
```

MORTAR245/PESTLE99・117死亡時攻撃・売買・HP吸収・HP増減賭け・霧/幻覚・守護神・終末・複数人戦、
旧serverの負値/99超交換や取得順保持リスト、現行公式との動的照合、学習器・学習済みモデルは未対応。
衝突C01〜C05も残す。受入検証はコピー抽選/空/1枚、自己使用と196往復、対象補充無し・未使用MAGIC/244除去、
復元・秘匿・旧ABI境界・batch原子性と旧26profile互換を対象とする。

## 検証結果（2026-10-31）

通常CTest83件、ASan/UBSan56件、27種類のPython suite計277テストが通過。
空/1枚/重複コピーの抽選、自己240の先行消費、196往復後の最終手札抽選、対象の補充無し、
未使用MAGIC/244除去、乱数/ticket消費、payload改竄拒否、snapshot・秘匿・旧ABI境界とbatch原子性を検証した。
既存26profileは各10万試合で、従来の全200項目の決定的出力が基準と一致した。

全機能有効・10万試合・seed42・意思決定上限512で、1/4スレッドの全212項目の決定的出力が一致した。
ASan/UBSanのRelWithDebInfoビルドでも1万試合の全212項目がReleaseと一致した
（checksum version27: `17059262411706518094`）。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 2,893,474 |
| 座席0 / 座席1勝利 | 49,691 / 49,730 |
| 勝者なし / 打ち切り | 579 / 0 |
| 手札除去神器の使用 / 解決試行 | 11,369 / 11,369 |
| 除去したコピー / 空の対象 | 11,277 / 92 |
| 手札除去への196反射 | 66 |
| 除去した武器 / 防具 | 5,974 / 2,882 |
| 除去した未使用奇跡 / その他神器 | 404 / 2,017 |
| 上記に含まれる244 | 25 |
| checksum version27 | 18194669591951025959 |

削除前の手札copyから選択slot/IDを検証し、同stepで行われる通常補充後の枚数差から削除数を推測しない。
統計はStepResultから計上し、LastItemRemovalの永続記録を重複して数えない。
ベンチchecksumは内部の削除前枚数/slotも含むが、RL観測には渡さない。

共通観測の新しい25特徴と221保持slotは[HP吸収v28](legacy-hp-absorption-profile.md)を参照。v27のAPIと機能範囲は維持する。
