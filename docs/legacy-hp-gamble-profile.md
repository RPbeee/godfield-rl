# HP増減賭け v33

`legacy-hp-gamble-subset-v33` はINCREASE_OR_DECREASE_HP237だけを追加する限定profile。
CLIの `hp-gamble` と `g33_*` は全拡張を有効にし、218種・旧重み合計471で動く。
現行公式の完全再現ではなく、旧非公式Flashサーバーを参考にした部分実装である。

[一次処理の調査](legacy-hp-gamble-rules.md)、[台帳・出典・hash](../data/legacy-hp-gamble-v1/README.md)、
[前段の購入profile](legacy-buying-profile.md)を参照。
既存32台帳のhashと保持番号を固定し、対象プール内で旧重みを正規化する。公式の配布確率とは扱わない。

## 237のmetadataと使用時抽選

| 旧ID / 新local index | type | 普通ATK / boost / DEF / HP / MP | amount | 価格 / 重み |
| --- | --- | --- | --- | --- |
| 237 / 0 | SUNDRY / INCREASE_OR_DECREASE_HP | 全0 | 10 | 1 / 2 |

MP費用・命中率・病気/harms・特殊防御はなく、通常DISCARDで捨てられる。
`HPGambleCard{Card card;int amount;}` の所属で符号未決の効果を保ち、無条件HP10回復として登録しない。
MYSTERY243など、複合的なランダム効果は追加しない。

既存 `ActionKind::use=3` のbaseへ手札237、targetへ自己または相手を指定する。
slots/counter/order・extra_count・extras・tailは0。1枚の単独列だけを使い、追加品やFREEを付けない。
237を消費して配布予定+1を得た後、使用時に1回だけ等確率で符号を選ぶ。
bit0は−10、bit1は+10。符号は相手の応答より前に公開し、解決時に再抽選しない。

自己対象は応答無しで即解決し、相手対象はresponseへ進む。
自然ベンチのサンプラーは237だけ自己/相手をpolicy RNGで均等に選び、旧flag=false時の方策乱数は変えない。
符号のgame RNGと、方策の対象選択RNGは別である。

## None payload・現在target・HP clip

普通CSV属性欄は空だが、active payloadはNone。通常防御/195や魔法/武器向け反応は受理しない。
先頭196だけは既存の特別扱いで反射でき、前置品付き196はNone適合検査を免れない。
反射で現在source/targetを交換しても、選択済みの±10を保つ。237の再消費や追加抽選はしない。

解決時の現在targetへ、+10ならHP99上限まで回復、−10ならHP0まで被害を与える。
元使用者や最初のtargetへ適用先を固定しない。普通DEFの減算・元素相克・DARK999・counter予約は使わない。
このHP効果自体はharmではなく、病気/harmsを解除・悪化させたりHeavenを落としたりしない。

| 解決前HP / 符号 | 効果直後HP | 記録する実change |
| --- | --- | --- |
| 40 / +10 | 50 | +10 |
| 95 / +10 | 99 | +4 |
| 99 / +10 | 99 | 0 |
| 40 / −10 | 30 | −10 |
| 3 / −10 | 0 | −3 |

実changeはこの適用直後に記録する。続く244/死亡走査・残counter・元owner自然tick・配布・終局は既存順。
244が救ってHP10へ戻ったり、自然tickがHPを変えても、それを賭け自体の回復/被害へ混ぜない。
196で反射が続く間は未解決とし、実changeを記録しない。

## 固定した価格表・状態・観測

`Config::hp_gamble` はデフォルトfalseで、他の拡張を必須にしない。
旧216件の `sale_prices()` とhash、旧 `sale_price(237)=-1` を固定する。
coreのprivate `trade_price()` がhp_gamble有効時だけ237の価格1を返し、BUY4の価格5分岐も維持する。
全218商品が売買でき、最大価格30は変わらない。売買による物理移転では賭けの符号を選ばず、HP効果も起こさない。

Effectは末尾追加のhp_gamble。`Pending::hp_gamble_card=237` とamount=±10で符号と来歴を保持する。
`LastHPGamble` のsource/targetは最初のimmutableな使用先で、deltaも反射で変えない。
解決後だけresolved_source/targetと実changeを記録し、初期の対象と現在の対象を区別する。
turnはlaunch時に現在turn+1、解決後はそのcompleted turn。符号は公開、未来の抽選prefixは非公開である。

`script_hp_gamble_rolls(span<uint8_t>)` は最大64個の0/1を受け付け、使用ごとに1個消費する。
196反射では進めず、未使用のprefix/位置やgame RNGをpolicy観測に含めない。
旧Pythonのseed→符号のbit単位一致は主張しない。

共通Snapshotはversion36。同じビルド・Config内のfixture用であり、旧形式の永続checkpoint移行はしない。
共通C++観測は1345次元、`g33_*` は先頭1278次元。v33では末尾67特徴が0。旧 `g32_*` は1251、g31以前も従来の出力幅を維持する。
v32では末尾94特徴が0。

