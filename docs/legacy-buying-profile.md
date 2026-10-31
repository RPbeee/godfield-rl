# 購入確認 v32

`legacy-buying-subset-v32` はBUY4だけを追加する限定profile。
CLIの `buying` と `g32_*` は全拡張を有効にし、217種・旧重み合計469で動く。
現行公式の完全再現ではなく、旧非公式Flashサーバーを参考にした部分実装である。

[売買の一次処理調査](legacy-buying-rules.md)、[台帳・出典・hash](../data/legacy-buying-v1/README.md)、
[前段の強制売却profile](legacy-selling-profile.md)を参照。
既存31台帳のhashと保持番号を固定し、対象プール内で旧重みを正規化する。公式の配布確率とは扱わない。

## BUY4と凍結した価格表

| 旧ID / 新local index | type | ATK / boost / DEF / HP / MP | 価格 / 重み |
| --- | --- | --- | --- |
| 4 / 0 | TRADE / BUY | 全0 | 5 / 20 |

MP費用・命中率・病気/harms・特殊防御はない。通常DISCARDで捨てられる。
専用台帳の所属でBUYを保つ。`BuyCard{Card card;}`、local0=4。
旧216件の `sale_prices()` とhashを変えず、旧 `sale_price(4)` も-1のまま固定する。
coreのprivate `trade_price()` がbuying有効時だけ4の価格5を返し、それ以外は旧表へ委譲する。
そのため4もSELL3の商品にでき、全217種の商品価格が揃う。最大価格30は変わらない。

## 使用・抽選と購入確認

`ActionKind::buy=16` はbaseへ手札の4、targetへ自己または相手を指定する。
slots/counter/order・extra_count・extras・tailは0。追加品やFREEを付けない。
開始時にBUY4を1枚消費して配布予定+1を得る。

解決時の現在targetの手札コピーを一様抽選する。重複品も各コピーを1候補とする。
空手札なら抽選乱数を消費しない。非空では1枚だけの場合も1回消費する。
自己使用では元4を先に消費するので、そのコピーは抽選対象に含めない。

| 結果 | 次の動作 |
| --- | --- |
| 空 | item無しのempty結果を記録して終了 |
| 自己の品 | 選んだitemを公開するが購入/再消費せずself結果で終了 |
| 他者品で現在buyerのYEN不足 | insufficient結果で終了。MP/HPへ請求しない |
| 他者品で現在buyerのYENが足りる | offered結果と商品/価格を公開し、新しいpurchase phaseへ |

`Phase::purchase=2` のactorは現在sourceであるbuyer。元turn_ownerとは限らない。
`buy_decline=17` / `buy_accept=18` はbase/targetをno_slot、他fieldを0にして選ぶ。
NOはdeclined結果で終了し、YESはpurchased結果へ進む。商品抽選のやり直しはしない。
確認は追加の1意思決定を使い、offer時点ではowner自然tick/配布/手番終了をまだ実行しない。
上限に達すれば未決offerのまま打ち切られる。

## YESの支払いと物理配送

YESの解決は次の順。

1. 現在targetの選ばれた物理コピーを直接手札から除去する。
2. 現在targetへ商品価格を加え、YEN99でclipする。
3. 現在sourceの手札が16枚未満なら商品IDの物理コピーを末尾へ追加する。
4. 現在sourceから価格分のYENを払う。

**購入は現金YENだけで行い、MP/HPを資金にしない。**
原典の汎用dealItemは満杯なら失敗しても取引を戻さないが、**この閉じたBUY scopeでは合法な購入は必ず配送に成功する。**
最初のbuyerは4を消費し、反射後のbuyerも196を消費しているため、offer中のbuyer手札は最大15枚。
確認前に別の手札取得はなく、満杯offerやpurchasedなのにdelivered=falseの履歴は復元時に拒否する。
原典の汎用的な満杯時非rollbackは調査資料の参照挙動であり、通常runや合法fixtureの到達例にしない。
sellerが99上限で全額を受け取れなくてもbuyerは商品価格全額を払う。
移るのは物理品であり、武器の捨て禁止/245のDISCARD skipとは別処理。
MAGICを買っても保持化・MP費用・使用効果は発生せず、既存の保持能力も移転/減少しない。
配送や対象品の除去で配布予定を追加せず、最初のBUY4だけがsource配布予定+1を得る。
最後に既存の244/死亡走査・残counter・元owner自然tick・配布・終局を行う。

