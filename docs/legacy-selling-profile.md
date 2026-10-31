# 強制売却 v31

`legacy-selling-subset-v31` はSELL3だけを追加する限定profile。
CLIの `selling` と `g31_*` は全拡張を有効にし、216種・旧重み合計449で動く。
現行公式の完全再現ではなく、旧非公式Flashサーバーを参考にした部分実装である。

[売買の一次処理調査](legacy-trade-rules.md)、[台帳・全商品価格・出典・hash](../data/legacy-selling-v1/README.md)、
[前段の杵/臼profile](legacy-pestle-mortar-profile.md)を参照。
既存30台帳のhashと保持番号を固定し、対象プール内で旧重みを正規化する。公式の配布確率とは扱わない。

## SELL3と独立価格表

| 旧ID / 新local index | type | ATK / boost / DEF / HP / MP | 価格 / 重み |
| --- | --- | --- | --- |
| 3 / 0 | TRADE / SELL | 全0 | 5 / 20 |

MP費用・命中率・病気/harms・特殊防御はない。通常DISCARDで捨てられる。
専用台帳の所属でSELLを保つ。BUY4は配布/操作/価格表へ含めない。

新 `SalePrice{CardId id;int price;}` は旧215種と3の全216品をID順に保持する。
`price-source-rows.csv` は全216行の原列を保存し、従来の価格付きCardとも照合する。
価格fieldが無い旧奇跡型を変えず、保持可能MAGIC22種類の価格も固定する。
`max_sale_price=30` は56/186、MAGIC最大は220の25。売却3自身の商品価格は5である。
`sale_price(id)` の未知IDは-1で、価格0のMORTAR245と区別する。

## 行動・先行消費・取引順

`ActionKind::sell=15` はbaseへ手札の3を指定し、slotsへ**別の物理商品1slot**を指定する。
targetは自己または相手。counter/order・extra_count・extras・tailなどの未使用fieldは0にする。
別の3、武器・防具、245、244、未使用MAGICも商品にできる。保持能力は商品にできない。

開始時にSELL3と商品を先に手札から消し、SELL3の配布予定+1だけを得る。
商品は通常使用でも保持化でもなく直接の消費なので、武器の捨て禁止や245のDISCARD skipを通らない。
商品自身のMP費用・追加効果・244発動は発生せず、未使用MAGICを売っても学習しない。

解決時の現在targetに対し、次の順で処理する。

1. 商品価格をYENから払い、不足分をMP、さらにHPから払う。各実損失は現在量でclipする。
2. targetの現在の手札が16枚未満なら、商品IDの新しい物理コピーを末尾へ追加する。
3. 現在sourceへ商品価格を加え、YEN99でclipする。

targetが全額を払い切れなくてもsourceへの請求額は価格のまま。
**容量16で配送に失敗しても、支払い・source加算・開始時の商品消費は戻さない。**
受取による配布予定は増やさない。物理MAGICの受取は保持化せず、後で使ったときに初めて学習する。

自己対象も支払い→配送→加算の順。同じ人物でも先のMP/HP損失を後のYEN加算で相殺しない。
例えばYEN0/MP2/HP40で価格10を自己売却すれば、MP0/HP32へ減った後でYEN10を得る。
自己launchでは先に2枚消費するため配送の空きができるが、相手の満杯手札へは失敗し得る。

商品244が解決中に配送され、targetが支払いで暫定HP0になった場合、後続の死亡走査でその244を使える。
商品245の受取も通常の手札所持であり、以後の99検索対象となる。
配送・金銭処理の後に既存の244/死亡走査、残counter、owner自然tick、配布、終局へ進む。

## None payloadと反射

普通CSV属性欄は空だが、SELLのactive payloadは属性None。普通防御/195や魔法向け反応は受理しない。
先頭196は既存の特別扱いで反射できる。前置品を付けた196はNoneの適合検査を免れない。
商品と価格を保ち、反射で現在source/targetを入れ替える。商品を再消費したり保持化したりしない。
解決前の反射では支払い/配送eventを出さず、元使用者に売上を固定しない。
selfは応答無しで即解決し、他者はresponseへ進む。BUYの確認phaseは今回追加しない。

## 状態・観測

`Config::selling` はデフォルトfalseで、他の拡張を必須にしない。
`SellCard{Card card;}` のlocal0=3。旧Card/Miracle型・保持番号・台帳hashは固定する。
`Pending::sale_card=3` と `sale_item` は先行消費済み商品を保持し、Effectは末尾追加のsell。
`LastSale` は現在source/target・商品・価格・配送成功・各実資源差・完成turn番号を記録する。
`Player::incoming_sale` は配送で受け取った物理1枚の検証用creditで、通常のgift-backed消費予算と分ける。
生存者の配布後にclearし、そのturnの取引で受取後に終局した人物には残り得る。policy観測には直接載せない。

共通Snapshotはversion35。同じビルド・Config内のfixture用であり、旧形式の永続checkpoint移行はしない。
共通C++観測は1317次元、`g31_*` は先頭1221次元。v31では末尾96特徴が0。旧 `g30_*` は1175、g29以前も従来の出力幅を維持する。
v30では末尾142特徴が0。相手の残手札・非公開保持数・乱数やfixture prefixは追加しない。