| 位置 | 内容 |
| --- | --- |
| 1251 | hp_gamble有効なら1 |
| 1252〜1267 | 自分の手札slotが237なら1 |
| 1268 | 保留効果がHP増減賭けなら1 |
| 1269 | 現在の保留符号付きamount / 10 |
| 1270 | 最後の使用記録があれば1 |
| 1271 | 最初のsourceが自分なら1 |
| 1272 | 最初のtargetが自分なら1 |
| 1273 | 最初に選んだ±10 / 10 |
| 1274 | 最後の記録が解決済みなら1 |
| 1275 | 解決済みsourceが自分なら1 |
| 1276 | 解決済みtargetが自分なら1 |
| 1277 | 効果直後の実HP変化 / 10 |

## C ABI・Python・統計

新C ABI [hp_gamble_c_api.h](../include/godfield/hp_gamble_c_api.h) は `g33_*` version33。
88byte・kind0〜18・最大65参照・tail@17、mask幅35。旧34語を保ち、word34は237手札membership。
新action kindは増やさず、既存utility USEの対象・未使用fieldを `legal()` で検証する。
旧g32以前は新flagを無効にし、出力幅・行動配置・従来のRNG/検証統計を保つ。

[Pythonラッパー](../python/hp_gamble_env.py)は `HPGambleBatchEnv` を提供する。
batchは全active行を検証してから更新し、1行でも不正なら原子的に拒否する。
終了行は固定・報酬0で、不正手を無視し自動resetしない。報酬は固定座席順。
返却バッファーの保存時はコピーし、同一ハンドルを同時操作しない。

```sh
python3 python/hp_gamble_env.py
./build/godfield_legacy_bench 100000 1 42 hp-gamble
./build/godfield_legacy_bench 100000 4 42 hp-gamble
```

ベンチはStepResultから使用時の正負roll・初期self/相手、解決時の現在source/target・実回復/被害・clip・196・244同stepを集計する。
pre-stepのtarget HPで期待changeを検査し、最終HP差から自然tick/244を賭けの量へ足し戻さない。
初期履歴と反射後の符号不変、237が売買商品になった件数も確認する。
checksum version33だけへ新event/保留/履歴とroll prefix内容・count/indexを混ぜ、旧32までの式と保持loopを固定する。

117死亡時攻撃・MYSTERY243・霧/幻覚・守護神・終末・複数人戦、
旧serverの負値/99超交換や取得順保持リスト、現行公式との動的照合、学習器・学習済みモデルは未対応。
衝突C01〜C05も残す。受入検証は±10/self/相手/99cap/致死clip・公開符号/196不変、
単独使用・None適合・244/tick分離・売買価格fallback・復元/秘匿・旧ABI境界・旧32profile互換を対象とする。

## 検証結果（2026-11-01）

通常CTest101件、ASan/UBSan68件、33種類のPython suite計369テストが通過。
±10/self/相手・99上限/致死clip、使用時の公開符号・196反射後の不変、単独使用/None適合、
244と自然tickの分離、売買237の価格fallback、prefix・履歴・予算の復元/秘匿・旧ABI境界を固定fixtureで確認した。
既存32profileは各10万試合で、従来の全297項目の決定的出力が基準と一致した。
全機能有効・10万試合・seed42・意思決定上限512で、1/4スレッドの全315項目の決定的出力が一致した。
ASan/UBSanのRelWithDebInfoビルドでも1万試合の全315項目がReleaseと一致した
（checksum version33: `3294291453240839054`）。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 3,303,459 |
| 座席0 / 座席1勝利 | 49,495 / 49,752 |
| 勝者なし / 打ち切り | 753 / 0 |
| 237使用 / 解決 | 7,233 / 7,233 |
| +10抽選 / 正の効果解決 | 3,642 / 3,642 |
| −10抽選 / 負の効果解決 | 3,591 / 3,591 |
| 初期self / 解決時self | 3,613 / 3,613 |
| 初期の相手対象 | 3,620 |
| 実回復 / 実HP損失合計 | 36,412 / 34,410 |
| 実change0 | 0 |
| 回復の99clip / 被害の0clip | 2 / 292 |
| 解決前の196反射回数 | 21 |
| 賭け解決と244発動が同じstep | 5 |
| SELLの商品となった237 / 購入された237 | 317 / 139 |
| checksum version33 | 12632247955128968068 |

policyの対象選択とgameの符号抽選を分け、自然runでも自己/相手・正/負の両方が通った。
実変化はStepResultで検査し、自然tick・244の後の最終HP差を賭けの回復/被害と混同しなかった。
未解決の196反射で符号/初期履歴を変えず、旧216価格表を上書きしない売買fallbackも確認した。
