# 杵と臼 v30

`legacy-pestle-mortar-subset-v30` はPESTLE99/MORTAR245を追加する限定profile。
CLIの `pestle-mortar` と `g30_*` は全拡張を有効にし、215種・旧重み合計429で動く。
現行公式の完全再現ではなく、旧非公式Flashサーバーを参考にした部分実装である。

[一次処理の調査](legacy-pestle-rules.md)、[台帳・出典・hash](../data/legacy-pestle-mortar-v1/README.md)、
[前段の自己被害profile](legacy-self-damage-profile.md)を参照。
既存29台帳のhashと保持番号を固定し、対象プール内で旧重みを正規化する。公式の配布確率とは扱わない。

## 武器99と受動品245

| 旧ID / 各familyのlocal index | type | 属性 / ATK | MP / 命中率 | 旧価格 / 重み |
| --- | --- | --- | --- | --- |
| 99 / 杵0 | WEAPON | 光 / 30 | 0 / 0 sentinel | 5 / 1 |
| 245 / 臼0 | SUNDRY | 普通の属性欄は空 / 0 | 0 / 0 sentinel | 0 / 1 |

両者に追加ATK役割・DEF・HP/MP回復・病気/harms・特殊防御はない。
専用台帳の所属で元のPESTLE/MORTAR extraを保つ。99は武器なので捨てられない。
245は通常使用できず、杵の検索対象になる受動品である。

99は**単独1品の攻撃だけ**を許可し、普通boost・setter・追加奇跡・FREEを付けられない。
245を99の追加品や普通防御として使うこともできない。

## 臼検索と対象選択

99消費後、固定参加順でnot.deadの座席を調べ、245を持つ最初の座席をtargetにする。
1対1では両者が所持していればseat0が選ばれる。手番人物からの循環順ではない。
この場合raw威力を999へ設定し、245は消費しない。
臼が無ければ自己も含む生存座席をランダムに選び、raw30のまま攻撃する。
入力のtargetで結果を強制しない。target選択用のfixture prefixは無臼時だけ消費する。
元の率0なので命中乱数は消費せず、対象抽選を命中抽選eventへ数えない。

最初の選択時点のsource/target・臼による固定の有無・raw30/999を公開履歴に記録する。
それ以前の相手の245所持や、選ばれなかった臼/他の手札は観測しない。
245が後で240に除去されても、既存攻撃の来歴や999を30へ戻さない。

自己を選んだ場合は応答を挟まず解決する。他者を選んだ場合は既存のresponseへ進む。
属性は光のままなので、対応する普通防御・195・特殊防御の既存合法性を使う。
反射や跳ね返し後もrawと臼来歴を保ち、再検索・再命中抽選・99再消費はしない。
**原典の臼ログは防御/反撃禁止を意味しない。** 普通防御・反撃予約・被害適用の後のreturnであり、
受理された通常/特殊応答は従来どおり処理する。

防御後raw残余をrとしてtargetへ被害を与え、実HP損失は残HPでclipする。
r0の完全防御も診断用解決eventに含める。ミス/遮断や継続中の反射は解決に含めない。
244・死亡確定・残counter・owner自然tickは既存の項目終了順を使い、派生counterへ杵/臼来歴を継承しない。

## 245の捨て選択と強制除去

245をDISCARDの選択へ含めることはできるが、**245の実消費だけをskipする**。
他に選んだ捨て可能品は通常どおり捨てる。245だけを選んだ場合も、手札不変・配布予定+0で手番を終える行動として扱う。
`Card.discardable=false` はこの実消費されない性質であり、選択自体を拒否する意味ではない。
240による強制除去はこのskipを通らず、245も1コピー除去できる。対象への補充はない。

## 状態・観測

`Config::pestle_mortar` はデフォルトfalseで、celestialだけを必要とする。
`PestleCard{Card, Element, mortar_amount}` のlocal0=99・mortar_amount999、
`MortarCard{Card}` のlocal0=245。保持配列や奇跡indexは増やさない。
`Pending::pestle_card` は99、`mortar_card` は0または245。
`LastPestle` は最初の選択履歴で、反射中の現在source/targetとは区別する。
共通Snapshotはversion35。同じビルド・Config内のfixture用であり、旧形式の永続checkpoint移行はしない。

共通C++観測は1317次元、`g30_*` は先頭1175次元。v30では末尾142特徴が0。旧 `g29_*` は1135、g28以前も従来の出力幅を維持する。
v29では末尾182特徴が0。相手の手札・非公開保持数・乱数やfixture prefixは追加しない。

