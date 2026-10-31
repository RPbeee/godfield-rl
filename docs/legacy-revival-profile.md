# 死亡確定前の自動蘇生 v26

`legacy-revival-subset-v26` はSUNDRY244のREVIVEを扱う限定profile。
CLIの `revival` と `g26_*` は全拡張を有効にし、207種・旧重み合計419で動く。
現行公式の完全再現ではなく、旧非公式Flashサーバーを参考にした部分実装である。

[一次処理の調査](legacy-revival-rules.md)、[台帳・出典・hash](../data/legacy-revival-v1/README.md)、
[前段の自己交換profile](legacy-exchange-profile.md)を参照。
既存25台帳のhashと保持indexを固定し、対象プール内で旧重みを正規化する。公式の配布確率とは扱わない。

## 244の自動消費

| 旧ID / ローカルindex | 原典type / extra | 発動時の回復 | 旧価格 / 重み |
| --- | --- | --- | --- |
| 244 / 0 | SUNDRY / REVIVE | HP+10 | 10 / 1 |

普通ATK・追加ATK・DEF・HP/MP回復用の能動値・MP費用・命中率・病気/harmsはない。
普通Cardのhpは0、別のpassive amountは10とし、通常の回復品へ分類しない。
通常use、武器への追加、保持cast、FREE carrierとしては使えない。手動discardは可能。
botの「捨てない」方策を合法性に混ぜない。

itemまたはowner病気tickの終了時、固定seat0→1でHP0・dead=falseを走査する。
その時点でnot.dead人数が2以上あり、対象が手札244を持てば、最初の1枚を直接消費してHP10へ戻す。
まだdeadにしていないため、そのflagはfalseのまま。既にdeadとなった人物を戻す機能ではない。
蘇生自体は乱数・新決定・相手の応答・反射・Glory枚数検査・MP支払・保持化を使わない。
**補充予定を増やさず**、MP/YEN・病気・harms・worseChanceも変えない。
以後のqueue、病気、配布は通常どおり処理する。

1回の走査では1人1枚だけ。複数244を持つ場合は、後の別の死亡境界で再び使える。
同じstepでitem終了時に1枚、続く病気死亡でさらに1枚が使われる場合もある。

## 固定座席順と反応境界

HP正の人数やシャッフルされた手番順で判定してはいけない。
両者HP0/dead=falseで同時に走査を始めた場合、次の非対称がある。

| 244の所持 | 結果 |
| --- | --- |
| なし | 両者dead |
| seat0だけ | seat0はHP10、seat1はdead |
| seat1だけ | seat0が先にdeadとなり、seat1の244も不発。両者dead |
| 両者 | 両者HP10・dead=false |

反射chain中の暫定HP0にはまだ発動せず、itemが完了するまで待つ。
その間に回復できれば244を温存する。175でHeavenから落ちた場合もこの境界を守る。
item終了の死亡走査は次の予約counterより前なので、蘇生したHP10で次itemを受ける。
owner病気でHP0ならもう一度走査するが、同じ病気tickを繰り返さない。
Heavenで蘇生後HP15になる通常継続回復と、Heaven悪化で死亡→蘇生後HP10で止まる場合を区別する。

117のDYING_ATTACKは追加しない。244が発動すれば死亡走査がcontinueして117を呼ばず、
244が発動しなければ先にdead化され、duelの残alive人数は1以下になる。
その後の「2人以上」という117発火条件は満たせず、244追加後も死亡時効果は到達不能である。

## HP0交換と統計

交換のHP0配分でも、死亡確定前に244を使えれば即敗北を避けられる。
例えば3資源合計70をHP0/MP50/YEN20へ配分し、その後244でHP10へ戻ると、最終資源合計は80になる。
交換自体の総和保存は成立しており、増えた10は別の神器による回復である。
病気tickで再度HP0になる場合は、さらに244を使うか、そこで死亡する。

ベンチはStepResultの座席別発動数を集計し、永続するLastRevivalを重複計上しない。
奇跡の実HP変化や交換の資源配分へ蘇生10を混ぜず、自然tickとも分ける。
v26のexchange_suicidesは244が発動しなかったHP0配分だけを数え、発動したHP0配分はexchange_revival_stepsへ分ける。
後者は「そのstepで244が発動した」という件数で、後続の病気死亡まで常に生存するという意味ではない。
旧25profileの既存統計・checksum計算式は維持する。

