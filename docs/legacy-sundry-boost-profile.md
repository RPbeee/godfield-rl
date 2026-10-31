# SUNDRYの物理追加ATK v35

`legacy-sundry-boost-subset-v35` はSUNDRY INCREASE_ATK238だけを追加する限定profile。
CLIの `sundry-boost` と `g35_*` は全拡張を有効にし、225種・旧重み合計479で動く。
現行公式の完全再現ではなく、旧非公式Flashサーバーを参考にした部分実装である。

[一次処理の調査](legacy-sundry-boost-rules.md)、[台帳・出典・hash](../data/legacy-sundry-boost-v1/README.md)、
[前段の魔法跳ね返しprofile](legacy-magic-bounce-profile.md)を参照。
既存34台帳のhashと保持番号を固定し、対象プール内で旧重みを正規化する。公式の配布確率とは扱わない。

## 238は武器ではなく物理boost

| 旧ID / local index | type | base ATK / boost / DEF / HP / MP | 価格 / 重み | 通常DISCARD |
| --- | --- | --- | --- | --- |
| 238 / 0 | SUNDRY / INCREASE_ATK | 0 / 10 / 0 / 0 / 0 | 15 / 2 | 可 |

原CSVはvalue空・sub_value10。SUNDRYの `Item.getAtk()` がsub_valueを追加ATKとして使う。
sub_value10をMP費用へ変えず、MAGICや保持能力にしない。MP費用・命中率・病気/harms・特殊防御はない。
`SundryBoostCard{Card card;}`、local0=238。単独のweapon base/USEや祈りを妨げるWEAPON所持とは扱わない。

対応する保証命中WEAPONの追加として、通常のboost slotsまたは順序付きextrasで物理コピーを消費する。
同じ手札slotを反復できず、手札にある別コピーはそれぞれ使える。1攻撃の238は最大15枚。
1枚につき配布予定+1を得るが、MP支払・学習・保持再使用は発生しない。
PESTLE99・元から率が正の武器など、従来追加を禁止したbaseには付けられない。

## 順序・57・FREEと属性

238はその位置で現在のrawへ10を足す。**消費N枚を、最終rawが必ず10N増えるという意味にしない。**

| 例 | rawの処理 |
| --- | --- |
| ATK5の武器 → 238 | 5+10=15 |
| ATK5の武器 → 238 → 214 | (5+10)×2=30 |
| ATK5の武器 → 214 → 238 | 5×2+10=20 |
| MP20の57 → 238 → 有料197 | 40+10の後、MP2支払で36へ再設定し197の2を加えて38 |
| MP20の57 → 有料197 → 238 | 36+2+10=48 |

57で再設定するのは**後続の有料MAGIC**に到達したとき。238自体はMPを払わず、57を再設定しない。
FREEで免除されたMAGICもその再設定を起こさない。238の後で有料MAGICへ進めば、先の+10が再設定に失われ得る。
後のDOUBLE_ATKはその時点の全量を倍化し、WIDE_ATKは既存の全敵/1対1正規化・命中100抽選を使う。

238はNE属性の物理追加。既存の属性合成/65・85 setter/209のNE再設定と順序を保つ。
旧ADD_ATTRIBUTEの追加時ATK非加算（C04）は変えない。
最初の後置FREEの直前が238なら、その物理品をMP無料扱いにしても元から費用0であり、
別のMAGICへ無料化を付け替えない。MAGIC後のFREE消費/skipや未使用suffixは既存規則に従う。

完成後の攻撃は元WEAPON type・payloadのまま。対応する防御・反射/跳ね返し・195・counterの既存規則を使う。
反応のたびに238を再消費せず、元のHP吸収/自己被害/元命中結果も保つ。

## canonical防御と原典の受理差

このprofileでは**DEFを持たない238を防御候補から除く**。普通応答・特殊反応のprefixにも使えない。
原典serverの弱い入力guardは、NE防御やMAGIC向け特殊prefixなどで238をzeroDEFとして消費する入力を受理し得る。
応答時の手札交換には戦略的意味があり得るため、「効果0だから冗長で同値」とは主張しない。
公式UIによる受理は未照合であり、今回は既存canonical防御の方針を維持して採用しない。
この点は原典全入力受理の完全再現との差として[差分表](official-spec-gap.md)にも残す。

## 価格・状態・観測

`Config::sundry_boost` はデフォルトfalseで、他の拡張を必須にしない。
旧216価格表/hashを変えず、旧 `sale_price(238)=-1` を固定する。
coreのprivate `trade_price()` は有効時だけ238の価格15を返し、既存4/237/魔法跳ね返し6種の分岐も維持する。
全225商品が売買でき、物理移転だけではboostを発動しない。240の強制除去や通常DISCARDも可能。
boost10は既存最大15以内なので、57・raw/counter上界と最大商品価格30は変えない。

新Action kind・Effect・Pending marker・tape・history・保持slotは作らない。
既存AttackCompositionの物理品列に238を記録し、通常attackは既存のmask表現を使う。
`StepResult::sundry_boost_cards_used` は攻撃extraで実消費した238の枚数。破棄/売買/除去では0を返す。

共通Snapshotはversion35。同じビルド・Config内のfixture用であり、旧形式の永続checkpoint移行はしない。
共通C++観測と `g35_*` は1317次元。旧 `g34_*` は1300、g33以前も従来の出力幅を維持する。
v34では末尾17特徴が0。