## None・反射・公開結果

BUYの普通CSV属性欄は空だがactive payloadはNone。通常防御/195や魔法向け反応は使えない。
先頭196は既存の特別扱いで反射できる。前置品付き196はNone適合検査を免れない。
反射中にはまだ商品を選ばず、最後に解決する現在source/targetをbuyer/sellerとする。
BUY4は再消費せず、元使用者へ支払いや所有先を固定しない。

選ばれた商品IDと価格・buyer/seller・outcome・配送/実YEN差は公開する。
抽選前の手札枚数、選択slot/ticket、相手の残り手札や保持数は公開しない。
`script_buy_tickets(span<uint64_t>)` は最大64個、非空の現在手札枚数Tに対しticket % Tを選ぶ。
T0は消費無し、T1も1個消費。fixture prefix/位置はpolicy観測へ含めない。
旧Pythonのseed→コピー対応や取得順リストのbit単位一致は主張しない。

## 状態・観測

`Config::buying` はデフォルトfalseで、他の拡張を必須にしない。
Effectは末尾追加のbuy、`Pending::buy_card=4` が来歴を保持する。
`BuyOutcome` はnone0/empty1/self2/insufficient3/offered4/declined5/purchased6。
`LastBuy` は商品・現在source/target・価格・結果・配送・実YEN差・turn番号を記録する。
抽選前手札枚数とslotはprivate内部値。offeredのturnは現在turn+1、完了した他の結果では完成turnを表す。
`Player::incoming_buy` は配送で受け取った1枚の検証用creditで、既存incoming_saleと別管理する。
生存者の配布後にclearし、そのturnに配送された後のHP0終局では最新purchased記録との一致を検証する。
同一completed turnへSaleとBuyの両履歴を作らない。これらの内部予算は観測しない。

共通Snapshotはversion36。同じビルド・Config内のfixture用であり、旧形式の永続checkpoint移行はしない。
共通C++観測は1345次元、`g32_*` は先頭1251次元。v32では末尾94特徴が0。旧 `g31_*` は1221、g30以前も従来の出力幅を維持する。
v31では末尾124特徴が0。

| 位置 | 内容 |
| --- | --- |
| 1221 | buying有効なら1 |
| 1222〜1237 | 自分の手札slotが4なら1 |
| 1238 | 保留効果がBUYなら1 |
| 1239 | 現在purchase phaseなら1 |
| 1240 | 確認中の商品ID / 363 |
| 1241 | 確認中の価格 / 30 |
| 1242 | 最後のBUY記録があれば1 |
| 1243 | 最後のsourceが自分なら1 |
| 1244 | 最後のtargetが自分なら1 |
| 1245 | 最後の商品ID / 363 |
| 1246 | 最後の商品価格 / 30 |
| 1247 | 最後のoutcome / 6 |
| 1248 | 最後の配送に成功したら1 |
| 1249 | buyerの実YEN支払 / 99 |
| 1250 | sellerの実YEN加算 / 99 |

## C ABI・Python・統計

新C ABI [buying_c_api.h](../include/godfield/buying_c_api.h) は `g32_*` version32。
88byte・kind0〜18・最大65参照・tail@17、mask幅34。旧32語を保ち、word32はBUY4候補、
word33はpurchase_choices（bit0=NO、bit1=YES）。phase・actor・affordabilityや未使用fieldは `legal()` で検証する。
旧g31以前は新kind16〜18を拒否し、出力幅・行動配置・従来のRNG/検証統計を保つ。