## 状態・観測・API

`Config::revival` はデフォルトfalseで、他の拡張を必要としない。
`StepResult::revivals[2]` は各座席でそのstepに消費した244の枚数、`State::last_revival` は最後のID/seatを表す。
共通Snapshotはversion36。同じビルド・Config内のfixture用であり、旧形式の永続checkpoint移行はしない。

共通C++観測は1345次元、`g26_*` は先頭1062次元。v26では末尾283特徴が0。旧 `gg_*` は1043、gj以前も従来の出力幅を維持する。
v25では末尾302特徴が0。相手の未使用244を含む手札・非公開保持数・乱数やfixture prefixは公開しない。
発動済みの最後の座席だけは公開情報として返す。

| 位置 | 内容 |
| --- | --- |
| 1043 | revival有効なら1 |
| 1044〜1059 | 自分の手札が244なら1、その他0 |
| 1060 | 最後の蘇生記録があれば1 |
| 1061 | 最後の蘇生座席が自分なら1 |

新C ABI [revival_c_api.h](../include/godfield/revival_c_api.h) は `g26_*` version26。
行動はggと同じ88byte・kind0〜14・最大65参照・tail@17、maskも25語で位置を保つ。
自動蘇生専用のaction kindやmaskを増やさない。既存discard maskで手動廃棄を指定できる。
複合条件は `legal()` で検証し、予約byte・未使用参照・他kindの追加fieldは0にする。

[Pythonラッパー](../python/revival_env.py)は `RevivalBatchEnv` を提供する。
batchは全active行を検証してから更新し、1行でも不正なら原子的に拒否する。
終了行は固定・報酬0で、不正手を無視し自動resetしない。報酬は固定座席順。
Python返却バッファーを保存するときはコピーし、同一ハンドルを同時操作しない。

```sh
python3 python/revival_env.py
./build/godfield_legacy_bench 100000 1 42 revival
./build/godfield_legacy_bench 100000 4 42 revival
```

117死亡時攻撃・売買・HP吸収・手札除去・HP増減賭け・霧/幻覚・守護神・終末・複数人戦、
旧serverの負値/99超交換や取得順保持リスト、現行公式との動的照合、学習器・学習済みモデルは未対応。
衝突C01〜C05も残す。受入検証は固定seat順、最初の244消費/補充無し、複数回発動、病気/Heaven、
counter/反射/HP0交換の境界、復元・秘匿・旧ABI境界・batch原子性と旧25profile互換を対象とする。

## 検証結果（2026-10-31）

通常CTest80件、ASan/UBSan54件、26種類のPython suite計264テストが通過。

固定seat順と同時HP0の非対称、最初の244消費と補充無し、手動discard/通常use拒否、
itemと病気の二重境界、Heaven/反射/counter/HP0交換、復元・秘匿・旧ABI境界を検証した。
193の蘇生後MP加算と死亡済みtargetのskip、194の死亡済みsourceへの入金と最後の生者の蘇生不発、
196の死亡済みsourceへの返却取消しと蘇生済みsourceへの応答継続も固定fixtureで確認した。
既存25profileは各10万試合で、従来の全192項目の決定的出力が基準と一致した。

全機能有効・10万試合・seed42・意思決定上限512で、1/4スレッドの全200項目の決定的出力が一致した。
ASan/UBSanのRelWithDebInfoビルドでも1万試合の全200項目がReleaseと一致した
（checksum version26: `88621185523842625`）。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 2,867,450 |
| 座席0 / 座席1勝利 | 49,666 / 49,775 |
| 勝者なし / 打ち切り | 559 / 0 |
| 自動蘇生の発動数 | 1,245 |
| seat0 / seat1での発動数 | 616 / 629 |
| 蘇生を含むstep数 | 1,245 |
| 同step複数回 / 両seat発動 | 0 / 0 |
| 蘇生によるHP回復合計 | 12,450 |
| HP0交換で244が発動したstep数 | 58 |
| checksum version26 | 11094869395297990504 |

自然runでは同step複数回・両seat同時発動は0だったが、固定fixtureで別途検証した。
発動数は手札消費枚数と一致し、実HP差から蘇生分を分けて奇跡/交換/自然tickを集計した。

共通観測の新しい23特徴と補償なしの手札除去は[HP吸収v28](legacy-hp-absorption-profile.md)を参照。v26のAPIと機能範囲は維持する。