| 位置 | 内容 |
| --- | --- |
| 1300 | sundry_boost有効なら1 |
| 1301〜1316 | 自分の手札slotが238なら1。全phaseで現在手札を表す |

## C ABI・Python・ベンチ

新C ABI [sundry_boost_c_api.h](../include/godfield/sundry_boost_c_api.h) は `g35_*` version35。
88byte・kind0〜18・最大65参照・tail@17、mask幅37。旧36語を保ち、word36はsundry_boost_slots。
main時の238手札所属で、response/purchase/doneでは0。boost_slotsと組み合わせ、対象/順序/重複は `legal()` で検証する。
新しい操作は増やさず、旧g34以前の出力幅・行動配置・従来のRNG/検証統計を保つ。

[Pythonラッパー](../python/sundry_boost_env.py)は `SundryBoostBatchEnv` を提供する。
batchは全active行を検証してから更新し、1行でも不正なら原子的に拒否する。
終了行は固定・報酬0で、不正手を無視し自動resetしない。報酬は固定座席順。
返却バッファーの保存時はコピーし、同一ハンドルを同時操作しない。

```sh
python3 python/sundry_boost_env.py
./build/godfield_legacy_bench 100000 1 42 sundry-boost
./build/godfield_legacy_bench 100000 4 42 sundry-boost
```

ベンチはpre-actionの物理コピーから実消費枚数を検算し、通常/ordered・57・有料reset・DOUBLE/WIDE・7属性を分類する。
FREEの直前を含めた順序replayでrawとMPを計算し、保留中の攻撃、57のpower、即時吸収/自己被害eventが得られる場合に照合する。
自己攻撃などで通常pendingが消える場合まで、全rawがeventに残るとは主張しない。raw最大値はpre-action replay値である。
238入り元攻撃への反応step・そこから予約したcounter数、破棄・売買・240除去も集計し、派生counter自体へ238来歴を足さない。
checksum version35だけへ新event/pre-action・238手札位置/既存compositionを混ぜ、旧34までの式と保持loopを固定する。
Usageのprofile列挙も全35種類と一致させる。

117死亡時攻撃・MYSTERY243・霧/幻覚・未対応状態奇跡・守護神・終末・複数人戦、
旧serverの負値/99超交換や取得順保持リスト、現行公式との動的照合、学習器・学習済みモデルは未対応。
衝突C01〜C05と238防御受理差も残す。受入検証は238単独/base/防御拒否・普通boost/複数コピー・
57 reset/DOUBLE/WIDE/属性/FREE順序、反応/原payload・売買/破棄・復元/秘匿・旧ABI境界・旧34profile互換を対象とする。

## 検証結果（2026-11-01）

通常CTest108件、ASan/UBSan73件、35種類のPython suite計414テストが通過。
238のbase/USE/防御拒否、複数物理コピー・plain/ordered、57有料resetとFREE/DOUBLE/WIDE/属性順序、
反応と原payload、価格fallback/売買/破棄、復元/秘匿・旧ABI境界を固定fixtureで確認した。
既存34profileは各10万試合で、従来の全333項目の決定的出力が基準と一致した。
全機能有効・10万試合・seed42・意思決定上限512で、1/4スレッドの全358項目の決定的出力が一致した。
ASan/UBSanのRelWithDebInfoビルドでも1万試合の全358項目がReleaseと一致した
（checksum version35: `4722516657118525409`）。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 3,279,887 |
| 座席0 / 座席1勝利 | 49,729 / 49,527 |
| 勝者なし / 打ち切り | 744 / 0 |
| 238入り攻撃 / 消費238枚数 | 8,782 / 8,826 |
| 通常 / ordered攻撃 | 0 / 8,782 |
| 57をbaseに使用 / 有料MAGIC reset | 73 / 0 |
| 238後のreset | 0 |
| DOUBLE併用 / WIDE併用の攻撃 | 53 / 89 |
| 元攻撃への反応step / 元攻撃からcounter予約 | 770 / 340 |
| 238破棄 / SELL商品 / 購入品 / 240除去 | 6,903 / 272 / 97 / 27 |
| pre-action replayのraw最大値 | 704 |
| rawをevent/pendingと照合した攻撃数 | 8,782 |
| 自己対象攻撃 | 0 |
| 最終NE / 火 / 水 | 8,731 / 18 / 33 |
| 最終木 / 土 / 光 / 闇 | 0 / 0 / 0 / 0 |
| checksum version35 | 12937716155326189344 |

自然runの通常attack・57有料reset・自己対象0は未対応を意味せず、これらの組合せは固定fixtureで検証した。
消費238枚数と最終威力を分け、後の倍化/再設定と元素合成を順序replayで確認した。
反応中は238入りの元攻撃を追跡し、元攻撃が終わった後の派生counterへ同じ来歴を継承して数えない。

244可用数の検算は、事前手札から244の破棄・商品先行消費・強制除去・購入元の消費を引き、
当stepの成功配送だけを加える。自己売却の消費と受取は相殺する。
`seed78564` の1試合回帰は、SELLで当step配送された244が支払い直後のHP0を救う
`sale_revival_steps=1` を必須にし、Release/ASan/UBSanで通過した
（checksum: `5033921231095143693`）。coreの挙動や旧統計を変えずに、この検算境界を保護した。
