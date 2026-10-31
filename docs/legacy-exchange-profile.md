# HP・MP・YENの自己交換 v25

`legacy-exchange-subset-v25` はTRADE神器2のEXCHANGEを扱う限定profile。
CLIの `exchange` と `gg_*` は全拡張を有効にし、206種・旧重み合計418で動く。
現行公式の完全再現ではなく、旧非公式Flashサーバーを参考にした部分実装である。

[一次処理の調査](legacy-exchange-rules.md)、[台帳・出典・hash](../data/legacy-exchange-v1/README.md)、
[前段の忘却profile](legacy-forgetting-profile.md)を参照。
既存24台帳のhashと保持indexを固定し、対象プール内で旧重みを正規化する。公式の配布確率とは扱わない。

## 交換2と配分の範囲

| 旧ID / ローカルindex | 原典type / 効果 | 旧価格 / 重み |
| --- | --- | --- |
| 2 / 0 | TRADE / EXCHANGE | 10 / 20 |

普通ATK・追加ATK・DEF・HP/MP回復値・MP費用・命中率・病気/harmsはない。
`Kind::trade=5` を末尾に追加し、既存5値を移動しない。捨てられる普通神器であり、奇跡として保持しない。
手札2を消費して配布予定+1を得る。価格10は売買用metadataであり、交換の手数料ではない。
SELL3/BUY4は今回追加しない。

交換先のHP/MP/YENをそれぞれ0〜99で指定し、3値の合計が使用直前の3資源合計と一致することを要求する。
差分の足し引きではなく、要求した3値へ同時に再配分する。例えばHP40/MP10/YEN20からHP30/MP30/YEN10へ交換できる。
全て現在値のままでも合法だが、神器の消費・配布予定・1決定と自然な手番進行は処理する。
配分先のHP0も合法で、他の座席が生存していれば即敗北になる。使用直前はHP1以上の生存座席を必要とする。HP1以上へ自動補正しない。

旧serverは整数化と総和一致だけを調べ、負値や99超、HPの最低1を検証しない。
この環境は各0〜99に限定したcanonical subsetであり、その未検証値を受け付ける旧server挙動までは再現しない。
公式UIの配分範囲は確認できていない。botのHP30/MP30等の方策は合法性の根拠にしない。

## 応答と自然tick

原典は `isAction` にして自分へ強制し、資源を直接代入する。交換はその場で解決し、相手の応答や反射を作らない。
武器/MAGIC由来にはならず、追加品・MAGIC_FREE・無料cast・保持castにはできない。
MP費用、保持化、命中抽選、反撃予約もない。

配分後は従来どおり通常手番の病気tickと終局判定を行う。
`LastExchange.before/after` と `StepResult.exchange_before/after` は病気tick前の資源配分を表す。
最終HPだけから交換のHP差を計算すると病気ダメージ/回復を混ぜてしまうため、配分と自然tickを別集計する。
例えば配分でHPを増やしてからColdで1失った場合、その1は交換の消費ではない。

## 行動・状態・観測

`Config::exchanges` はデフォルトfalseで、他の拡張を必要としない。
新 `exchange`（kind14）を使い、baseは手札2のslot、targetは255、slots/counter_count/counter_orderは0にする。
extra_countは3、extras[0..2]は順に新HP/MP/YEN。これらは追加品参照ではなく配分値である。
残りのextras・extra_tail・予約byteは0にする。通常のuse（kind3）へ交換2を渡して配分を省略することはできない。
全体を事前検証し、合計不一致・範囲外・不正fieldは状態と乱数を変えず拒否する。

`State::last_exchange` に神器ID・実行座席・配分前後の3値を記録する。現在の保留effectは新設しない。
共通Snapshotはversion37。同じビルド・Config内のfixture用であり、旧形式の永続checkpoint移行はしない。

共通C++観測は1368次元、`gg_*` は先頭1043次元。v25では末尾325特徴が0。旧 `gj_*` は1017、gi以前も従来の出力幅を維持する。
v24では末尾351特徴が0。交換の前後資源は公開情報だが、相手の手札・保持数・乱数やfixture prefixは公開しない。