[Pythonラッパー](../python/buying_env.py)は `BuyingBatchEnv` を提供する。
batchは全active行を検証してから更新し、1行でも不正なら原子的に拒否する。
終了行は固定・報酬0で、不正手を無視し自動resetしない。報酬は固定座席順。
返却バッファーの保存時はコピーし、同一ハンドルを同時操作しない。

```sh
python3 python/buying_env.py
./build/godfield_legacy_bench 100000 1 42 buying
./build/godfield_legacy_bench 100000 4 42 buying
```

ベンチはStepResultから使用・抽選・empty/self/insufficient/offered・YES/NO・配送/支払い・196反射を集計する。
抽選前のコピー列でslot/ID/価格を照合し、価格/上限と配送時容量をpre-step値から検査する。
offer数=decision数+終端の未決offer数、YES数=配送成功+失敗、商品種別の内訳も確認する。
現scopeの合法購入では配送失敗は0であり、到達不能な満杯の例を通常runの成功例にはしない。
BUY4のSELL商品件数を追加し、旧216価格表の上書きをせずfallbackが動くことを検査する。
checksum version32だけへ新event履歴・private手札枚数/slot・phase/pending・受取creditを混ぜ、旧31までの式と保持loopを固定する。

117死亡時攻撃・HP増減賭け・霧/幻覚・守護神・終末・複数人戦、
旧serverの負値/99超交換や取得順保持リスト、現行公式との動的照合、学習器・学習済みモデルは未対応。
衝突C01〜C05も残す。受入検証はT0/T1/重複/自己4消費後/196後の抽選、offer/YES/NO/不足、
全217価格・物理MAGIC保持不変・245/244・不可能な満杯offer/配送false履歴の拒否、tick/配布1回・受取budget、
復元・秘匿・旧ABI境界・旧31profile互換を対象とする。

## 検証結果（2026-11-01）

通常CTest98件、ASan/UBSan66件、32種類のPython suite計353テストが通過。
T0/T1/重複/自己4消費後・196後の抽選、offer/NO/YES/現金不足、全217価格と保持不変、
4/196消費からの配送余裕と不可能な満杯offer/配送false履歴の拒否、予算・秘匿・旧ABI境界を固定fixtureで確認した。
追加のmax_decisions3でYES/NO解決と同時に上限へ達する境界も、Release/ASan/UBSanで検証した。
既存31profileは各10万試合で、従来の全275項目の決定的出力が基準と一致した。
全機能有効・10万試合・seed42・意思決定上限512で、1/4スレッドの全297項目の決定的出力が一致した。
ASan/UBSanのRelWithDebInfoビルドでも1万試合の全297項目がReleaseと一致した
（checksum version32: `14888991247400539634`）。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 3,292,580 |
| 座席0 / 座席1勝利 | 49,626 / 49,624 |
| 勝者なし / 打ち切り | 750 / 0 |
| BUY使用 / 抽選処理 | 92,260 / 92,260 |
| 空 / 自己品 / 現金不足 | 770 / 0 / 6,992 |
| offer / 確認decision | 84,498 / 84,498 |
| NO / YES | 42,489 / 42,009 |
| 配送成功 / 失敗 | 42,009 / 0 |
| buyerのYEN支払 / sellerの実加算合計 | 259,465 / 259,459 |
| 価格0の商品を購入 | 308 |
| 解決前の196反射回数 | 538 |
| 終端の未決offer | 0 |
| 購入と244発動が同じstep | 2 |
| 武器 / 防具 / MAGIC / その他の購入 | 20,076 / 10,216 / 1,476 / 10,241 |
| SELLの商品となったBUY4 | 2,904 |
| checksum version32 | 5311350460614090260 |

自然runで自己品0でも未対応とは扱わず、自己4消費後の抽選を固定fixtureで検証した。
全YESが配送に成功し、確認中のbuyer最大15枚という閉scopeと整合した。
支払とseller加算の差はYEN99上限によるもの。抽選/offer/decision/配送・商品種別の収支を別々に検査し、
未決offerと完了取引を混ぜずに集計した。BUY4の商品価格fallbackも実際の2,904件で使われた。