| 位置 | 内容 |
| --- | --- |
| 1175 | selling有効なら1 |
| 1176〜1191 | 自分の手札slotが3なら1 |
| 1192〜1207 | 自分の手札slotの商品価格 / max_sale_price |
| 1208 | 現在の保留効果がSELLなら1 |
| 1209 | 現在の保留商品ID / 363 |
| 1210 | 現在の保留価格 / max_sale_price |
| 1211 | 最後の売却解決記録があれば1 |
| 1212 | 最後のsourceが自分なら1 |
| 1213 | 最後のtargetが自分なら1 |
| 1214 | 最後の商品ID / 363 |
| 1215 | 最後の商品価格 / max_sale_price |
| 1216 | 最後の配送に成功したら1 |
| 1217〜1219 | targetのYEN/MP/HP実損失 / 99 |
| 1220 | sourceの実YEN加算量 / 99 |

## C ABI・Python・統計

新C ABI [selling_c_api.h](../include/godfield/selling_c_api.h) は `g31_*` version31。
88byte・kind0〜15・最大65参照・tail@17、mask幅32。旧30語を保ち、word30はSELL3候補、word31は商品候補。
baseと商品slotの相違、1品指定、対象や未使用fieldは `legal()` で検証する。
旧g30以前は新kind15を拒否し、出力幅・行動配置・従来のRNG/検証統計を保つ。

[Pythonラッパー](../python/selling_env.py)は `SellingBatchEnv` を提供する。
batchは全active行を検証してから更新し、1行でも不正なら原子的に拒否する。
終了行は固定・報酬0で、不正手を無視し自動resetしない。報酬は固定座席順。
返却バッファーの保存時はコピーし、同一ハンドルを同時操作しない。

```sh
python3 python/selling_env.py
./build/godfield_legacy_bench 100000 1 42 selling
./build/godfield_legacy_bench 100000 4 42 selling
```

ベンチはStepResultから使用・解決・自己対象・配送成功/失敗・価格・商品種別・実資源差・反射・244同stepを集計する。
支払順と自己alias、99上限、配送時手札容量をpre-step値から検査し、自然tick/244の後の最終HP差を売却損失へ混ぜない。
checksum version31だけへ商品/slots・新event履歴・pending・incoming_saleを混ぜ、旧30までの式と保持loopを固定する。

BUY4・117死亡時攻撃・HP増減賭け・霧/幻覚・守護神・終末・複数人戦、
旧serverの負値/99超交換や取得順保持リスト、現行公式との動的照合、学習器・学習済みモデルは未対応。
衝突C01〜C05も残す。受入検証は全216価格/22MAGIC、消費と保持不変・商品245/244、自己支払順、
満杯でも非rollback、196後current source、244/自然tickと配送budget、復元・秘匿・旧ABI境界・旧30profile互換を対象とする。

## 検証結果（2026-11-01）

通常CTest95件、ASan/UBSan64件、31種類のPython suite計335テストが通過。
全216価格と22MAGIC、先行消費と保持不変・245/244商品、自己aliasのYEN→MP→HP→credit、
満杯配送失敗の非rollback、196反射後current source、244配送即発動と自然tick、
配送creditの双方向/終局条件・active gift予算の復元検証、秘匿・旧ABI境界を固定fixtureで確認した。
既存30profileは各10万試合で、従来の全257項目の決定的出力が基準と一致した。
全機能有効・10万試合・seed42・意思決定上限512で、1/4スレッドの全275項目の決定的出力が一致した。
ASan/UBSanのRelWithDebInfoビルドでも1万試合の全275項目がReleaseと一致した
（checksum version31: `5773965477589128375`）。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 2,998,711 |
| 座席0 / 座席1勝利 | 49,552 / 49,652 |
| 勝者なし / 打ち切り | 796 / 0 |
| SELL使用 / 解決 | 93,105 / 93,105 |
| 配送成功 / 失敗 | 93,080 / 25 |
| 自己対象 / 相手対象の解決 | 0 / 93,105 |
| 価格0の商品 / 価格合計 | 568 / 617,314 |
| targetのYEN / MP / HP実損失合計 | 580,597 / 30,100 / 6,353 |
| sourceの実YEN加算合計 | 617,286 |
| 解決前の196反射回数 | 578 |
| 売却解決と244発動が同じstep | 2 |
| 武器 / 防具 / MAGIC / その他の商品 | 43,312 / 25,069 / 4,120 / 20,604 |
| checksum version31 | 11317147624611195865 |

自然runで自己対象0でも未対応とは扱わず、自己aliasの支払順や244配送を固定fixtureで検証した。
商品価格合計と各実損失/実加算はclipにより異なり得る。自然tick・244回復はStepResultの売却差分へ混ぜない。
商品別の解決数、配送成功/失敗、自己/相手の内訳と、実請求/creditの上限を集計検査した。