| 位置 | 内容 |
| --- | --- |
| 1135 | pestle_mortar有効なら1 |
| 1136〜1151 | 自分の手札slotが99なら1 |
| 1152〜1167 | 自分の手札slotが245なら1 |
| 1168 | 現在の保留攻撃が杵由来なら1 |
| 1169 | 現在の保留攻撃が臼固定由来なら1 |
| 1170 | 最初の杵選択履歴があれば1 |
| 1171 | 最初のsourceが自分なら1 |
| 1172 | 最初のtargetが自分なら1 |
| 1173 | 最初の選択が臼による固定なら1 |
| 1174 | 最初のraw30/999 / 999 |

fixtureの `script_pestle_targets(span<uint8_t>)` は最大64個の座席0/1を受け付ける。
無臼時の生存対象だけを指定でき、臼固定時は消費しない。policy観測へprefixや位置を漏らさない。
通常の抽選はこのローカル環境の乱数であり、旧Pythonのseed→対象のbit単位一致は主張しない。
非MF/非倍化の杵有効構成ではraw基底上界999・counter上界 `999 << 32` を使用する。旧profileの480と観測分母は固定し、
倍化/MFを含む全機能構成では既存のより大きいraw/counter上界を保つ。

## C ABI・Python・統計

新C ABI [pestle_mortar_c_api.h](../include/godfield/pestle_mortar_c_api.h) は `g30_*` version30。
行動は88byte・kind0〜14・最大65参照・tail@17を保ち、新しいaction kindは増やさない。
mask幅は30。旧28語を保ち、word28はmain時の99手札membership、word29は245手札membership。
245の選択可否と実消費skipを区別し、組み合わせは `legal()` で検証する。
予約byte・未使用参照・他kindの追加fieldは0にする。

[Pythonラッパー](../python/pestle_mortar_env.py)は `PestleMortarBatchEnv` を提供する。
batchは全active行を検証してから更新し、1行でも不正なら原子的に拒否する。
終了行は固定・報酬0で、不正手を無視し自動resetしない。報酬は固定座席順。
返却バッファーの保存時はコピーし、同一ハンドルを同時操作しない。

```sh
python3 python/pestle_mortar_env.py
./build/godfield_legacy_bench 100000 1 42 pestle-mortar
./build/godfield_legacy_bench 100000 4 42 pestle-mortar
```

ベンチはStepResultから初期選択・臼固定/無臼・自己対象・解決残余・実HP損失・反応・244同stepを集計する。
245の捨てskipと240の強制除去も別記録する。rawは最大値だけを記録し巨大な理論値を総和へ加えない。
checksum version30だけへ新event履歴・pending99/245・最初の選択履歴を混ぜ、旧29までの式と保持loopを固定する。

117死亡時攻撃・売買・HP増減賭け・霧/幻覚・守護神・終末・複数人戦、
旧serverの負値/99超交換や取得順保持リスト、現行公式との動的照合、学習器・学習済みモデルは未対応。
衝突C01〜C05も残す。受入検証は固定所持者順/無臼乱数/self/保持非消費、反射後の不変来歴、
普通/特殊応答・反撃・244/自然tick、245捨てskipと240、復元・秘匿・旧ABI境界・旧29profile互換を対象とする。

## 検証結果（2026-11-01）

通常CTest92件、ASan/UBSan62件、30種類のPython suite計319テストが通過。
臼0/複数/両seat/既dead/暫定HP0と固定検索順、無臼self含む抽選、単独列・245非消費、
普通防御/195/特殊/反撃、反射後来歴、244と自然tick、245捨てskip/240強制除去、
snapshot・秘匿・旧ABI境界と1000試合のrolloutを固定fixtureと合わせて検証した。
既存29profileは各10万試合で、従来の全241項目の決定的出力が基準と一致した。
全機能有効・10万試合・seed42・意思決定上限512で、1/4スレッドの全257項目の決定的出力が一致した。
ASan/UBSanのRelWithDebInfoビルドでも1万試合の全257項目がReleaseと一致した
（checksum version30: `17602264282388587421`）。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 2,815,542 |
| 座席0 / 座席1勝利 | 49,725 / 49,419 |
| 勝者なし / 打ち切り | 856 / 0 |
| 杵攻撃 / 臼固定 / 無臼抽選 | 5,540 / 409 / 5,131 |
| 初期targetが自己 / 相手 | 2,755 / 2,785 |
| 杵解決 / 解決時self | 5,540 / 2,756 |
| 残余0 / target実HP損失合計 | 1 / 133,267 |
| targetのHPclip / 観測したraw最大値 | 2,786 / 999 |
| 杵payloadの反射/跳ね返し / 遮断 | 24 / 0 |
| 解決と244発動が同じstep | 46 |
| 245の捨て実消費skip | 20,163 |
| 240が除去した245コピー | 92 |
| checksum version30 | 7284010896953083478 |

初期の臼検索/ランダム選択と、反射後の現在source/targetを分けて検査した。
防御後残余とHPclip、244/自然tickを別の値として扱い、巨大rawは総和に加えず最大値だけを記録した。
通常runの遮断0は未対応を意味せず、固定fixtureで属性消去後の特殊反応を検証した。