| 位置 | 内容 |
| --- | --- |
| 1017 | exchanges有効なら1 |
| 1018〜1033 | 自分の手札が交換2なら1、その他0 |
| 1034 | 最後の交換記録があれば1 |
| 1035 | 最後の交換座席が自分なら1 |
| 1036〜1038 | 交換前HP/MP/YEN、それぞれ / 99 |
| 1039〜1041 | 交換後HP/MP/YEN、それぞれ / 99。自然tick前 |
| 1042 | 交換後3資源の合計 / 297 |

旧特徴位置を維持する。従来の手札kind尺度 `/3` も残すため、TRADEの値5はその特徴で1を超える。
新しい手札交換flagと対応maskで、交換品を明確に識別できる。

## C ABI・Python

新C ABI [exchange_c_api.h](../include/godfield/exchange_c_api.h) は `gg_*` version25。
`gg_action` はgi/gjと同じ88byteで65番目tail@17も保ち、kind0〜13の意味を変えない。
新kind14だけがextra_count3と配分3値を使う。旧APIへこのkindを渡さない。
mask幅は25。旧24語の位置を保ち、word24は交換2の手札slotを表す。合計や範囲は `legal()` で検証する。

[Pythonラッパー](../python/exchange_env.py)は `ExchangeBatchEnv` と配分3値の `Action.extras` を提供する。
batchは全active行を検証してから更新し、1行でも不正なら原子的に拒否する。
終了行は固定・報酬0で、不正手を無視し自動resetしない。報酬は固定座席順。
Python返却バッファーを保存するときはコピーし、同一ハンドルを同時操作しない。

```sh
python3 python/exchange_env.py
./build/godfield_legacy_bench 100000 1 42 exchange
./build/godfield_legacy_bench 100000 4 42 exchange
```

売買・HP吸収・手札除去・HP増減賭け・霧/幻覚・守護神/蘇生・死亡時効果・終末・複数人戦、
旧serverの負値/99超交換、公式UIの配分範囲、現行公式との動的照合、学習器・学習済みモデルは未対応。
衝突C01〜C05も残す。受入検証は保存配分、各0〜99とHP0、原子的拒否、1決定の消費/敗北、自然tick分離、
復元・秘匿・旧ABI境界・batch原子性と旧24profile互換を対象とする。

## 検証結果（2026-10-31）

通常CTest77件、ASan/UBSan52件、25種類のPython suite計251テストが通過。
合計保存・各0〜99・HP0配分と即敗北、使用前HP0の拒否、no-op時の1決定/消費/配布、
不正合計・範囲・fieldの原子的拒否、自然病気tickの分離、snapshot・公開資源/秘匿・旧ABI境界を検証した。
既存24profileは各10万試合で、従来の全181項目の決定的出力が基準と一致した。

全機能有効・10万試合・seed42・意思決定上限512で、1/4スレッドの全192項目の決定的出力が一致した。
ASan/UBSanのRelWithDebInfoビルドでも1万試合の全192項目がReleaseと一致した
（checksum version25: `2333698678240207140`）。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 2,854,370 |
| 座席0 / 座席1勝利 | 49,574 / 49,851 |
| 勝者なし / 打ち切り | 575 / 0 |
| 交換使用 / 配分変更なし | 100,908 / 51 |
| HP0配分による即敗北 | 3,094 |
| 交換によるHP増加 / 減少の合計 | 381,337 / 1,470,899 |
| 交換によるMP増加 / 減少の合計 | 1,257,226 / 337,465 |
| 交換によるYEN増加 / 減少の合計 | 810,813 / 641,012 |
| 交換後の自然tick実ダメージ / 回復 | 8,158 / 4,759 |
| checksum version25 | 3741224508493568955 |

交換イベントの配分前後で資源保存を検査し、各資源の増減を別々のuint64で集計する。
自然tickのHP変化をその配分差へ混ぜず、最終HP/MP/YENが範囲内で、交換が1決定だけを消費することも確認した。

共通観測の新しい19特徴とHP0配分後の自動蘇生は[HP吸収v28](legacy-hp-absorption-profile.md)を参照。v25のAPIと機能範囲は維持する。
