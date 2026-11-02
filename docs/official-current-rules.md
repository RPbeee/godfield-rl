# 現行公式公開クライアントの一次調査

**公式公開資料のv2同期（2026-11-02）:** [official-v2](../data/official-v2/README.md) に最新版JA/JSの出典を別世代として固定し、ID19/26/117の画像名と117「石板」を反映した。296モデルの数値・能力・curse・giftRate・ID順はv1から変更0。[v1→v2比較](../data/official-v2/upgrade-review.json) を保存し、旧official-v1のpins・crosswalk・legacy37 fingerprintは変更しない。新JSの抽出関数/分岐の再照合は下記v27節に記録する。全JS・全UI・権威的serverの同値性を保証するものではない。過去のfresh一致記録と各版の計測は、当時の取得物に対する履歴として保持する。

この文書は2026-11-01のv1と2026-11-02のv2を通常のHTTP GETで取得した調査記録である。ログイン、匿名認証、部屋への参加、対戦、Cloud Functionsへのコマンド送信は行っていない。旧Flashエミュレーターの仕様を現行公式の根拠として扱わない。

現行公式は[公式サイト](https://godfield.net/)に配信される Dart コンパイル済みJavaScriptとローカライズJSONから、神器台帳、教典、画面の選択制約、表示処理を検証できる。ただし、これだけで全ゲーム状態遷移や乱数処理まで取得できたという根拠はない。以下の「未確定」は仕様の推測で埋めない。

## v1取得物と固定方法（履歴）

| 公式URL | バイト数 | SHA-256 |
| --- | ---: | --- |
| [トップHTML](https://godfield.net/) | 2,485 | `fa085ea6481eb4e273dd28fe9eada8f0f2245af836872032dcff1cdc5427fef9` |
| [main.dart.js](https://godfield.net/main.dart.js) | 440,238 | `764a50524e4b6b3f510415da7128abd8ad99dcb87b45b8572d98ddd96889cabd` |
| [日本語JSON](https://godfield.net/i18n/ja.json) | 49,137 | `03a4782dc6899aeedd9b7986a49bd3c0217b96ac38c0779deb588bc94cca3e7c` |
| [英語JSON](https://godfield.net/i18n/en.json) | 46,734 | `bb1b433ba6d336765589aec940cca0bac8b7c0bdcee33969c207a17365509188` |

調査時の作業コピーは `/tmp/godfield-official-index.html`、`/tmp/godfield-official-main.dart.js`、`/tmp/godfield-official-ja.json`、`/tmp/godfield-official-en.json`。JSの行番号は上記ハッシュのファイルに対するものであり、公式の更新後に同じ行番号を使ってはならない。ja.jsonの応答はHTTP 200、`Last-Modified: Thu, 29 Oct 2026 07:43:52 GMT`、取得時のHTTP Dateは2026-10-31 17:25:42 GMTだった。

HTMLは `/main.dart.js` を読み込む。JSの `fN`（4371–4380行）は `/i18n/{lang}.json` を取得して、`texts.reference`、属性・災い・守護神の説明、`items` を読み込む。台帳 `items` は296行である。現行IDは `ty`（3936–3970行）の配列位置+1で付与され、旧CSVのIDと同一ではない。

JSのハッシュは従来の第三者抽出資料[B]に記録されたものと一致した。今回は公式配信URLから同じバイト列を取得できたため、そのファイル自身を一次根拠として検証できる。ただし、過去の第三者説明や観測まで自動的に公式一次根拠へ昇格するわけではない。

## 確定度の区別

| 根拠 | 確定できること | 確定できないこと |
| --- | --- | --- |
| 公式JSONの教典・説明 | 現在ユーザーへ説明される規則、名称、確率 | サーバー実装の端点・同時効果順・乱数消費 |
| 公式JSONの `items` | 表示台帳の属性、効果タグ、値、MP、価格、各率 | 全生成分布やサーバー側の例外 |
| JSの計算・選択処理 | 実際のクライアント表示算式、UIで選択できる組合せ | UI外入力をサーバーがどう扱うか |
| JSのイベント受信処理 | クライアントが解釈する状態・公開イベント・表示変化 | イベントを生成する権威的エンジンの全規則 |

## 光・闇と属性合成

公式JSONの `texts.elementDescriptions` と `texts.reference.lightNote` は次の規則を説明する。

- 無属性の攻撃は任意属性で防御できる。
- 火と水、木と土がそれぞれ対になる。
- 光攻撃は属性による防御を認めない。
- 闇攻撃は任意属性で防御できるが、被害を受けるとHP0になる。
- 光は基本4属性の代用として合成できる。

JSの `aN.kO`（13039–13052行）も、火/水・木/土それぞれの対属性または光を認め、攻撃属性が光なら常に不適合を返す。その他（闇・無属性）は適合を返す。したがって[差分表](official-spec-gap.md)の **C01は現行公式教典とクライアント判定の両方で旧版との差が確認できる**。旧 `LIGHT:DARK` の防御分岐を公式profileへ移してはならない。反射等の特殊反応は `e1`（13026–13038行）で別に判定するため、「属性で防御不可」と「全応答不可」は区別する。

合成は `pq`、`l9`、`tx`（3977–4001行）。`l9` は選択順に属性を合成し、属性付け替え・属性消去・無料化の個別タグを処理する。`tx` は防御の属性合成。攻撃量は `tw`（3971–3976行）で先頭の攻撃量に後続を加え、倍化と無料化を個別に扱う。現行の付け替えワンド（ID66、83）はJSONで加算可能なATK2/5を持ち、`tw` でも通常加算対象である。旧65/85の追加時非加算とは異なり、**C04はクライアントの計算結果まで確認できた**。サーバー側の実HP変化との独立照合は未実施。

## 特殊防御と反射：公開clientで確認した範囲

固定JSの `e1`（13036–13047行）は、reflectWeaponを無属性の武器、reflectMiracleを属性を問わない奇跡、reflectAnythingを全categoryへ適合させる。最後の能力を持つ190も、旧版の「先頭だけ万能」というサーバー条件をそのまま採る根拠にはならない。現行の実際の選択条件は `ev`（9478–9501行）で別に確認する。

`ev` がmatching反応品を追加できるのは、選択列が空か、直前がfilterAtkElementの場合だけ。普通防具→反射は不受理で、反射へ前置できるのは先頭から連続した属性消去だけである。属性消去→普通防具は可だが、そこへ反射を追加することはできない。matching品を普通DEFに切り替える分岐も無い。非matching時にはarmorの通常DEFの経路へ入る。非攻撃の回復へは属性消去を前置できないが、matchingする奇跡反射・万能反射を単品で選べる。

反射イベント `reflect`（11032–11056行）は旧sourceの表示プレイヤーを保存し、現在のsource paneを消し、`cZ`（12076–12099行）で攻撃側を反転する。反転後は以前の防御者がsource、保存した旧sourceがtargetになる。反射ごとに主手番ownerを新規決定する処理はここに無い。

`cZ` は防御列の末尾から反応品等を外し、残ったfilterAtkElementの列を元攻撃モデルの末尾へ移してから、表示上の攻撃列を反対側へ運ぶ。再集計には元aggregateの動的ATK・hitRate表示値を渡す。したがって元の威力・能力・categoryを返し、消去された属性を無属性として維持する表示構造であり、普通防具による減算を持ち越す構造ではない。元の奇跡を新たに手札から起こす `useAttackItems` に戻る処理でもない。

一方、`useDefenseItems`（10802–10879行）は受信したitems列全件を表示した後、その応答列のMP費用を引き、初使用奇跡を保持表示へ追加する。matching反応を見つけた時点でloopをbreakする処理は無い。submit時の `ee(null)` も選択済み列全件を消費予告する。防御奇跡233/234の直後のFREEはappend条件で許されるが、physical反射や190の直後のFREEは許されない。旧エミュレーターの「特殊後FREEは未消費suffix」という規則を現行へ移す根拠にはしない。サーバー側で実際に発行するitems列と最終消費結果のtraceはまだ取得していない。

自己がtargetになった場合、`ft.aT`（12400行付近）は新しい別defender paneを作らず、source側へ矢印を向ける。bounceイベントも別paneが無ければ応答演出 `bB` を省く（11020–11030行）。自己返却に独立したresponse paneが無いことは確認できるが、これはサーバーでの自動解決や応答省略を証明しない。ローカル実行器が自己反射・自己跳ね返しもresponseを続けるのは、明示的な実験規約である。

bounceの対象はevent.playerIdで既に与えられ、handler自身は対象抽選をしない。blockイベントも遮断の演出だけで、補充・次手番を決定しない。counterAttack（11603–11616行）は指定indexの防具を光らせるだけで、反撃の生成や重複、FIFO予約を実装しない。nextAttack（10881–10888行）が受け取るatk/mp/cp等の値を `fv`（12115–12143行）が表示するため、反撃威力のraw/実HP損失依存、×2の算出時点、死亡skip、乱数消費、gift発行順は未確定である。

これを根拠として、限定実行器v2は物理反射38/39/156/167/174/190と消去189を加えた139モデル/重み319に限定し、v3でblockMiracle8種とblockWeapon233を加えた148モデル/重み328へ進めた。v4はさらにFREE・setter・倍化8種を加えた156モデル/重み337。v5は跳ね返し8種を加えた164モデル/重み351。v6は単純確率攻撃21種を加えた185モデル/重み372。v7は攻撃反撃181/184を加えた187モデル/重み374。v8は資源反撃187/188を加えた189モデル/重み376。v9はHP吸収/同被害5種を加えた194モデル/重み381。v10は閃光付与/反撃/解除7種を加えた201モデル/重み404。v11は暗雲付与/反撃3種を加えた204モデル/重み407。v12は除去2種を加えた206モデル/重み409。v13はMP武器58と二回攻撃14を加えた208モデル/重み411。v14は全敵化232を加えた209モデル/重み412。v15はHP増減202を加えた210モデル/重み414。v16は自動復活208を加えた211モデル/重み415。v17は杵94/臼209を加えた213モデル/重み417。v18は両替3を加えた214モデル/重み437。v19は同じプールのまま除去結果の公開要約を加える。v20は満杯配送を除く売却4を加えた215モデル/重み457。v21は自己対象/満杯配送を除く購入5を加えた216モデル/重み477。v22は病気4状態/付与6種を加えた222モデル/重み483。v23は自己病気防具168/MP+Heaven雑貨198を加えた224モデル/重み485。v24はFog4種と観測/対象選択を加えた228モデル/重み489。v25は112の通常/死亡時攻撃を加えた229モデル/重み490。v26は同じ台帳で取引配送overflowとFogのforeign SELLを扱い、v27はその規約を維持して公式台帳v2へ同期した。v28はDream4種と互換クラス限定maskを加えた233モデル/重み494を扱い、v29は178のredrawを加えた234モデル/重み495を扱い、v30は同プールで自己BUY照会を追加し、v31は別入口で外部指定の守護神42行動を実行した。現行の[限定実行器v32](official-execution-subset.md)は201/239召喚とcanonical42の自動行動/離脱を実験規約で接続し236モデル/重み499を扱う。互換クラス外の偽装やEarth/Moonの一般神器/奇跡選出・超常現象・終末は含めず、反応source/target・元payload・各応答者の消費と抽選来歴を分離する。サーバー同等性を確認した全面反応実装ではない。

### 跳ね返し8モデルの一次確認とv5規約

公式JSONでは21がbounceWeapon、34/114/125/137/151/164/234がbounceMiracle。21はATK5・giftRate7、34はATK9/isPlusAtk、5防具はDEF1/3/5/7/9、234はcost5の奇跡で、残りのgiftRateは各1。合計8モデル/追加重み14を確認した。`gkN` は234の主動作開始を拒否し、物理品の通常ATK/追加ATK/非matching普通DEFは保持する。`e1` は武器跳ね返しにweaponsかつ無属性、奇跡跳ね返しにmiraclesという条件を課す。回復奇跡も後者にmatchingする。

bounce handler（11000–11030行）はreflectと同じ `cZ` を実行し、防御者が次のsourceになる。元aggregateの動的ATK/hit値を保って攻撃列を移し、次targetは受信済み `event.playerId` (`c3.b`) を参照する。handler内に生存候補の列挙・抽選は無い。JA教典 `bounceNote` は「攻撃を弾くと、その攻撃は誰かに向かう。」、ENも “it goes to somebody” とだけ説明し、生存者限定・自己/味方の包含・一様性・候補1人時の乱数消費は確定しない。

v5は全aliveを入力seat順に列挙し、自己・味方・旧sourceを含めてSplitMix64の有界抽選で一様に選ぶ**ローカル実験規約**を定める。候補1人でも1draw、反射は0draw、自己targetもresponseを続ける。公開 `bounce_target_rule` で未検証の抽選規約を明示し、公式serverの分布と同一視しない。

234は既存MC経路で初回used保持/再使用とも5MP、直後FREEなら完成列費用0。234→FREEの選択は `ev` のMC直後特例で許可され、全itemsの消費予告と費用計算に含む。無料化して同tokenを後の応答で再使用できるため、physical-onlyや固定32の連鎖上限を流用できない。v5は意思決定・全seatの物理消費/MP/補充予算と、乱数anchorからのredirect再生で検査する。消費発行・死亡・補充・次手番のscheduleは引き続き公式server未検証である。

### 遮断9モデルの一次確認とv3規約

公式JSONのblockMiracleは42/50/55/56/165/171/177/179の8種、blockWeaponは233の1種。全て無属性でgiftRate1、v2集合との重複は無い。物理8種は価格15で、ATK11/13/15/15とDEF9/11/13/15を持つ。56はisPlusAtkで追加ATK15にも使える。233はcost6の奇跡で、`gkN`（13024–13033行）では主動作開始を拒否する。

`e1` はincoming categoryがmiraclesならblockMiracle能力を反応結果 `block` と判定し、威力が正か、回復か、属性が何かは条件にしない。武器遮断233はcategoryがweaponsかつ現在属性が無属性のときだけ。`ev` の空列/直前filter条件はblockにも適用され、matching品の普通DEF opt-outや、普通防具→blockは受理しない。filter→blockは、incoming_is_attackを満たす攻撃への前置であれば可能。回復奇跡へのblockは単品でmatchingし、filterを使って奇跡を武器へ変える処理ではない。

この一般predicateと現行台帳の効果の実在は別である。単純回復奇跡は235がHP、236がCPを増やし、MP増加の奇跡は台帳に無い。MP回復雑貨や守護神のMP増加を、効果が似ているという理由でmiraclesへ読み替えない。

233の費用と保持表示は既存の通常MC経路に従う。submit時の `ee(null)` はMCをusedとして保持し、18枠未満なら補充用placeholderを作る。`useDefenseItems` は受信items列を表示した後、`la` の費用をMPから引く。233単品は初使用/再使用とも6MPで、既usedのtokenを使い直しても新しい保持品を別に作らない。防御奇跡→FREEのUI条件はv3では対象外だったが、v4から実行対象へ加える。

公式 `block` event handlerは `dE` を呼び、`dE` は音と `aZ("block", side)` の表示後に戻る（9765–9772行）。元のHP/MP/CP効果を取消す権威的な処理、補充予定の処理、手番決定はここに無い。v3は現在payload全取消、原発MCの既払いMPは戻さず再課金もしない、遮断品の消費/MP/used保持/補充予約は通常処理、未終局/未打切りならまとめて補充して元ownerから巡回する、という明示的なローカル規約にする。反射途中のblockもその場で終端にし、blockをpending反射ledgerへ積んで次の応答を作ることはしない。

### FREE・setter・倍化の一次確認とv4規約

46/143/163/173/204はcutCostで、204がgiftRate2、残りは1。setter66/83とdoubleAtk231も各giftRate1で、v3と重複しない8種/追加重み9を確認した。

`tw/l9/la`（3971–4007行）は、追加のcutCostのATK/攻撃属性をskipし、各MCの直後がcutCostならそのMC費用を免除する。複数のペアは全て免除され、最初のFREEだけを採る旧版と異なる。先頭46はATK12で、後続MCを自動的に無料にしない。防御 `tx` はcutCostも通常どおり属性合成する。setterはATK2/5を加えて属性上書き、倍化は累積ATKを2倍にして、その後の属性を無属性との通常合成にする。`eV`（14171行）はATK/命中率の表示文字列を作る関数であり、解決順を決めるエンジンではない。

選択中の `ev/kM/ip` はMP残高を参照せず、submit先頭（9596行付近）で完成列の `la` と本人MPを比較する。したがって途中の壁233だけならMP不足でも、直後へFREEを選んで最終費用0にすることはできる。prefixごとに「現時点で費用を払える」を強制すると、この公開clientの選択経路を誤って拒否する。

応答の `ev` は最後がMCかつ次がcutCostなら、反応後の通常禁止より先に許可する。233→FREEは該当し、物理反射→FREE、233→FREE→さらにFREEは該当しない。`useDefenseItems` は受信items全件を表示してから費用を引き、submitの `ee(null)` も選択列全件の消費を予告する。v4は233をused保持、FREEを物理消費、費用0・補充cap内とするローカル規約を採り、旧「特殊後suffix未消費」を採用しない。公式serverでの最終消費・補充発行順の独立trace照合は引き続き未完了である。

204と231は主動作単独不可、setter66/83は普通の武器基底としても使える。反射はこの順序付き計算後のpayloadを維持し、原発MCを再課金・再取得しない。これらの公開計算・選択と、ローカル実行器のターン・補充・死亡順を同じ確定度として扱わない。

## 攻撃反撃：一次で確認した範囲とv7規約

原典の非災い反撃は181（火/75%/counterAtk）、184（土/counter2xAtk）、187（counterBoost2xMP）、188（counterTakeCP）の4種。全てcategory armor、通常ATK/DEF/MP0・価格10・giftRate1。v7では181/184の2種を追加し、v8で資源反撃187/188も対象へ加える。

`fv`（12115–12143行）は181/184に受信attack.atk、187にattack.mp、188にattack.cpを動的表示として渡す。`gf3`（12293–12295行）は動的ATK>0ならincoming_is_attackを認めるが、187/188の非ATK payloadは認めない。反撃のcategoryをweaponsへ置換する処理は無く、`e1/ev` から181/184への普通属性防御/filter・190万能反射が適合し、武器/奇跡専用反応は適合しないことを確認できる。

教典は181を「攻{ダメージ}」、184を「攻{ダメージ×2}」と説明する。`counterAttack` は指定indexの選択品を点滅させ、`nextAttack` はserverから受け取ったattackを表示する。実HP損失かraw残余か、被害0/死亡/反射/遮断時の生成、FIFOや命中抽選・死者skip・補充順を決めるコードではない。187のMPは現在targetへ加算するhandler、188のtakeCPは現在targetから先に全額請求し現在sourceへ同額加算するhandlerまで分かるが、187の対象方向を決めるserver処理は未確定である。

v7は実HP減少>0かつ生存した防御者だけが、選択181/184の順で各1counterを生成する規約を採る。181は実HP減少1倍/火/75%、184は2倍/土/必中、生成powerは198以下。被害を与えた現在source一人へ向かい、counter生成者originを保つ。counter FIFOを次の初期chance対象より先に処理し、childcounterは末尾へ追加、開始時source/target死亡は無draw skipとする。181はsingle targetの開始時に1回命中抽選し、redirect後再抽選はしない。

command末尾の自然終局→cutoffを優先し、元owner死亡なら残り初期chance対象を取消すが、既予約counterは生存条件を満たせば続ける。全action後だけalive補充/次ownerへ進む。これらのtrigger/実HP基準/順序/deathskip等は**公式server未検証の実験規約**で、公開 `counter_rule='experimental:actual-hp-loss;survivors-only;selection-order-fifo-before-next-target;skip-dead-source-or-target;single-target;darkcloud-forces-without-draw;roll-before-response;redirects-no-reroll'` に明示する。

全pending playを非再帰ActionBatchへ統一し、launch前state/RNG/counters/原発command+response journalから同じexecutorで完全再生して現在frame/FIFOまで比較する。将来FIFO/journal/RNGやnativecounter集計は公開viewへ出さない。旧版のraw残余×2・死者source実行・固定32等を現行の確定ルールとして移さない。

## 資源反撃：算術の一次根拠とv8規約

187のJA説明は「+MP{ダメージ×2}」、188は「¥{ダメージ} 没収」。`fv` は187へ受信attack.mp、188へattack.cpを動的表示として渡し、`gf3` はこれらpower0のarmorをattackと判定しない。`e1/ev` は190万能反射だけをmatchingさせ、普通DEF/filter/他counter/武器・奇跡用反応は適合しない。guardian欄neptune/venusはカードmetadataであり、abilityがsetGuardianではないこの2品からplayerguardianを召喚する根拠は無い。

`boostMP`（11303行付近）は現在targetへ `dH(event.mp)` を適用し、MPを99上限へclipする。`takeCP`（11341–11342行）は現在targetへ `bc(-event.cp)` を適用後、現在sourceへ `bc(event.cp)` を適用する。`bc/dH/eW`（13234–13244行）により不足CP→MP→HPへ波及、加算は99でclipし、実所持CPを上限にしない。自己aliasでも逐次処理するので、CP3/MP4/HP40への名目10はCP10/MP0/HP37、CP95へ名目10は95→85→95になる。資源primitiveはHP0でdeadを自動付与せず、後からCPを加えてもHPは戻らない。

一方、187のselftarget・明示response・実HP損失×2、188の旧incoming.sourceへの対象設定・実HP損失額、survivor生成やFIFOは公開server処理から未確定である。v8はこれらを**実験規約**とし、187はpassで一度だけ現在targetのMPを回復、自己190反射後もresponse継続、188は現在targetへ全額請求→現在counter.sourceへ全名目額creditとする。190で徴収方向が交換され、自己HP0をCPcreditで蘇生させない。

通常damage bodyだけがcounter生成元であり、resource-debtによるHP損失から新counterを作らない。共通survivor/FIFO/deadskip/naturalcutoff/全action後gift規約は維持する。公開resource_counter_ruleは `experimental:mp-self-target-with-response;cp-target-incoming-source;debit-current-target-then-full-credit-current-source;resource-debt-no-new-counter`、counter_ruleのtarget表現はsingle-targetへ更新する。CP primitiveの確定算術と、未確認の生成/対象規約を混同しない。

MP187により旧paid234の最大19回/人は成立しない。初期MP総量<=99*nと各187の実MP増分<=99・生成counter数<=n*hand_limitを用い、response上限を `2*n*hand_limit+floor(99*n*(hand_limit+1)/5)+targets.size()` へ更新する。全execute replayで資源/MP/gift/FIFOを照合し、未来journal/予算を公開viewへ出さない。

## 単純確率攻撃：確認範囲とv6の規約

公式JSONでgiftRate>0・hitRate>0・ability noneの神器は21種/重み21。武器15種（95/96/99〜111）と奇跡6種（225〜230）で、全てgiftRate1、命中率は25/50/75。108は木ATK8・75%だけでなく通常DEF6を持つ。状態付与・吸収等の固有効果付き確率攻撃はこの21種に含めない。

`glk`（13033–13037行）はhitRateが正ならfalseを返し、`ip`（9523–9530行）が行う通常攻撃の初期対象自動選択へ入らない。標準選択はtarget無しだが、主動作のclick設置 `si2` と `aT`、submitの `targetPlayerId` 送信（9639–9642行）は一般経路として残る。したがって公開clientだけでは、全敵展開・明示target入力の禁止またはserverでの扱いを確定できない。

`ev`（9501行付近）は追加isPlusAtkをbaseがweaponsかつhitRate0の場合に限る。確率武器への通常boost/setter/Aura追加は許可しない。直前MCへのFREEは先行特例で、確率奇跡→FREEも構造上選べ、完成列の `la` で費用を検査する。

`hit`（10930–10955行）は受信event.playerIdへ表示targetを向けて命中演出/応答演出を行い、`miss`（10956–10963行）は同じく受信targetへ外れの演出だけを行う。`nextAttack`（10881–10888行）は受信したattackを `p3` でdecodeし `fv` で表示する。ここには対象queueの生成・命中乱数・確率端点・途中dead skip・補充順は無い。reflect/bounceは元aggregateのhitRate表示を保ち、再命中抽選するclient処理も見えないが、serverの再抽選無しまで独立照合したわけではない。

v6はtarget無しのplayだけを受理し、launch時aliveの敵を入力順に固定する。自己と同非Soloチームを除き、処理時deadなら無draw skip、生存者へ有界100抽選1回の `roll < hit_rate` を適用する。miss応答無し、hitだけresponse、redirect再抽選無しとし、原発消費/MC費用/used保持は1回。自然終局→cutoffは受理コマンド末尾で判定し、自動miss/skipはdecisionを増やさない。launchは最初hitまたは全missまでdrain後に判定し、response完了後は未打切りの場合だけ次対象を抽選する。origin死亡なら残り取消、補充はbatch全完了後とする。これらは**公式server未確認の実験規約**で、top-level `chance_rule='experimental:targetless-all-alive-enemies-in-input-order;darkcloud-forces-without-draw;roll-before-response;redirects-no-reroll;skip-dead;cancel-on-origin-death'` に明示する。

`e1` はhitRateを条件にしないため、属性消去でNEになった命中済み確率武器には21/38/233がmatchingする。旧版の％武器への反射制限を流用しない。このv6のchance専用restoreはv7で全pending play用ActionBatchへ統一し、launch前state/RNG/counters/commandと受理response journalから内部executorを再生して、途中でHP/deadが変わるbatchやcounterも検査する。将来roster/cursor/anchor/journal/RNGは公開観測へ出さない。

## HP吸収・同被害：一次算術とv9の実験規約

公式JSONでgiftRate>0のabsorbHPは27（NE武器ATK7/p10）、45（NE武器ATK12/p15）、98（木武器ATK3/75%/p10）、216（光奇跡ATK10/MP10）の4種。dealSameDamageは53（NE武器ATK14/p1）。全てgiftRate1で追加5/重み5、v9は194モデル/重み381、category内訳weapons93/armor72/sundries9/miracles20となる。

JA/EN説明は「HPを吸収する」/“Absorb HP”と「使用者に同じダメージ」/“Get the same damage”まで。現行JSの `dealDamage`（11247–11260行）はevent.playerId（0なら現在target）とevent.damageでHPを減らし、`boostHP`（11275–11289行）も同じplayerId規約でevent.hpを加える。HP0〜99clipとdieが別イベントなのは一次確認済みだが、両能力のraw量・適用順・Dark時の省略を算出するclient分岐は無い。

参考Flashの固定 `turn.inflictDamage` は正残余→Dark別処理、それ以外でsource吸収回復→source同被害→target被害の順。v9はこれを参考に、正nonDark残余rawを現在sourceへ先にhealまたはdamageし、続いてtargetへrawdamageする**実験規約**とする。自己aliasでも各段階をclipし、吸収HP95/raw12は99→87、同被害HP40/raw14は40→26→12、HP10/raw14は10→0→0。source/targetのdead確定は全body後である。

counterはtargetdamage段階だけの実HP減少と全body後survivorを基準にし、source側の副作用や自己net変化からは新counterを作らない。派生counterは元吸収/同被害を継承しない。miss/block/redirect/残余0では副作用を適用せず、Dark bodyも通さない。ただし現scopeでDark吸収/53へ合法に合成する経路は無く、Dark guardを到達可能例としない。

98は木75%のtargetless chanceを使い追加不可、216は光ATK10/MP10の保持/FREEを使う。新RNG/MP予算は不要。公開damage_effect_ruleは `experimental:positive-nondark-remainder;current-source-side-effect-before-target-damage;counter-basis-target-damage-only`。HP primitive以外の量・順序・死亡/反撃接合は現行server未照合と明示し、旧仕様の一致を現行公式適合の証拠へ数えない。

v9では状態を実行しない。v10は70/185/224と199/200/237/238を加えて閃光だけを扱う。防御の1枚制限と非病気curseの重複防止には公開clientの根拠がある。実HP損失時の致死を含む付与・明示解除まで持続・counterの生存条件/FIFOは実験規約とする。解除4種も閃光だけの閉じた状態集合で実行し、他の病気や霧/夢へ暗黙に対応しない。詳しくは[限定実行器のv10契約](official-execution-subset.md#v10の閃光と解除)を参照。

## 暗雲：公開説明・表示とv11規約

41はNE武器ATK11/addCurseOnDamage暗雲/p15w1、186は闇armor/ATKDEF0/counterCurse暗雲/p10w1、223は闇miracle/power0/addCurse暗雲/MP5w1。v11はこの3種を加え204モデル/重み407（95武器/74防具/11雑貨/24奇跡）となる。186は `fv` のcounterCurse分岐でcurseを動的表示するが、`gf3` はpower0のarmorをattackと判定しない。闇即死のdamageではなく、普通DEF/filter等は不可で190万能反射だけがmatchingする。

JAの暗雲説明は「全体攻撃を受けると必ず命中する」、ENは “Received %ATK attacks hit certainly”。`hit`（10930–10955行）は受信済み対象の暗雲を参照して不可避演出を選ぶ（viewerの霧による表示秘匿条件もある）。`nextAttack` はserverから受け取ったattackを表示するだけなので、無drawか、活性化時点、redirect後の再判定までは一次で確定しない。ENに沿えばWIDE専用へ限定する根拠は無いが、181counterへの適用も独立server trace照合は未了である。

`dA` は非病気curseの再付与を重複させず、通常HP更新やdieは暗雲を自動解除しない。mild説明はCold/Fever/Fog/Flashで暗雲を含まず、allは全curseを対象にする。removeCurses eventはserver指定リストを削除するので、これだけでserverの解除発行順を確定しない。

v11はpercent初期対象と181counterの活性化時に暗雲なら無drawforcedhit、元hit_rate維持/現在pending.forced_hit真、redirect再確認無しの**実験規約**を採る。Flash→DarkCloudのcanonical順・重複拒否で同居し、mildはFlashだけ/allは両方を解除。再付与idempotent・all解除まで持続・no tickとする。41の正HP被害/致死付与、186の生存counter、223のself即時/他者responseも既存のローカル規約へ接続する。

公開darkcloud_ruleは `experimental:force-percent-hit-without-draw-at-target-activation;redirects-no-recheck;idempotent-until-all-cure;no-tick`、chance/counter_ruleへdarkcloud-forces-without-drawを加える。実draw成功hitsとforcedを分離し、miss=rolls−hitsを維持する。Snapshot11のanchor/現在frameまで完全replayしてRNG非消費を検査し、累計forcedや未来journalを公開viewへ出さない。病気・霧/夢・他curseは未対応として拒否する。

v10検証範囲の訂正：専用native Flash2関数がmainへ未登録だったため、そのcounter FIFO/canonical偽造/late rollback等をv10実行済みとした範囲は撤回する。CTest件数・C API/Python18ケース・大規模benchの記録値は維持し、v11で登録を修正して初実行し、全source/test最終freeze後の検証でもfixture修正無しでRelease/SANとも通過した。 GNU/Clangではofficial_match native test targetに `-Wunused-function -Werror=unused-function` を追加し、同様の未使用テスト関数をcompile errorにする。

## 手札・使用済み奇跡の除去：現行数値とv12規約

公式JA/ENの205夜空のホウキは「神器を三個掃き飛ばす」/“Sweep away 3 artifacts”、206女神の石けんは「起こした奇跡を二個洗い流す」/“Wash away 2 performed miracles”。usedMiraclesNoteは起こした奇跡を売買と夜空のホウキの対象外と明記する。両品はsundries/無属性/ATKDEFMP0・価格10・giftRate1。v12は206モデル/重み409（95武器/74防具/13雑貨/24奇跡）。モデルabilityValueは0で、効果上限3/2は現行説明から派生し、旧版の1枚除去と混同しない。

removeItems（11495–11517行）とremoveUsedMiracles（11518–11544行）はserver指定event.itemsを `ds/kr`（11801行付近）で順に表示する。`kY` はid/modelId/fakeModelId/usedをdecodeし、本人対象だけ手札からidを探して消去する。他者対象でもカード表示は行い、fake時には `cA/bZ` で真modelへのflipも表示する。206はusedモデル欄の同モデルコピーを1個ずつ消す。抽選slot/populationや候補生成・RNGはこのhandlerに無い。dsの経路は除去自身の補充placeholderを作らないが、serverの独立gift発行scheduleは未確定である。

v12は205をused除外の現在手札（未使用MC含む）最大3、206をusedMCコピー最大2へ限定する。毎回inventory順にeligibleを再構築→有界一様抽選→stable eraseを繰り返すwithout replacement、empty0draw/sole1drawとする。自己は使用品消費後に即時、他者はsundry nonattackとして190だけが適合する明示response。これらの抽選・自己順・gift規約はserver未確認の実験条件である。

pending.amount=3/2は実除去数でなく上限。除去gift0でも使用品/190の既予約giftは維持し、empty-no-drawは通常完了gift RNGまで0にしない。206でused223を失ってもDarkCloudを解除せず、既払い費用や使用を巻き戻さない。removal_ruleは `experimental:unused-hand-max3;used-miracles-max2;uniform-without-replacement-in-inventory-order;bounded-draw-per-removal;empty-no-draw;no-removal-gifts`。

最終bodyはatomicで除去途中snapshot無し。pending replayは除去前の消費/反射/RNG/payload上限を検査し、完了後任意rawstateの公式到達履歴を証明しない。決定的replayではbody抽選と後続giftの一致を検証する。公開remove event表示に対してv12〜18ではMatch.observeの履歴投影が欠けていた。v19は直近の解決結果last_removalを追加し、除去modelを公開する。slot/RNG・候補母数・非除去手札は追加しない。

## 除去結果の公開：v19の観測契約

2026-11-02配信JSでもremove handler（11520–11568行）、描画 `dr/kq`（11830行付近）、reveal `bZ`（11784–11798行）、decoder `to`（3508行付近）の対象経路を比較した。識別子正規化後の制御構造/リテラルと手読の一致に限定した照合であり、新JSの全関数やserverの状態遷移を検証したものではない。固定snapshotの行番号は前節の歴史的根拠として残す。

v19はモデル/重みを増やさず214/437（trade1/98武器/74防具/16雑貨/25奇跡）を維持し、除去結果の公開gapを解消する。rulesetは `official-execution-subset-v19`、Snapshot19（旧1〜18拒否）、公開schemaは `official-execution-view-v17`。Match ABI2/command184/config24/status248、catalog/state ABI1・legacy37は維持する。

`last_removal` はnullまたは `{origin,source,target,model,removed_models}`。originは原発者、source/targetは反射後を含む解決時の現在座標、modelは205または206の単一整数ID、removed_modelsは実際の抽選順の整数ID配列で同モデルのコピー重複を残す。最終bodyが解決したときだけ作り、候補0の解決も空配列[]のeventを残す。使用品自体の消費や後のgiftをremoved_modelsへ混ぜない。

次のmain commandの受理で消去し、不正/失敗commandでは保持する。response処理自体は消去契機ではない。v19〜24の205/206は単一bodyのaction完了時にeventを作り、response phaseとeventは同居しなかった。v25以降の後続death workは別に扱う。reset/load_sceneで消去する。解決後のcutoffではeventを残し、未解決でcutoffした場合はnullのままとする。token/slot・RNG・元eligible数・非除去の他者手札を公開しない。これは全履歴の蓄積ではない。

公開 `removal_event_rule` は `experimental:resolved-body-only;public-removed-models-in-draw-order;duplicates-and-empty-preserved;clear-on-next-accepted-main;no-tokens-or-rng`。他者が対象でもeventを受信したviewerに除去品を表示する経路には公開clientの根拠がある。ただし全参加者/観戦者へのserver配信や空結果eventの必ずの発行は確認していない。ローカル抽選順をevent順とすること・履歴寿命・cutoffの保持は**公式server未検証の実験規約**である。配布プール、除去抽選、資源/死亡/gift/手番の遷移はv18から変更しない。

復元時はdecisions>0、全endpointが既知、model205/206、countが各3/2以下、対応モデルIDだけ、206なら奇跡のみを検査する。v19〜24ではresponse phaseのeventも拒否していた。完了eventとlast_danger/last_sale/last_purchaseの同居は拒否する。完了raw snapshotは構造整合を確認するだけで、その公開結果が過去の公式またはローカル対局で実際に起きたことまで証明しない。pendingはevent無しを要求し、既存の全action replayを維持する。

v19は全source/test最終freeze後のfull Release CTest129件/21.01秒・ASan/UBSan89件/51.62秒が通過した（各 `-j4` 同時、rolloutとの競合あり）。最終Release/SAN full buildはwarning/error0。focused Release3件/1.27秒（native0.46/C API0.02/Python0.79秒）・SAN2件/1.18秒（native1.03/C API0.15秒）が通過した。native零引数55関数のmain明示登録/unused guard、Python AST38ケース、Release/SAN双方の `fixed_query_probes==384` 完走を確認した。公式配信assetは上記の差分確認であり、fresh hash一致とは記録しない。

| v19人数 | 試合数 | 意思決定数 | 遷移checksum | 解決済み除去event | 空結果event | 除去event checksum |
| --- | --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,839,935 | `4941009284767353250` | 10,194 | 4,120 | `16555731515668663194` |
| 9 | 10,000 | 2,428,289 | `11062027547654796276` | 7,817 | 2,655 | `13048935337781861835` |

seed42の2人100,000試合/9人10,000試合を各1 worker/4 workersで実行し、worker数・時間以外の全fieldが一致した。v18 baselineともruleset・時間等の計測fieldを除く既存fieldがchecksumまで全一致し、除去結果の公開追加が遷移/抽選/既存集計を変えないことを確認した。全試合の終了snapshotを復元できた。`removal_events=hand_removal_resolved+used_removal_resolved` と公開removed_modelsの累計数が実除去コピー数に一致するguardも通過した。空結果は0候補でも解決したbodyのeventで、未解決cutoffを含めない。Release/SAN seed42各1,000試合も時間以外の全fieldが一致し、v18の既存非時間fieldとも一致した。遷移checksumは2人 `1463939626686470704` / 9人 `375287159278559213`、新除去event checksumは2人 `17881167250890207477` / 9人 `11841601995346163861`。今回はquiet速度を再計測せず、v19の速度値や改善倍率を主張しない。

詳細は[限定実行器v19記録](official-execution-subset.md#v19の過去の検証と公開除去結果)。

## 病気・閃光・暗雲

公式 `texts.reference.diseasesNote` は、病気の自然悪化を毎ターン5%と説明する。`diseaseUpgradeNote` は病気を重ねると悪化すると説明する。旧版の初期0%から増加する `worseChance` を公式実装と同一視できない。**C02の公開仕様上の差は確定**したが、5%抽選のタイミング、重複時の比較規則、乱数端点、複数人時のtick対象は公開文言だけでは未確定。

`texts.curseDescriptions` の継続量は風邪1、熱病2、地獄病5の被害、天国病5の回復。天国病の悪化はHP0と説明される。閃光は防御時の使用神器が1つに制限される。クライアント `ev`（9478–9481行）は閃光がある応答で2枚目の選択を拒否する。これは旧bot由来の制限しかなかった状態より強い現行一次根拠である。

暗雲はja.jsonでは「全体攻撃」、en.jsonでは `%ATK` の受け手が必中になる説明である。文言だけから追加魔法WIDEに限定してはならない。JSの `hit` イベント処理（10934–10948行）は対象の暗雲を参照し、命中演出を分ける。命中を決めるサーバー抽選や、必中時に乱数を消費するかはこの表示処理から確定しない。

## 病気4状態：公開説明とv22の実験規約

2026-11-02配信JAはCold−1/Fever−2/Hell−5/Heaven+5、毎turn自然悪化5%、病気へ病気を重ねる悪化とHeaven悪化時HP0を明記する。現行JSの `geY/dz`（13270–13281行）は病気一種を取り出し、新しいevent.curseに従い旧病気を除去/置換する。非病気Flash/DarkCloudを同じ病気置換へ混ぜない。

`disease`（11401–11427行）、`upgradeDisease`（11428–11442行）はevent.playerId/curse/natural等を受けて演出を分け、`upgradeHeaven`（11443–11451行）も演出である。数値HPの変化は別resource eventであり、このhandlerだけではowner限定tick・悪化後の継続効果・208救済・gift・自然終局/cutoffの順を確定しない。旧FlashがHeaven中の非病気付与も致死へ扱う処理を、そのまま現行へ移さない。

v22はonDamage武器30/33/48/62と状態奇跡219/220の六モデルを加え、222モデル/重み483（trade3/weapons102/armor74/sundries16/miracles27）とする。病気はCold/Fever/Hell/Heavenの4状態。v22時点では168のselfCurseと198のboostMPAndAddCurseは未対応で、関連八モデルを一括実装していなかった。rulesetは `official-execution-subset-v22`、Snapshot22（旧1〜21拒否）、公開schemaは `official-execution-view-v20`。Match ABI2の幅・opcode0〜8・purchase phase3/finished2、catalog/state ABI1・legacy37は維持し、学習器は追加しない。

一人の病気は一種類とし、Flash/DarkCloudとは独立に持つ。病気が無ければincomingを付け、既存病気があれば `max(incoming_rank,current_rank+1)` へ進める。既にHeavenへ病気を再付与するとHP0に落とし、Heaven状態を保つ。Flash/DarkCloudの付与はこの病気再付与と見なさず、Heavenfallを起こさない。これらの厳密なrank/落下処理はローカル実験規約である。

30はNE/ATK8/Hell、33はNE/ATK9/Cold、48はNE/ATK13/Cold、62はStone/ATK1/Coldで、正の通常target HP実被害に応じて既存onDamage経路で付与する。219はCold/MP6、220はHeaven/MP15の状態奇跡でused保持・FREE・適合する反射/遮断/跳ね返しを扱う。再付与でのHeavenfall分をcounter基準へ足さず、既存のregular target damage段階の実損失だけを使い、208救済後に生存していればsurvivor counterの対象となる。

mild解除199/237はCold/Fever/Flashを取り、Hell/Heaven/DarkCloudを残す。all解除200/238は対応する病気4種とFlash/DarkCloudの全六種類を対象とする。霧/夢まで新たに実行可能にする意味ではない。

元ownerの全actionが完了してadvanceへ入る冒頭でだけ病気tickを行う。ownerが既deadまたは病気無しならdraw無し。対象ならbounded100を一度引き、roll<5で一段悪化→変化後のCold HP−1/Fever−2/Hell−5/Heaven HP+5を適用する。Heavenからの悪化はHP0へのfallで、そのtickのhealはしない。次に208救済/死亡→自然終局→既予約gift→次ownerへ進む。固定5%を採用し、旧FlashのworseChance増加式を使わない。

body側の自然終局/decision cutoffが先に成立した場合はadvance/tickへ入らない。完成bodyでのcutoffでもtick無し。反射・連撃・全体攻撃の各frame間、counter FIFO途中、BUY見積中にはtickしない。BUYのaccept/decline等で全actionを完了した場合の対象は現在buyerではなく元ownerである。ownerが生きたまま同action中に208救済された場合も、この全action完了の境界で判定する。これらの対象・抽選順・境界・gift scheduleは公式server未確認の実験規約である。

nativeの特権 `last_tick` はHP/病気before-after・worsened/fallen・revival_tokenを記録し、body被害/202統計・除去・tickによる208消費を分離する。内部tick record・token・RNGを公開JSONへ加えず、公開するのはdisease_ruleとboardの病気/資源状態だけ。完了raw snapshotの構造検査は全公式履歴の到達証明ではない。

v22/23時点の公開 `disease_rule` は `experimental:one-disease-plus-flash-and-darkcloud;reapply-max-incoming-or-next;heaven-reapplication-hp0;owner-only-after-whole-action;natural-and-cutoff-before-tick;fixed5-percent-before-hp;post-upgrade-tick;post-tick-revival-before-gifts`。v22/23のraw stateは病気最大一種→Flash→DarkCloudのenum順・重複無し、最大三curseのcanonical集合だった。v24のFog拡張は後述する。tick recordは次main受理でclearし、現在の公開boardから過去のroll/tokenを復元して返す機能は作らない。 現行clientの病気表示は新たな受信curseで旧病気一種を置き換え、非病気Flash/DarkCloudは独立に残す。旧FlashのHeaven中に非病気を付けても転落させる挙動は流用しない。

ベンチの `disease_ticks` / `disease_worsened` / `disease_falls` はtick/自然悪化/Heavenfall件数、`disease_tick_revivals` はtick由来208救済数。`disease_hp_gained/lost` は復活前のtick HP増減を集計し、救済時はrawAfter=0として208の+10を除き、Heavenfallの実HP損失も含める。累計 `revivals` はbody由来とtick由来を含む。HP増減202のbody集計からtick HPdeltaとtick救済を外し、205/206のtoken消失数から同targetのtick208消費を除いて公開除去eventと照合する。これらの内部会計のためにlast_tick/tokenを利用しても、公開JSONには出さない。

v22の最終検証でRelease CTest129件/15.02秒・ASan/UBSan89件/44.60秒が通過した（各 `-j4`、大規模benchは先に完了済み）。最終Release/SAN full buildはwarning/error0。focused SAN2件/1.26秒（native0.90/C API0.37秒）が通過し、ReleaseはC++2件（native0.35/C API0.20秒）と、PythonのWideサンプラassert修正後の単独1件/0.61秒、その後のfull suite通過を記録する。Release focused総3件の合計時間は掲載しない。SANではPythonが対象外で、最終C++テスト確定後に検証した。native零引数68関数のmain各1回登録/unused guard、Python AST48ケースの重複無し、Release/SAN双方の固定query384完走を確認した。本体コードは追加変更していない。suite所要時間を版間の速度比較へ使わない。

| v22人数 | 試合数 | 意思決定数 | 勝数 | draw / 打切り | checksum |
| --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,992,222 | 49,802 / 50,088 | 110 / 0 | `14285408868039593824` |
| 9 | 10,000 | 2,539,475 | 1,051 / 1,146 / 1,110 / 1,100 / 1,096 / 1,116 / 1,152 / 1,102 / 1,104 | 20 / 3 | `12415748028992245618` |

| 人数 | tick | 自然悪化 / Heavenfall | tick208救済 / 全蘇生 | tick HP増 / 減（復活前） |
| --- | --- | --- | --- | --- |
| 2 | 135,265 | 6,673 / 826 | 245 / 4,427 | 83,246 / 230,930 |
| 9 | 95,831 | 4,866 / 1,013 | 233 / 4,355 | 89,891 / 168,855 |

seed42の2人100,000試合/9人10,000試合を各1 worker/4 workersで実行し、worker数・時間以外の全fieldが一致した。全終了snapshot（自然終局/打切り）を復元し、新旧会計boundsを確認した。Release/SAN seed42各1,000試合も非時間fieldが一致し、2人checksum `11520684194578492807`（tick1,245/tick救済1）、9人checksum `17074919555374801115`（tick9,187/tick救済27）。自然悪化の集計はローカル実験乱数の観測結果であり、公式serverの確率測定ではない。今回はquiet速度を再計測せず、v22の速度値や版間改善倍率を掲載しない。

詳細は[限定実行器v22記録](official-execution-subset.md#v22の過去の検証と病気tick集計)。

## 自己病気とMP/Heaven：168/198とv23規約

2026-11-02配信clientの `eu`（9500–9522行）と固定 `ev`（9475–9496行）は168を通常armorとして扱い、主動作の役割を与えない。属性が適合する普通DEF/counter防具とは組み合わせられるが、特殊反応は先頭または直前filterに限るため168との併用は不適合。Flashは一選択へ制限する。pure utility/status/tradeを普通DEFで受ける選択にはしない。

`selfCurse`（現行11614–11630行/固定11589–11605行）はevent.indexの選択responseカードを光らせるだけで、curse/HP/deadを書き換えない。病気は別addCurse/upgradeDisease、HPや復活/死亡も別eventなので、自己付与と被害/208のserver順は確定しない。198のtooltip（現行9402行/固定9377行）はMP20/Heavenを示すが、boostMP（11319行付近）とaddCurse（11368行付近）は独立した受信処理であり、tooltipの並びだけでは実行順を証明しない。

旧FlashのMP増加→addHarmや各DFS時の病気付与は補助資料に限定する。旧guardの弱いspecial prefix受理、Heaven中の非病気付与への致死、増加式の自然悪化は現行へ移さない。

v23は168熱狂仮面（Fire armor/普通DEF10/selfCurse Fever/価格10/giftRate1）と198天国草（NE sundry/MP+20とHeaven/価格20/giftRate1）を加え、224モデル/重み485（trade3/weapons102/armor75/sundries17/miracles27）とする。MP費用/guardianはどちらも無い。rulesetは `official-execution-subset-v23`、Snapshot23（旧1〜22拒否）、公開schemaは `official-execution-view-v21`。Match ABI2の幅・commands0〜8・purchase phase3/finished2、catalog/state ABI1・legacy37を保持し、学習器は追加しない。

168はmain使用不可の普通防具で、属性適合を満たせば同copy複数を順序付きで使える。Flashなら一枚だけ、leading filter→168→普通防具は可能、168と190/壁/跳ね返し等の特殊反応は同じ列に併用できない。通常の全response消費後、選択順に各168の自己Feverを一回ずつ付ける。同modelだからと副作用をdedupせず、完全防御でincoming被害0でも自己付与する。

その後incoming bodyを適用し、全body後にだけ208救済/死亡を確定する。prefixでHeavenfallによりHP0になっても本体前に蘇生させず、HP0&&!deadは同じtransactional step内だけに留める。応答待ちsnapshotへHP0を許すように制約を緩めない。counter基準はincoming bodyのtarget HP実損失だけで、prefix落下分を足さない。ただし自己HP吸収aliasならprefix HP0→source回復→target被害という順で正のbody損失が発生し得るため、prefix落下後のcounterが常に0とはしない。最終survivorは既存208救済後に判定する。

198はmainのplay専用でresponse品には選ばず、MP+20を先に0〜99clipしてからHeavenを付与し、全body後に救済/死亡を処理する。既にHeavenならMPは増加した後に再付与fallが起こり得る。自己は即時、他人はnonattack utilityとして空応答または190万能反射だけ、反射後の現在targetへ適用する。FREE/追加品・保持化・MP費用は無く、能力自体のRNGも0。全action後の病気tick/gift RNGは別である。

168/198をdiscardや売買商品として消費/移動しても、この新abilityを発動しない。商品として届いたことから自己FeverやMP20/Heavenを付けず、guardianを召喚しない。既存の一病気+Flash/DarkCloud、mild/all、固定5%/元ownerの全action完了tick、自然終局/cutoff優先、208/gift順はv22を維持する。

公開 `disease_item_rule` は `experimental:response-self-curse-in-selection-order-after-consumption;prefix-before-body;death-and-revival-after-whole-body;counter-basis-body-target-loss-only;mp-boost-before-disease;no-trigger-on-trade-or-discard`。clientに根拠がある普通防具の選択/属性・selfCurse選択indexの演出・198のMP20/Heaven表示と、消費後の順序/暫定HP0/208/body/counter/死亡を接合する**公式server未確認の実験規約**を区別する。失敗時は状態/RNG/出力を含む全stepをrollbackし、新たな副作用history/token/RNGを公開しない。 198は全response後のbodyでMPを増やし、nonattackへの反応は190だけなので同batchの有償MP反応予算を増やさない。v23/24時点のjournal上界は非BUY3,725/BUY3,726だった。v25のdeath追加分は後述する。新ベンチ `fever_mask_uses` は応答で使った168コピー数、`heaven_herb_plays` は198のmain開始数で、捨てる/商品移動を含めない。`heaven_herb_resolved` は自己即時または他者空応答の最終body解決数（190反射途中は除く）、`heaven_herb_mp_gained` は当該targetのMP after-before実増分で、99cap後の値を数える。同body後のtick/giftはMPを変えないため分離でき、名目20倍値に置き換えない。resolved<=plays/実増<=20*resolvedを検査する。未解決cutoffなら開始だけ、解決済みcutoffならbody実増も計上する。

v23は全source/test最終freeze後のfull build・focused・full suiteで、Release CTest129件/19.51秒・ASan/UBSan89件/49.90秒が通過した（各 `-j4` 同時、大規模benchは先に完了）。最終Release/SAN buildはwarning/error0。focused Release3件/1.29秒（native0.08/C API0.26/Python0.94秒）・SAN2件/1.05秒（native0.61/C API0.44秒）が通過した。native零引数72関数のmain各1回登録/unused guard、Python AST51ケースの重複無し、Release/SAN双方の固定query384完走を確認した。

| v23人数 | 試合数 | 意思決定数 | 勝数 | draw / 打切り | checksum |
| --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,965,139 | 49,606 / 50,283 | 111 / 0 | `4864183686981653755` |
| 9 | 10,000 | 2,519,891 | 1,099 / 1,104 / 1,079 / 1,137 / 1,090 / 1,131 / 1,099 / 1,184 / 1,063 | 14 / 0 | `12279515150450788112` |

| 人数 | 168使用コピー | 198開始 / 最終解決 | 198実MP増分（99cap後） |
| --- | --- | --- | --- |
| 2 | 4,471 | 4,547 / 4,547 | 90,931 |
| 9 | 3,552 | 3,367 / 3,367 | 67,336 |

| 人数 | tick | 自然悪化 / Heavenfall | tick208救済 / 全蘇生 | tick HP増 / 減（復活前） |
| --- | --- | --- | --- | --- |
| 2 | 183,727 | 9,283 / 2,472 | 367 / 4,554 | 229,225 / 355,557 |
| 9 | 136,022 | 6,822 / 2,336 | 339 / 4,393 | 212,748 / 265,590 |

seed42の2人100,000試合/9人10,000試合を各1 worker/4 workersで実行し、worker数・時間以外の全fieldが一致した。全終了snapshot（自然終局/打切り）を復元し、新旧会計boundsを確認した。Release/SAN seed42各1,000試合も非時間fieldが一致し、2人checksum `15297651180555030086`（168コピー45/198開始36/実MP720）、9人checksum `16876268204354222509`（168コピー350/198開始352/実MP7,040）。この大規模seedで198開始数と解決数が一致したことを、未解決cutoffが起きない保証にはしない。公式serverの病気順や乱数の測定結果ではない。今回はquiet速度を再計測せず、v23の速度値や版間改善倍率を掲載しない。

詳細は[限定実行器v23記録](official-execution-subset.md#v23の過去の検証と仮面天国草集計)。

## MP武器と二回攻撃：公開計算とv13規約

58マジカルステッキは無属性weapon・基礎ATK0・atkBy2xMP・価格10/giftRate1、14のこぶんぶんは無属性weapon・ATK3・attackTwice・価格10/giftRate1。どちらもisPlusAtkを持たず追加品としては使わない。v13は計208モデル/重み411（97/74/13/24）となる。

JSのpreview `dt`（9544–9557行）は完成モデル列の `la` 費用を自己MPから引き、`2*max(0,MP−cost)` を `aR` の基底overrideへ渡す。`dA.iJ`→`tw`（12280行付近/3971行付近）が後続boost・Auraを順適用し、FREEは攻撃威力/属性から除く。useAttackItems（10736–10749行）も完成列の費用を先に減算し、残MP×2で `bF/kn`（11774行付近）を再表示してMP表示を0にする。旧Flash MAGICALの「各有料MC後に途中威力reset」とは異なる。

例えばMP10で58→boost3→有料Aura6なら基底8から22、58→Aura6→boost3なら19。58→Aura→FREEなら費用0・基底20から40。MP0でもweapon categoryなので `gf3` はattackと判定し、boost3を加えれば威力3になる。MCを追加してもweapon categoryを保ち、現在属性に応じた物理反応の対象となる。反射表示は元aggregateの動的overrideを保つが、権威的serverの全会計を証明するものではない。

v13はlaunch時の元MPを費用込みで一度だけ全支払いし、完成列費用後の残MPをimmutableな基底にする。redirectや後続MP counterで再計算しない。`mana_weapon_rule` は `experimental:base-2x-mp-after-composed-miracle-cost;ordered-boosts;consume-all-mp-once-at-launch;redirects-no-repayment`。

14はJA「二回攻撃する」/EN “Attack twice” までが説明上の根拠で、公開JSのnextAttackは受信attackを表示するだけで二発の権威的scheduleを計算しない。固定旧Flashのturn/AttackData/全364行にも同対象2hitのrepeat処理は見つからず、DOUBLE_ATKは威力倍化なので代用根拠にならない。
v13は同じ指定targetへ原発payloadをfreshに二度作り、消費・MPは一度、counter FIFOを次hitより先、dead target skip・origin死亡で残り取消、自然終局/cutoff優先、補充は全action後とする**実験規約**を採る。filter/redirectは当該hitだけで、6damage一括ではない。`repeat_attack_rule` は `experimental:two-fresh-payloads-to-same-target;single-consumption-and-payment;counter-fifo-before-next-hit;dead-target-skip;origin-death-cancels-rest;gifts-after-whole-action`。新RNG/MP増加源は無く、Snapshot13/view-v11/C ABI1で公開する。

v13最終source/test freeze後の全Release CTest129件/20.39秒・ASan/UBSan89件/51.58秒、focused Release3件/0.94秒・SAN2件/1.52秒が通過した。Python25ケース・native零引数37関数main明示登録、最終2build/前段3ログwarning/error0、公式assetsのfresh hash/296 facts一致を確認。1/4 workersとRelease/SAN各1,000試合の全決定的fieldも一致した。独立速度と集計は[限定実行器v13記録](official-execution-subset.md#v13の過去の検証と集計)へ分け、公式server同等性や版間速度改善率の証拠とはしない。

## 蜃気楼：全敵説明とv14の実験規約

232＜蜃気楼＞/Mirageはmiracles・isPlusAtk・attackEveryEnemy・cost5・giftRate1。欠損するatk/hitRate/element/priceは台帳decoderで0/0/NE/0となる。JA/ENは全敵への攻撃と説明する。`gkN`（13024行付近）はisPlusAtkかつATK0の奇跡の単独開始を拒否し、`ev`（9478行付近）は初品weaponかつhitRate0の列への追加を認める。別のMC基底や確率武器には追加しない。

`tw/l9/la`（3971–4007行）は232を通常のATK+0/NE合成として扱う。有色→232でNE、232→後続setterならその色へ戻る。14/58やAura・FREEとの順序も同じ集計であり、各232の即後FREEだけがそのMP5を免除する。複数の別copyは各々課金/保持される。現在同token重複禁止は維持し、後のcommandでの再使用と分ける。

`ip`（9520行付近）は先頭1品を選んだ時だけ `glk` でtargetを自動選択し、232を追加してもtargetを消す処理は無い。submit（9639行付近）はtargetが非nullならtargetPlayerIdを送る。受信setTargetPlayer（10910行付近）の `A.kr` は232の存在を検出し、霧の演出分岐を除外するだけで、候補/順序やserver抽選を生成しない。`dR/ce` はalive敵の表示集合を作るが、それだけでserver実行順を証明しない。

v14はtarget無し/任意の既知alive targetを受理し、指定はrosterに影響しない表示用anchorとして扱う**実験規約**を採る。launch時alive敵を入力seat順に一度固定し、14があれば全roster二巡（A,B,A,B）、各payloadはfresh、58の基底MPはlaunchから固定、消費/支払い一度とする。複数232で対象をさらに増やさず、chance抽選/暗雲forcedを発生させない。旧WIDEのchance100化は流用しない。counter FIFO先行・dead skip/origin死亡取消・全action後giftは維持する。

公開 `wide_attack_rule` は `experimental:initial-alive-enemies-in-input-order;optional-alive-target-does-not-alter-roster;repeat-by-full-roster-pass;single-payment;counter-fifo-before-next-target;dead-target-skip;origin-death-cancels-rest;gifts-after-whole-action`、更新した `repeat_attack_rule` は `experimental:two-fresh-passes-over-initial-targets;single-consumption-and-payment;counter-fifo-before-next-hit;dead-target-skip;origin-death-cancels-rest;gifts-after-whole-action`。最大target entriesは2*(n−1)=16、response boundは最大3,725。Snapshot14は旧1〜13拒否/view-v12/C ABI1、209モデル/重み412（97/74/13/25）。target入力のserver受理、無draw、全体scheduleは未確認である。

v14は全Release CTest129件/19.09秒・ASan/UBSan89件/50.23秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、Release/SAN/bench/SAN bench/bridgeの5ログはwarning/error0。focused Release3件/0.81秒（native0.32/C API0.01/Python0.47秒）・SAN2件/1.17秒（native0.83/C API0.34秒）が通過した。Python MatchはAST確認27ケース、native零引数40関数は全てmainへ明示登録を確認した。公式assetsのfresh取得hash/296モデルfactsも一致した。 独立速度と大規模集計は[限定実行器v14記録](official-execution-subset.md#v14の過去の検証と集計)へ分ける。

## ドキドキ涙：増減表示とv15規約

202ドキドキ涙/Thump-thump Tearはsundries・boostHPOrDealDamage・abilityValue10・価格1・giftRate2、欠損属性はNE、ATK/DEF/MP/率0。JA/ENのカード表示は `a4`（9376行付近）がboostHPとorDealDamageを組み合わせるもので、HP+10または10damageまでを説明し、50/50は明記しない。このability文字列は固定JSでは表示箇所だけに現れ、抽選処理はない。

`p3`（3653行付近）はattack.atk/hitRate/mp/cp等をdecodeするが、202固有のsigned HP結果欄は無い。`boostHP/dealDamage`（11247–11301行付近）は受信eventが指定する人物、または現在targetへ `eW` のHP加減/0〜99clipを適用する。これだけで符号がいつ決まり、応答前に何を公開するかは確定しない。汎用attack.atk等に動的結果が届く可能性も排除できず、公式では必ず符号非公開とも断定しない。`eW` はdeadを自動設定せず、`die` は別の受信eventである。

現在のモデル列を `gf3` へ渡すとATK0のsundryはnonattackとなり、`ev/e1` では190だけが適合する。単独mainは `gkN` が許す一方、追加役割/isPlusAtk/MC費用は無く、追加boost/FREEへは使わない。ただしserverが負の結果を動的attackとして表現する場合の実際の受理は未照合である。

旧Flash237はlaunch時にrandrange(0,2)で±10を固定し、COMMAND.hpで応答前公開、clone反射で保持、最終targetへ一度適用する参考実装を持つ。これは現行一次資料ではなく、旧0damage/1healのbit対応もv15の0heal/1damageとは区別する。旧244蘇生や病気tick順を現行へ流用しない。

v15はlaunch bounded2を一度引き0で+10/1で−10、power0の固定signed amountを応答前に公開する。self即時/他者response、190反射で符号維持・再抽選無し、負でもnonattackでDEF/filter/counter無し、最終targetのHPだけ0〜99clip、全body後dead/自然終局優先/全action後giftとする**実験規約**を採る。`random_hp_rule` は `experimental:launch-once;uniform-plus10-or-minus10;zero-heals-one-damages;fixed-through-reflections;nonattack-no-defense-or-counters;hp-clips-0-99;gifts-after-action`。210モデル/重み414（97/74/14/25）、Snapshot15旧1〜14拒否/view-v13/C ABI1で、符号前公開を公式確認済みの観測仕様と呼ばない。

v15は全Release CTest129件/18.87秒・ASan/UBSan89件/49.59秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、release/sanitize/bench/SAN bench/native/bridge-fixの6ログはwarning/error0。focused Release3件/0.63秒（native0.08/C API0.01/Python0.53秒）・SAN2件/1.00秒（native0.68/C API0.32秒）が通過した。Python MatchはAST確認29ケース、native零引数43関数は全てmainへ明示登録を確認した。公式assetsのfresh取得hash/296モデルfactsも一致した。 独立速度と大規模集計は[限定実行器v15記録](official-execution-subset.md#v15の過去の検証と集計)へ分ける。

## 太陽のお守り：HP0と脱落の境界

現行208はsun-amulet/sundries/revive/abilityValue10・価格10/giftRate1。JA/ENはHP0になるとHP+10、教典は手動discard禁止と説明する。`gkN`（13024行付近）もreviveのmain開始を拒否し、`ev`（9499行付近）はdiscard対象から除く。ATK/DEF/追加役割は無く、response品としても使わない。

revive handler（11617–11649行）は受信event.itemのinstance idを本人手札で選び、`d_`→`ee(null)`（12743/12694行付近）で通常の物理消費と補充用空枠を作る。fakeならflip表示もある。どのcopyかは受信itemで既に決まっており、inventory最初を選ぶ抽選/探索をhandlerが行うわけではない。handler自体はHP加算やdead取消をせず、boostHPとdieは別の受信event。実gift発行数・HP0の発火時点・複数瀕死/終局との順は未確定である。

[旧244調査](legacy-revival-rules.md)は固定seat順、各時点のnot.dead人数2以上、最初copy一枚、直接discard/gift0を示す。両HP0でseat1だけ244なら救われない非対称も旧実装の挙動であり、現行一次根拠ではない。旧版は手動discard可なので、その合法性も現行へ移さない。現行死亡時攻撃は112昇天弓で、旧117と別ID。revive/die/attackDyingly各dispatchのコード上の並びは、serverの発行順を示さない。

v16は全body後に入力seat順でHP0&&!deadを調べ、各時点not.dead>=2なら最初208一枚を消費/HP+10/1gift予約、失敗ならdead確定とする**実験規約**を採る。1境界1人1copy、次のbodyで再びHP0なら残りcopyを使える。既deadを復活させず、MP/CP/usedMC/curseを変えない。giftは全action後まで遅延、terminal/cutoffなら取消す。targetの実損失は復活前の被害段階を保ち、復活後survivorならcounterを許す。

公開 `revival_rule` は `experimental:post-body-input-order;hp0-and-not-dead;two-not-dead-required;first-copy-once-per-boundary;hp-plus10;one-deferred-gift;counter-basis-pre-revival-loss;passive-and-undiscardable;full-hand-prayer-no-draw`。武器が無い時の祈り選択は公開clientと既存selection_readyに沿って満杯でも許すが、満杯gift0/環境RNG0・手番進行はローカル規約である。211モデル/重み415（97/74/15/25）、Snapshot16旧1〜15拒否/view-v14/C ABI1。privilegedな累計revivals/anchor/tokenは観測へ出さない。202のHP増減集計は復活healを除いたbody増減を使い、負の202から復活してnet正でもdamage分類を保つ。

v16は全Release CTest129件/17.77秒・ASan/UBSan89件/47.15秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、release/sanitize/bench/SAN bench/native/bridgeの6ログはwarning/error0。focused Release3件/0.62秒（native0.08/C API0.01/Python0.53秒）・SAN2件/1.21秒（native0.90/C API0.31秒）が通過した。Python MatchはAST確認31ケース、native零引数46関数は全てmainへ明示登録を確認した。公式assetsのfresh取得hash/296モデルfactsも一致した。 独立速度と大規模集計は[限定実行器v16記録](official-execution-subset.md#v16の過去の検証と集計)へ分ける。

v16検証範囲の訂正：C APIの固定seed照会6本の開始値を誤って275に変更し、条件seed<64へ入らなかったため、閃光解除・暗雲forced・ホウキ・石けん・MP武器・HP増減の各64回、計384照会がv16では未実行だった。この384件をv16で実行済みとした範囲は撤回する。全Release129件/SAN89件の通過・他assert・native/Pythonの相当照会・大規模計測値は事実として保持する。v17では6loopを `seed{}`（0）へ戻し、`fixed_query_probes==384` の完走必須assertを追加した。Wide+FREE探索も開始275から0へ戻すが、こちらはv16でも275以降で探索を実行していた。v17の修正後Release/SANで384照会完走を再検証して通過した。過去のv16検証範囲を遡って増やさない。

## 杵と臼：現行99damageとv17規約

94あぶないキネ/Dangerous Pestleは光weapon/ATK30/danger/価格5/giftRate1。209あぶないウス/Dangerous Mortarはsundries/attractDanger/abilityValue99/価格1/giftRate1。JA/ENは誰かへの攻撃と、臼がキネから99damageを受けることを説明する。旧99/245の999・臼価格0とは異なる。

`attractDanger`（10970–10999行）はserver指定event.itemをtarget側paneへ置き、本人ならinstance idを選択→`d_`→`ee(null)`で臼を通常消費し、補充用空枠を作る。`fG`（12407行付近）は防御paneが空の時だけresponse選択へ入るため、臼が既に置かれた表示ではその分岐を通らない。これはUI経路の根拠であり、serverの防御コマンド拒否・実gift数・イベント発行順の証明ではない。209の手動main/discardはclientが拒否する。

`ev`（9478行付近）は94に単独限定を課さず、必中weaponとして普通boost/属性setterや210火の玉(+2/MP2)・217流星(+10/MP7)・231Aura・232Wide、MC直後FREEを許す。`tw/l9/la` は通常合成で、例えば94→火の玉はFire/32、94→AuraはNE/60、94→WideはNE/30。`glk/ip` は最初enemyを自動選択し、submitは選択targetを送り得る。霧演出の条件がdangerを除外しても、候補/抽選順は計算しない。

[旧版杵・臼調査](legacy-pestle-rules.md)の列長1・臼非消費・999・自己応答省略は現行へ移さない。v17はtarget無し/既知aliveヒントを受理してヒントを無視し、launch時のinput順最初のalive臼所持者/最初copyへ固定、いなければ全alive（自己/味方込み）へ一様bounded draw1とする。臼ありは正の基底override99から順序boost/Aura/属性/FREEを合成し、一枚消費/gift1予約、0DEFで応答無しのbodyを解決。臼無しは基底30/通常responseで自己も応答、redirect後の再選択はしない。これらは**実験規約**であり、説明の99が追加後も必ず固定99である/ないというserver確認結果ではない。

公開clientが許す94+232だけは優先規則未確定としてruntime明示unsupported（-5）、他の対応boost/MC/FREEは扱う。209は手動play/response/discard不可だが、205のunused強制除去には含む。`danger_rule` は `experimental:launch-once;first-alive-mortar-in-input-order;otherwise-uniform-all-alive;optional-alive-target-ignored;base99-before-ordered-boosts;consume-one-mortar-with-deferred-gift;attracted-body-no-response;redirects-no-reselection;wide-combination-unsupported`。Snapshot17旧1〜16拒否/view-v15/C ABI1、213モデル/重み417（98/74/16/25）。公開last_dangerは実行済launchのorigin/target/models/mortar/power/elementに限定し、response中も初期選択を保ち次main受理で消す。未選択の臼所持者・token・RNGは返さない。

v17は全Release CTest129件/18.30秒・ASan/UBSan89件/48.50秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、release/sanitize/bench/SAN bench/native/bridgeの6ログはwarning/error0。focused Release3件/0.76秒（native0.32/C API0.01/Python0.43秒）・SAN2件/1.25秒（native0.93/C API0.31秒）が通過した。Python MatchはAST確認33ケース、native零引数49関数は全てmainへ明示登録を確認した。公式assetsのfresh取得hash/296モデルfactsも一致した。修正C APIの6本×64照会はRelease/SANで `fixed_query_probes==384` の完走を確認した。 独立速度と大規模集計は[限定実行器v17記録](official-execution-subset.md#v17の過去の検証と集計)へ分ける。

## 両替：入力範囲とv18規約

現行3両替/Exchangeはtrade/exchange・価格5/giftRate20。JA/ENはHP1=MP1=CP1の交換と説明する。旧2の価格10と区別し、価格5を操作費用として差し引かない。`tj/fo/fp`（3582/11864–11921行）はHP↔MP・HP↔CPを±1/±10で動かし、各0〜99で停止、総和を保存する。HP1から0へ減らす操作も可能。

submit（9635行付近）は配分のmp/cpだけを送ってhpは送らず、exchange event（11080–11100行）は受信hp/mp/cpをsource paneへ直接代入する。従ってtypedな3値を総和検査すれば現行のmp/cpへ一意に射影できるが、serverのHP導出/入力検査の全実装が得られたわけではない。`ev` はtrade基底へ追加品を許さず単品、`la` はMCを含まない列の費用0、`ee` は物理消費/補充用空枠1で、奇跡保持化をしない。sourceの自己配分には表示根拠がある一方、target指定を送る一般経路は残るため、target無し限定をserver入力禁止とは断定しない。

v18は新kind exchange=4、本人所有の3一token、target無し、allocation=(HP,MP,CP)各0〜99/配分前総和保存/同値可を採る。物理消費/gift1予約→直接3値代入→既存finalize_deaths→自然終局/cutoff優先→全action後gift/次手番で、反応・counter・FREE・MPfeeは無い。HP0の自殺/208救済も既存規約へ接続し、総和保存は復活+10の前までとする。配分選択に環境RNGを使わず、samplerはpolicy RNGで可行配分を選ぶ。

`exchange_rule` は `experimental:single-owned-exchange;targetless-self;three-resources-0-99;sum-conserved-before-revival;zero-mp-fee;no-response;one-deferred-gift;death-and-revival-before-gift`。範囲/総和/自己表示と、死亡・復活・gift/手番のserver未確認scheduleを分ける。214モデル/重み437（trade1/98/74/16/25）、Snapshot18旧1〜17拒否/view-v16。Match ABI2のcommandは168→184byteで型付き配分を追加し、呼出前のversion/size検査と旧Match ABI1 clientの移行が必要。catalog/state ABI1・legacy37は維持し、終末専用sacrificeと94+Wideは未対応のままにする。

v18は全Release CTest129件/17.79秒・ASan/UBSan89件/46.30秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、release/sanitize/bench/SAN bench/native/bridgeの6ログはwarning/error0。focused Release3件/1.08秒（native0.29/C API0.02/Python0.77秒）・SAN2件/1.36秒（native1.00/C API0.35秒）が通過した。Python MatchはAST確認36ケース、native零引数52関数は全てmainへ明示登録を確認した。既存C API384照会guardも完走し、公式assetsのfresh取得hash/296モデルfactsも一致した。 独立速度と大規模集計は[限定実行器v18記録](official-execution-subset.md#v18の過去の検証と集計)へ分ける。

## 売却：公開clientの順とv20の限定契約

固定clientと2026-11-02配信clientの対象経路では、SELL4はtrade/MP0/価格5/giftRate20。`ev`（固定9499行付近）は先頭SELLに未使用の商品一個だけを許し、submitは二品を要求する。usedMiraclesNote/used表示は使用済み奇跡を取引から除く一方、武器・太陽のお守り208・臼209を商品から除く条件は無い。選択targetを送ることは確認できるが、serverの自己/味方/不明/死亡target受理を完全に確定したものではない。

最新版 `ed`（12720行付近）はSELLと商品を二品消費し、操作品だけ補充placeholderを作る。未使用奇跡の商品を保持化しない。受信SELL（11123行付近）は現在buyerへのCP減算→現在sellerへのCP加算→overflowItem処理→buyerへの商品表示/配送の順で、BUYの承認phaseとは別経路である。`bc/dH/eW` のCP不足→MP不足→HP、0〜99clipは既存resource primitiveの根拠を使う。

`bb`（12702行付近）は表示枠が18以上なら最初の空placeholderを除き、serverから別途与えられたoverflowItemは `eH` で処理する。実物のeviction候補・選択乱数はclientから確定しない。event.item.idが配送tokenを持っていても、元token維持/新規発行のserver規約は確定しない。190の反応predicateはcategory判定前にreflectを返すが、応答待ち/自己即時・死亡/復活/補充の全scheduleは実験規約として分ける。

旧Flashでは満杯時dealItem失敗を無視する売却経路があるが、現在のoverflowItem表示とは異なる。現在serverの満杯配送や返金規約の根拠にはしない。

v20は4売る/SELL（trade/sell/価格5/giftRate20）を加え、215モデル/重み457（trade2/weapons98/armor74/sundries16/miracles25）とする。rulesetは `official-execution-subset-v20`、Snapshot20（旧1〜19拒否）、公開schemaは `official-execution-view-v18`。Match ABI2のcommand184/config24/status248は変更せず、新kind sell=5を追加する。catalog/state ABI1とlegacy37を保持し、学習器を追加しない。

`Command(actor, kind="sell", items=(sell_token, commodity_token), target=target)` は主手番で、本人所有の物理4と未使用商品一個の順を要求する。二tokenは別物、生存targetを明示し、自己/味方も許可、allocationは無し。used奇跡は商品不可だが、武器・208/209・別コピーの4・未使用奇跡は売れる。play4や追加FREE列へ変換しない。MP費用0で、価格5は操作料ではない。

原発時に4と商品を物理消費し、操作4だけにgift1を予約、商品の補充は無し。奇跡の商品もused保持や取得効果を起こさない。他者宛はtrade/power0/無属性/amount=商品priceのnonattackとしてresponseを作り、空応答または190万能反射だけを許す。自己宛は即時解決。反射で商品とpriceを固定したまま現在source/targetを交換し、再消費しない。

最終bodyでは現在targetのCPから名目priceを引く（不足はMP→HPへ波及）→現在sourceのCPへ名目price全額を加算（上限99、実所持CP額で制限しない）→現在targetへ商品の新token/unused instanceを直接渡す→208復活/死亡→自然終局/cutoff優先→既予約gift/次ownerとする。自己aliasもこの逐次順で、CP不足のHP損失を後のCP加算が戻すことはない。直接配送は環境RNGを使わず、通常の後続gift抽選と区別する。受け取った208による救済もこの順のローカル規約である。

v20〜25では配送先が実物で満杯なら明示unsupported（-5）でatomicに拒否し、samplerも除外する。自己売却は二品消費後の空きを考慮する。現行clientにはserver指定overflowItemの処理があるが、実物のeviction選択は未確定である。旧Flashの満杯配送失敗を無視して料金だけ処理する挙動を流用せず、満杯配送まで含む完全SELL対応とは呼ばない。

`last_sale` はnullまたは解決時の `{origin,source,target,model,price}`。modelは商品IDで、originは原発者、source/targetは現在の解決座標。解決時のみ作り、次main受理/reset/loadでclear、不正/失敗時は保持、解決済みcutoffでは残し未解決cutoffではnullにする。last_danger/last_removal/last_purchaseと排他、v20〜24ではpending/response phaseにも完了eventを置かなかった。現在もpurchase quoteとの同居は禁止する。token・配送先の他手札・RNGを出さず、完了raw snapshotの構造検査を全過去履歴の到達証明とみなさない。

v20〜25時点の公開 `sell_rule` は `experimental:one-unused-commodity;explicit-alive-target;zero-mp-fee;one-operation-gift;foreign-response-and-reflection;buyer-debit-before-current-source-credit;fresh-token-delivery-before-death;full-recipient-unsupported`。商品の232/Aura231/MP武器58/杵94/復活208等のabilityを売却payloadの構成時に実行・継承せず、単一対象・power0・属性無し・商品priceだけを用いる。配送後208の独立した救済hookは別に扱う。商品fakeも既存unsupportedのまま。構造体は同じABI2でも、clientは新opcode5の認識とruleset20のloader照合が必要である。

直接配送後の予約giftは `min(old_gift, hand_limit−new_items_count)` へ防御的に正規化する。実物を追い出す処理ではない。現scopeの正常経路では原発二品消費/gift1、190消費/gift1が作る空きにより通常は超過しない。実物満杯のunsupportedをこのclampで回避しない。

ベンチの `sell_plays` は操作回数、`sell_resolved` は自己即時/他者空応答で最終bodyが完了した数（190反射途中は除く）、`sell_nominal_cp` は名目price総額で実CP支払/HP損失/加算clip量ではない。`sold_miracles` は配送商品が奇跡だった件数でcast/学習ではない。`sold_miracles<=sell_resolved<=sell_plays` を確認する。v20の `physical_used` はSELL一回につき操作4だけ+1、商品は移送として除外し、通常play/defend/exchangeの選択使用や奇跡first/recastへ混ぜない。



v20は全source/test最終freeze後のfull Release CTest129件/21.25秒・ASan/UBSan89件/51.43秒が通過した（各 `-j4` 同時、rolloutとの競合あり）。最終Release/SAN full buildはwarning/error0。focused SAN2件/1.60秒（native1.20/C API0.40秒）が通過し、Releaseはfixture修正後Python単独1件/0.42秒と最終full suiteの通過を記録する。Release focused合計時間は掲載しない。native零引数59関数のmain各1回登録/unused guard、Python AST41ケースの重複無し、Release/SAN双方の固定query384完走を確認した。

| v20人数 | 試合数 | 意思決定数 | 勝数 | draw / 打切り | checksum |
| --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,988,600 | 49,818 / 50,061 | 121 / 0 | `16830441695517365036` |
| 9 | 10,000 | 2,453,031 | 1,046 / 1,087 / 1,127 / 1,144 / 1,170 / 1,116 / 1,138 / 1,044 / 1,113 | 14 / 1 | `5359086279512792637` |

| 人数 | 売却操作 / 最終解決 | 名目CP総額 | 奇跡商品配送 | 除去event / 空結果 | 除去event checksum |
| --- | --- | --- | --- | --- | --- |
| 2 | 66,834 / 66,834 | 381,497 | 3,795 | 9,825 / 4,163 | `10697524422810475650` |
| 9 | 30,416 / 30,416 | 170,764 | 2,177 | 7,377 / 2,655 | `170769133704707136` |

seed42の2人100,000試合/9人10,000試合を各1 worker/4 workersで実行し、worker数・時間以外の全fieldが一致、全終了snapshot（自然終局/打切り）を復元できた。Release/SAN seed42各1,000試合も非時間fieldが一致し、checksumは2人 `16214924716417635012`（SELL642回）/9人 `1248515908024842571`（SELL2,934回）。新SELLと既存の会計boundsを確認した。プール/方策/効果を増やした結果で、v19との遷移一致は主張しない。今回はquiet速度を再計測せず、v20の速度値や版間改善倍率を掲載しない。

詳細は[限定実行器v20記録](official-execution-subset.md#v20の過去の検証と売却集計)。

## 購入：確認表示とv21の限定契約

2026-11-02配信JS（hash5ece06…）と固定clientを区別して確認した。5はtrade/buy/価格5/giftRate20、通常submitは物理BUY一tokenとtargetPlayerIdを送り、購入決定 `d1`（9601–9610行）は `{bought:bool}` だけを送る。usedMiraclesNoteは使用済み奇跡をBUY対象外とする。mainの単品tradeとMP0、操作消費時の補充placeholder一個にもclient根拠がある。

見積のdecoder `p6`（3666行付近）はbuyingItemModelIdを読み、reconnect表示と `fF`（12461–12470行）の現在sourceのYES/NOへつながる。両ボタンは作られ、YES controlsにCP不足を無効化する比較は見当たらない。buy event（11163–11177行）は受信itemを表示して本人対象から消すが、候補抽選をしない。canNotBuy（11179–11198行）はbuyerへpoor/selfならsafeを表示し、受信itemをsellerへ戻す。空結果のevent形や厳密な同一item検査はこの表示handlerから確定しない。

setBought（11199–11249行）はYESなら現在buyerのCP減算→現在sellerのCP加算→buyer側overflowItem処理→buyerへ商品、NOなら金銭無しでsellerへ商品を戻す。client resource primitiveはCP不足時のMP/HP波及も持つため、現行serverがCPだけの購入可否を要求する証明にはならない。ローカルのCP不足時自動終了は実験規約として明示する。商品の表示はeventを受信したviewerの根拠であり、全参加者/観戦者へのserver fanoutを証明しない。

旧Flashの現金条件・候補抽選・自己BUYの別経路は歴史的補助資料に限定する。現在の一時商品表示、支払順、元slot/tokenへ戻すローカルescrowを旧serverの完全再現や現行server事実とは扱わない。

v21は5買う/BUY（trade/buy/価格5/giftRate20）を加え、216モデル/重み477（trade3/weapons98/armor74/sundries16/miracles25）とする。rulesetは `official-execution-subset-v21`、Snapshot21（旧1〜20拒否）、公開schemaは `official-execution-view-v19`。Match ABI2/command184/config24/status248は据置で、kind buy=6/accept=7/decline=8とphase purchase=3を追加する。finished=2は維持する。clientは新opcode/phaseとruleset21のloaderを認識する必要がある。catalog/state ABI1・legacy37を保持し、学習器は追加しない。

主手番 `Command(actor, kind="buy", items=(buy_token,), target=seller)` は本人の物理5一個と本人以外の明示alive targetを要求する。自己BUYはserver条件未確定のため明示unsupported（-5）とし、play5・追加MC/FREE・allocation混入を許さない。MP費用0、価格5は操作料ではなく、BUY操作にgift1を予約する。他者へのnonattackは空応答または190万能反射だけを許し、反射後の現在sourceがbuyer/現在targetがsellerになる。原発操作を再消費しない。

最終bodyで現在sellerの未使用手札一個を一様bounded抽選し（used奇跡を除く）、原slot/tokenを特権escrowとして一時的に取り除く。候補0ならdraw無し・見積無しで終了、候補1でもdraw1回。現在buyerのCP<商品priceならその場で元slot/tokenへ戻し、金銭移動無しで終了する。このCP条件に借金支払を使わない。候補生成/抽選・空結果・CP不足終了・slot同一性はserver未確認の実験規約である。

CP>=priceならpurchase phaseへ入りactorは現在buyer、公開 `purchase={origin,buyer,seller,model,price}` だけを示す。現在effect queueは空になるがbatchは残し、主動作/response/抽選/escrowを全journal replayで照合する。元slot/token・候補母数・乱数・他の未使用手札は公開しない。見積中はlast_purchaseを作らない。

buyerの `Command(actor, kind="accept")` / `Command(actor, kind="decline")` はitems空・target無し・allocation無し。YESはbuyerのCPからpriceを減算→sellerへ名目全額CP加算（上限99）→buyerへ新tokenのunused商品を直接配送→既存の復活/死亡・自然終局/cutoff・gift/次手番へ進む。NOはsellerの元slot/tokenへ戻して金銭移動無し。商品分giftは無く、未使用奇跡もcast/保持化せず、配送/返却は環境RNGを使わない。後の通常gift抽選は別に残る。

v21〜25では実物満杯buyerへのYESはunsupported（-5）でatomic拒否し、NOは可能。通常の閉じた経路では操作5または190の消費で空きがあるが、実物eviction規約は導入しない。未完了escrowを残したcutoffでは元slot/tokenへ返却し、予約giftを取消す。既に配送まで解決した結果のgift取消とは区別する。自己BUY/満杯配送を含む完全BUY対応とは呼ばない。

`last_purchase` はnullまたは `{origin,buyer,seller,model,price,bought,reason}`。reasonは `bought` / `declined` / `empty` / `insufficient-cp` / `cutoff`。emptyはmodel0/price0、launch後でも商品未選定のcutoffはnullとする。次main受理/reset/loadでclear、失敗時は保持。見積と完了eventは同居させず、last_danger/last_removal/last_saleとも排他。完了raw snapshotの構造検査は全過去履歴の到達証明ではない。

v21〜25時点の公開 `buy_rule` は `experimental:foreign-only;one-operation-gift;uniform-unused-commodity;empty-no-draw;cp-only-affordability;buyer-accept-or-decline;escrow-return-on-decline-or-cutoff;fresh-token-delivery;full-accept-unsupported`。`bought` はreasonがboughtかどうかから派生するbooleanで、独立した履歴flagではない。特権 `PurchaseOffer` はoriginal item（token/model/used=false）と原slotを保持するが、公開purchaseにはこれらを追加しない。承認/拒否も新しい意思決定として既存cutoff規約に従う。

購入決定をjournalへ一件加えるため、BUY actionだけ既存の保守的response/journal boundへ+1する。非BUYの最大3,725を変えず、全体の保守的な上界へさらに+1を見込めば3,726となる。実際のBUYは単一target・190の物理反射だけなので、これより小さい。購入承認でも商品配送後のgift予約を `min(gift, hand_limit−new_items_count)` に正規化し、実物evictionを行わない。

ベンチは `main_commands+responses+purchase_commands=decisions` へ三分離する。`buy_plays` は操作数、`buy_quotes` は公開見積phase開始数、`purchase_commands` はaccept/decline数、`bought`/`declined` は各決定結果、`buy_empty`/`buy_insufficient_cp`/`buy_cutoff` は各完了reason、`buy_nominal_cp` はboughtの名目price総和（sellerのclip後実増分ではない）。`bought+declined=purchase_commands<=buy_quotes<=buy_plays`。`physical_used` はBUY操作5だけ+1で、商品移動・購入決定・奇跡cast/学習を加えない。v20以前のmain+responses=decisionsは過去版の定義として残す。

v21は全source/test最終freeze後のfull Release CTest129件/20.63秒・ASan/UBSan89件/51.45秒が通過した。最終Release/SAN buildはwarning/error0。focused Release3件/1.46秒（native0.68/C API0.23/Python0.55秒）・SAN2件/1.44秒（native1.09/C API0.34秒）が通過した。native零引数63関数のmain各1回登録/unused guard、Python AST45ケースの重複無し、Release/SAN双方の固定query384完走を確認した。

| v21人数 | 試合数 | 意思決定数 | 勝数 | draw / 打切り | checksum |
| --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 4,189,270 | 49,527 / 50,355 | 118 / 0 | `16901879550753049792` |
| 9 | 10,000 | 2,648,336 | 1,069 / 1,090 / 1,121 / 1,178 / 1,113 / 1,118 / 1,134 / 1,057 / 1,094 | 17 / 9 | `5307337226493388206` |

| 人数 | BUY操作 | 見積 / 購入決定 | bought / declined | empty / CP不足 / 未決cutoff | 成約名目CP |
| --- | --- | --- | --- | --- | --- |
| 2 | 66,229 | 61,060 / 61,060 | 30,447 / 30,613 | 593 / 4,576 / 0 | 160,439 |
| 9 | 66,971 | 62,291 / 62,291 | 31,091 / 31,200 | 250 / 4,430 / 0 | 163,807 |

seed42の2人100,000試合/9人10,000試合で各1 worker/4 workersのworker数・時間以外の全fieldが一致し、全終了snapshot（自然終局/打切り）を復元できた。新旧会計boundsと三phaseの意思決定数を確認した。Release/SAN seed42各1,000試合も非時間fieldが一致し、2人checksum `9959108341010664234`（BUY655/bought284/declined324）、9人checksum `14166364559885031647`（BUY6,766/bought3,156/declined3,185）。今回の大規模方策で未決cutoff件数0だったことを、cutoff返却が到達不能という根拠にしない。quiet速度を再計測せず、v21の速度値や版間の改善倍率は掲載しない。

詳細は[限定実行器v21記録](official-execution-subset.md#v21の過去の検証と購入集計)。

## 霧：何が見えなくなるか

公式教典は、霧の使用者が敵へ神器を使うとき、敵の中から使用先が無作為に選ばれると説明する。敵という集合はチームに依存し、味方・自己まで同じ規則で無作為化するとは書かれていない。

現在のクライアントでは、以下を直接確認できる。

1. `tL`（4021–4067行）はプレイヤーのHP/MP/CP、災い、守護神、dead、items等を読み込む。霧表示があることと、通信データが全て伏せられていることは同義でない。
2. 状態再構成 `ft.fs`（12352–12357行）は、生存している自分が霧を持ち、終局していない場合、敵の表示を `em(true)` で覆う。同チームの表示はこのループでは覆わない。個人戦では自分以外が対象になる。
3. `em`（13257–13273行）はプレイヤー情報コンテナ `z` を取り外し、青い半透明の覆いへ置き換える。`iT`（13173–13211行）、`c5`（4154–4176行）、`ox`（4078–4098行）をたどると、このコンテナには名前・チーム記号・HP/MP/CPの領域が含まれる。災いと守護神も同コンテナへ追加される（`dA` 13245行、`sd1` 13295行以降）。座席の存在そのものを取り除く処理ではない。通常pane内の名前/teamは覆われるが、覆いの外にある使用済み奇跡paneの名前/team/usedモデルは閲覧可能である。view全体から名前/teamを消す根拠にはしない（[状態と表示の契約](official-state-view.md#通常パネルと使用済み奇跡パネルは別)参照）。
4. 自分に霧が付く/解除されるイベントは `bC(true/false)`（11743–11759行、11359–11377行、11444–11451行）を通じて敵の覆いを切り替える。
5. 敵候補の `dR`（13149–13151行）は生存者から敵関係 `ce` で絞る。UIは敵が複数の場合に霧の音・演出を分ける（9562行、10910行）。この音の条件はサーバーの乱数生成そのものではない。

公式profileの観測では、内部に真値を保持していても、霧の本人へ敵HP/MP/CP・災い・守護神等をそのまま渡してはならない。どの過去イベントの記憶を保持できるかは別の知識状態として扱う必要がある。合法手マスクにも、覆われた敵情報を利用した識別結果を出さないこと。

未確定なのは、再選択の厳密な実行段階、選択前後の公開イベント、敵1人時の乱数消費、特殊・全体・守護神由来の例外、病気や守護神の演出による部分的情報開示である。既存旧版の `not forced` 条件を現行へそのまま採用する根拠はまだない。

## Fog4モデルとv24の実験規約

JAは霧の本人が敵へ使った神器の対象を敵内でrandomと説明する。現行 `aT`（9581行付近）は選択targetと霧の音を扱うが、その場でcommand targetを抽選しない。受信setTargetPlayer（10918–10951行）はevent.playerIdで表示を決め、armor/Danger/Wide等を除く音の条件はserverの対象抽選規則そのものではない。

fresh再構成（12379–12384行）は生存Fog viewer/未終局/生存敵だけを覆う。使用済み奇跡paneは覆いの外にあり、名前/team/usedモデルは別に閲覧できる。hit（10955–10972行）はFogで敵paneが隠れると暗雲forcedの演出を抑えるため、Matchのforced_hitも元の命中対象に従ってnullにし、反射後にその暗雲を漏らさない。event経路の表示は全員へのserver配信を証明しない。

v24は72（Water weapon/ATK3/onDamage Fog/p10w1）、97（Water weapon/ATK3/50%/onDamage Fog/p8w1）、182（Water armor/counter Fog/power0/DEF0/p10w1）、221（Water miracle/Fog/MP3/w1）を加え、228モデル/重み489（trade3/weapons104/armor76/sundries17/miracles28）とする。rulesetは `official-execution-subset-v24`、Snapshot24（旧1〜23拒否）、公開schemaは `official-execution-view-v22`。Match ABI2の幅・kinds0〜8・purchase phase3/finished2、catalog/state ABI1・legacy37を維持し、学習器は追加しない。

raw stateは病気最大一種→Fog→Flash→DarkCloudのenum順/重複無し、最大四curseへ広げる。mildはCold/Fever/Fog/Flash、allは対応する病気4種/Fog/Flash/DarkCloudの全七種類を解除する。Fogは非病気でHeaven再付与fallを起こさず、Fog単独には自然tickを追加しない。72/97の付与は既存の正target HP実損失、182はsurvivor/FIFOの純curse反撃、221は保持/FREEと適合する魔法反応に従う。

Fogの本人がmain playまたはbuyで明示的に敵を指定した場合、launchでalive敵をinput順に列挙して一様bounded drawを一度引き、候補一人でもdrawする。202を使う場合はこの対象draw→符号drawの順。自己/味方指定は変えず（自己BUYの既存unsupportedは維持）、%攻撃・Wide232・Danger94はFog drawを除外する。14は選択先に二回、redirect/bounce/counter/購入YESNOで選び直さない。祈り/discard/exchangeもこの敵指定launchに含めない。

教典の「敵に使った神器は敵内でrandom」に対し、一様性/一人時draw/launch時点/例外/順序は**公式server未確認の実験規約**である。特権 `last_fog={owner,intended,target,models}` は次main受理でclearし、公開JSONへ出さない。pendingの全journal replayは元command/状態/RNGから対象選択とpayloadを検査するが、完了raw sceneの全歴史の到達証明ではない。

観測は既存project_viewで、生存Fog viewerかつ未終局のときだけ生存敵statusをnullにする。自己/味方/dead敵、観戦/dead viewer/終局後には覆いを掛けない。通常status欄と別の使用済み奇跡paneに表示される名前・team・usedモデルは公開のまま。未使用の他者手札枚数/中身は出さない。

`pending.forced_hit` は現在targetではなく、命中判定時の `initial_target` のboard paneがFogで隠れるviewerに対し、内部true/falseの両方をnullにする。元のhidden敵のDarkCloud判定が190反射で可視selfへ戻ってもnullを維持し、可視の元targetならboolを返す。内部の必中判定/乱数消費は変えない。pending payloadやpurchase/removal/sale等の公開eventは別表示経路として保持するが、全viewerへのserver fanoutは未照合である。

v24/25ではFog本人のforeign SELLは敵/味方を問わず、配送容量を読む前にunsupported（-5）として拒否し、自己SELLは許す。samplerはFogの有無にかかわらずSELLを自己targetに固定し、hiddenな他者未使用手札数を配送先候補から読まない。非Fogで手動foreign SELLをしたときの旧overflow-capacity依存gapは残す。全query/全profileの情報非干渉を達成したとは主張せず、この限定拒否と方策変更を明示する。

ベンチの `fog_main_commands` は受理main前のownerがFogだった全kind、`fog_launches` はそのmainでlast_fogを生成した数、`fog_target_changed` はintended!=selectedの数。response中のrecord残存を重複計上せず、changed<=launches<=fog_main_commands<=main_commandsを検査する。2人戦の候補一人で対象不変でも、launch drawは一回消費する。view-v22のforced_hitはbool/nullの三状態として読み、nullをfalseと同一視しない。

v24/25時点の公開 `fog_rule` は `experimental:launch-once-for-explicit-enemy-target;uniform-alive-enemies-in-input-order;one-draw-including-singleton;self-and-allies-unchanged;chance-wide-danger-excluded;redirects-no-reselection;fog-foreign-sell-unsupported;sampler-sell-self-only`。canonical集合を広げた現行 `disease_rule` は `experimental:one-disease-plus-fog-flash-and-darkcloud;reapply-max-incoming-or-next;heaven-reapplication-hp0;owner-only-after-whole-action;natural-and-cutoff-before-tick;fixed5-percent-before-hp;post-upgrade-tick;post-tick-revival-before-gifts`。病気のrank/5%/owner境界やFlash/DarkCloudの既存規約を変えず、Fogの導入を旧版のnot-forced条件へ置き換えない。

v24の最終検証でRelease CTest129/129件（55.56秒）・ASan/UBSan89/89件（85.03秒）が通過した。時間は並行suite実行の経過であり、エンジンの性能値や版間速度比較には使わない。native零引数77関数のmain登録、Python AST55ケースの重複無し、新bridge Fog4ケース、C API固定6×64 query384完走を確認した。動的Wall+FREE fixtureは最初のseed8990へ届くよう探索上限を8192から16384へ広げて実行した。source/native/benchの独立read-only監査で重大な指摘は無かった。

| v24人数 | 試合数 | 意思決定数 | 勝数 | draw / 打切り | checksum |
| --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,947,197 | 49,585 / 50,310 | 105 / 0 | `15312008538106736201` |
| 9 | 10,000 | 2,506,619 | 1,102 / 1,134 / 1,104 / 1,057 / 1,129 / 1,099 / 1,106 / 1,102 / 1,154 | 11 / 2 | `1417387029820349467` |

| 人数 | Fog本人main | Fog launch draw | intendedから対象変更 |
| --- | --- | --- | --- |
| 2 | 112,904 | 30,180 | 0 |
| 9 | 102,818 | 49,146 | 34,241 |

seed42の2人100,000試合/9人10,000試合を各1 worker/4 workersで実行し、worker数・時間以外の全fieldが一致した。全終了snapshot（自然終局/打切り）を復元し、新旧会計boundsを確認した。2人戦の変更0でもlaunchの一候補drawは消費する。Release/SAN seed42各1,000試合の非時間fieldも一致し、checksumは2人 `788222134307769052` / 9人 `57481880523464303`。Fogのroll/対象分布やsampler変更を含むローカル集計で、公式serverの測定や旧版との全profile非干渉の証明ではない。今回quiet速度は再計測せず、v24の速度値・改善倍率は掲載しない。

詳細は[限定実行器v24記録](official-execution-subset.md#v24の過去の検証とfog集計)。

## 昇天弓112：現行表示・旧Flashとの差・v25規約

現行JAは昇天弓112の死亡時75%攻30を説明し、台帳の通常部分はLight ATK1/25%。受信 `attackDyingly`（現行11705–11749行付近）はserverから渡されたsource・atk・hitRate・itemを表示し、本人ならinstance IDで選択して `cZ→ed(null)` の消費/placeholder表示へつなぐ。active attack表示も動的atk/hitRateを使う。これだけでは発火gate・何copyか・対象集合・順・抽選・死亡/終局・実gift発行は確定しない。

旧Flash117は通常Light ATK1/75%で、死亡時の値30と通常率を区別する。旧room.handlePlayersDeathは244救済→dead化→not.dead人数2以上なら最初の117だけをforced queueへ置き、旧forced pathは117を通常消費しない。既dead sourceのalive敵全員へrosterを作り、beforeEndInningはqueueをdrainする。旧duel不発はこの旧gateの結果であり、現行serverの証明ではない。現在clientには消費表示があるため、旧非消費を採用しない。

v25は112昇天弓（Light weapon/通常ATK1・25%/死亡時ATK30・75%/価格10/giftRate1）を加え、229モデル/重み490（trade3/weapons105/armor76/sundries17/miracles28）とする。rulesetは `official-execution-subset-v25`、Snapshot25（旧1〜24拒否）、公開schemaは `official-execution-view-v23`。Match ABI2の幅・kinds0〜8・purchase phase3/finished2、catalog/state ABI1・legacy37を維持し、学習器は追加しない。

主動作の112は通常の25%攻撃としてtarget無しのalive敵rosterを使い、消費/支払/命中/RNG/応答は既存規約に従う。死亡時は別の自動payloadで、Light30/75%・モデル112・death来歴を持つ。通常の1/25%から動的値を識別し、旧117の通常75%を現行112へ流用しない。

208救済を先に処理し、実際にdeadになった人物だけを対象とする。全bodyのactual death flags確定後に自然終局なら112を消費/予約しない。非終局なら新死者の在庫112を全copy在庫順に直接消し、copyごとにその境界のalive敵input順へframeを一括予約する。死亡者へgift予約は増やさず、通常consume helperとは分ける。同じcopyが複数enemy payloadの来歴になっても、物理消費は一回だけである。

decision cutoffはbody/死後の112消費・予約より後に判定する。従ってこの場合のcopyは消費済みで、残workを取消し、まだ開始していない命中roll/responseは行わない。自然終局優先では生存者一人・全員同じ非Solo team等の既存条件も使い、duelの最後の敵死亡から道連れを発生させない。このgate・全copy・消費順・cutoffの差は**公式server未確認の実験規約**である。

新しいdeath FIFOを普通counter FIFO・初期rosterより先にdrainし、途中で新たに死亡した人物のframeは末尾へ追加する。死亡者はsource/originのまま自動攻撃できるが、dead actorの手動commandを許す意味ではない。ownerは元mainのまま。対象deadはskip、alive対象への活性化時に75%を抽選し、DarkCloudならdraw無し必中。Fogの追加再選択はせず、反射/跳ね返しで再命中rollしない。返された先がdeadならframeを終了、bounceは既存alive候補へ向ける。普通防御・属性filter・適合反応は現在payloadに従う。

全main kindのprestate/原command/受理response journalから同executorを再生し、死者・消費copy・death FIFO・RNG・普通counter・現在frameを照合する。病気tickによる死亡も同じ仕組みへ接続し、特権 `tick_processed` stageで元ownerのtickを一回に限る。tick後に死亡時応答を待っても、完了時に同ownerへ再tickしない。giftと次ownerは全work後まで遅延し、自然終局/cutoffで残work/giftを取消す。安定raw sceneの全過去履歴まで証明する契約ではない。

death frame数の保守的上界は `player_capacity*(player_capacity−1)*hand_limit`（最大9×8×18=1,296）。copy数とframe数を混同せず、journal上界へこのdeath_boundを加える。旧v23/24の最大3,725/BUY3,726だけを死亡queueへ流用しない。死亡FIFO・自動消費token・未来RNG・stage・内部累計は公開せず、現在の `pending.death` booleanとdeath_ruleを公開する。死亡時payloadでもforced_hitのinitial_targetに対するFog null化は維持する。

ベンチの `death_bows` は自動消費した112コピー数で、通常の `physical_used` へ混ぜない。`death_created` は全予約frame数、`death_started` は75%命中または暗雲forcedでalive actorのresponseを開始したframe数、`death_rolls/hits` は実抽選/成功、`death_forced_hits` は無draw命中、`death_responses` は反射等を含む実defend回数。bows<=created<=8*bows、hits<=rolls<=created、started=hits+forced、forced<=created−rolls、responses>=startedを検査する。新しいtickだけを集計し、死後応答中に残るlast_tickを重複計上しない。自己205/206の後に同tickで消えた208/自動112を除去eventのtoken消失数へ混ぜない。公開JSONへ累計/未来FIFO/来歴stageを追加しない。通常配布で未対応のgiftモデルは残り8種で、Dreamは調査のみとする。

公開 `death_rule` は `experimental:revival-before-actual-death;nonterminal-boundaries-only;consume-all-bows-in-inventory-order;copy-then-alive-enemies-in-input-order;light30-75-percent;death-fifo-before-counters-and-initial-roster;dead-source-allowed;darkcloud-forces-without-draw;redirects-no-reroll;dead-redirect-target-ends-frame;no-fog-reselection-or-gifts;owner-tick-once;natural-and-cutoff-cancel-pending`。教典の死亡時75%攻30とclientの受信演出を、ここで選んだqueue/終局/再帰/救済/giftのserver規約と取り違えない。

v25の最終検証でRelease CTest129/129件（56.07秒）・ASan/UBSan89/89件（85.40秒）が通過した。並行suite実行の経過時間であり、エンジン性能や版間比較の値には使わない。両buildはwarning0。native零引数83関数はRelease/SANで全てmainへ各1回登録、Python AST59ケースは重複無し、新Death bridge4ケース、C API固定query384完走を確認した。source/benchの独立read-only監査で重大な指摘は無かった。

| v25人数 | 試合数 | 意思決定数 | 勝数 | draw / 打切り | checksum |
| --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,960,357 | 50,255 / 49,644 | 101 / 0 | `14674230429006413330` |
| 9 | 10,000 | 2,394,313 | 1,126 / 1,069 / 1,115 / 1,069 / 1,110 / 1,124 / 1,132 / 1,133 / 1,102 | 17 / 3 | `16501005794355218399` |

| 人数 | 自動112コピー | 予約frame / 開始 | 実roll / hit / miss | 無drawforced | 実defend応答 |
| --- | --- | --- | --- | --- | --- |
| 2 | 0 | 0 / 0 | 0 / 0 / 0 | 0 | 0 |
| 9 | 2,467 | 12,204 / 8,868 | 10,960 / 8,240 / 2,720 | 628 | 8,871 |

seed42の2人100,000試合/9人10,000試合で各1 worker/4 workersのworker数・時間以外の全fieldが一致し、全終了snapshot（自然終局/打切り）を復元できた。2人で死亡時fieldが全て0なのは自然終局優先gateに沿う結果である。Release/SAN seed42各1,000試合も非時間fieldが一致し、checksumは2人 `2410992088900070692` / 9人 `10897494970209546519`。deathと既存会計のboundsを確認した。このqueueや終局gateを公式serverで測定した結果とは扱わず、今回はquiet速度を再計測せず、v25の速度値・改善倍率を掲載しない。

詳細は[限定実行器v25記録](official-execution-subset.md#v25の過去の検証と死亡時攻撃集計)。

## 取引overflow：表示根拠とv26規約

現行clientはSELL/setBoughtでserver指定overflowItemを受け、`eH→bE` の表示を通して本人の該当tokenを取り除き、使用済み奇跡paneも更新する。表示経路にcategory/usedで除外する条件は見当たらないが、これはserverのeviction候補や一様抽選を計算するコードではない。全existing実物/一copy抽選はローカル規約として別に示す。旧Flashの満杯dealItem失敗無視・料金だけ処理を現在へ移さない。

v26はモデルを増やさず229モデル/重み490（trade3/weapons105/armor76/sundries17/miracles28）を維持し、SELLの満杯配送・BUYにも共用する配送helper、Fogのforeign SELL、先行capacity queryのgapを扱う。rulesetは `official-execution-subset-v26`、Snapshot26（旧1〜25拒否）、公開schemaは `official-execution-view-v24`。Match ABI2の幅・kinds0〜8・purchase phase3/finished2、catalog/state ABI1・legacy37を維持し、学習器を追加しない。Dreamは調査のみで未対応のままである。

取引配送先の実物itemsが上限なら、配送前からある全実物をinventory順に列挙し、uniform bounded draw一回で一copyをevictしてから新token/unused商品を渡す。使用済み奇跡、208/209/112等のpassiveも候補で、同モデル別copyを別候補として扱う。一候補でもdraw一回。新商品自身や補充placeholderから選ばず、空きがあればoverflow draw無しで直接配送する。使用済み奇跡の商品選択除外と、overflow候補としての可否は別契約である。

v26当時の229モデルprofileではBUYのbuyerは操作5または反射190を消費するため常にcap−1以下で、quote中の流入/giftも無い。したがって満杯BUYはこの閉じたprofileでは到達不能で、共通配送helperの備えに留まる。検証はfullSELLで共用helperの全候補順を確認し、通常BUYでoverflow_model=nullを確認するもので、偽raw quote importでBUY fullを到達させたcoverageを主張しない。

CP debit/current sourceへのfull nominal credit→既存itemのeviction→fresh delivery→既存208救済/死亡/112予約の順。eviction自身に補充・MP返金・curse解除・passive等のabilityを起こさない。残った/届いた208や112は後の実死亡境界で通常のhookに従う。使用済み奇跡を失った場合はused paneから消えるが、付与済み状態を取消さない。配送後の予約gift min-clampは維持する。BUYのdecline/未決cutoffは原slot/tokenへの返却なので、このoverflow抽選を行わない。

`last_sale` / `last_purchase` に任意の `overflow_model` を加え、eviction無しならnull、有りなら失ったモデルIDだけを公開する。token/元slot/候補母数/used flag/RNGは追加しない。受信clientのoverflowItem→reveal/本人手札消去/used pane更新に表示根拠があるが、全viewerへのserver fanoutは未確認。uniform分布・全実物候補・一候補draw・補充/死亡順は**公式server未確認の実験規約**であり、これで公式サーバーの完全な取引仕様を取得したとは呼ばない。

Fogのforeign SELLをunsupportedにせず、play/buyと同じく明示enemy hintならlaunchでalive敵input順から一様draw一回（一人でも一回）を行う。自己/味方は維持する。商品が232/%武器/Danger等であっても、商品を対象modifierへ混ぜず、操作SELLだけでtarget/power0/element無し/priceを決める。反射後の再選択は無い。samplerのSELLは全alive input順のtargetを候補にし、他者容量をpeekして除外しない。legal/samplerが満杯か否かで事前拒否する依存を解消し、overflow結果は行動解決後の公開eventとする。

v19〜24では単一body完了がaction完了で、完了eventはresponseと同居しなかった。v25以降のdeath FIFOでは元の除去/売却/購入bodyが完了しても後続death応答が残り得るため、last_removal/last_sale/last_purchaseはこの応答中も保持する。購入quoteとの同居は禁止し、次main受理でclear・失敗保持・相互排他・解決済みcutoff保持は維持する。pending journalの全replayで取引/抽選/overflow/死後workを照合するが、完了raw stateの全過去到達履歴は証明しない。

ベンチ `sell_overflows` / `buy_overflows` は各解決stepで一回だけ数え、`overflow_events=sell_overflows+buy_overflows`、sell_overflows<=sell_resolved、buy_overflows<=boughtを検査する。`overflow_event_checksum` はSELL/BUYのkind5/6を分けた公開event tupleから作りsum集計し、slot/token/元候補数は含めない。これは通常observeへ内部累計を追加するものではない。

自己BUY、通常giftが満杯時のclamp/抽選scheduleの公式照合、94+Wide等の別gapは残す。ここでのfull配送対応はSELL/BUY直接配送の範囲であり、一般の全配布/全モデル/全効果への対応を主張しない。

公開 `overflow_rule` は `experimental:trade-delivery-only;full-recipient-uniform-existing-item-in-inventory-order;one-bounded-draw;all-existing-items-including-used-miracles-and-passives;cp-debit-and-credit-before-eviction;eviction-before-fresh-delivery-and-death;no-eviction-gift-or-ability;public-model-only;no-overflow-on-decline-or-cutoff-return`。

公開 `sell_rule` は `experimental:one-unused-commodity;explicit-alive-target;zero-mp-fee;one-operation-gift;foreign-response-and-reflection;buyer-debit-before-current-source-credit;fresh-token-delivery-before-death;full-recipient-uniform-existing-item-overflow`。

公開 `buy_rule` は `experimental:foreign-only;one-operation-gift;uniform-unused-commodity;empty-no-draw;cp-only-affordability;buyer-accept-or-decline;escrow-return-on-decline-or-cutoff;fresh-token-delivery;full-recipient-uniform-existing-item-overflow`。

公開 `fog_rule` は `experimental:launch-once-for-explicit-enemy-play-buy-sell;uniform-alive-enemies-in-input-order;one-draw-including-singleton;self-and-allies-unchanged;chance-wide-danger-excluded-except-sale-commodity;redirects-no-reselection;sampler-sell-all-alive-without-capacity-peek`。

native最終88関数のmain登録、Python63 methodの重複無し、新bridge4ケース、C API固定query384完走を確認した。final focused bridgeは2/2件・合計0.94秒（C API0.30/Python0.63秒）が通過した。fullSELLで共通helperの候補順・usedMC/passive・gift0/支払順・Fog/反射・容量query境界を検査し、通常BUYではoverflow_model=nullを確認する。初期配送先が満杯でも190反射後の最終recipientだけを評価し、BUY decline/cutoffはfull sellerの原slot/tokenへ追加draw無しで返す境界もReleaseで確認した。最後の追加assertを含むSANの確認はfull suite欄へ記録する。満杯BUYへの偽raw quote到達coverageは主張しない。

v26の最終検証でRelease CTest129/129件（56.44秒）・ASan/UBSan89/89件（86.64秒）が通過した。両buildはwarning/error0。時間は並行suite実行の経過であり、エンジン性能や版間速度比較には使わない。最後に追加した最終受取先での容量判定・BUY返却時の追加draw無しも、最終Release/SAN双方のsuiteで確認した。

| v26人数 | 試合数 | 意思決定数 | 勝数 | draw / 打切り | checksum |
| --- | --- | --- | --- | --- | --- |
| 2 | 100,000 | 3,988,255 | 49,954 / 49,935 | 111 / 0 | `11173370200104368305` |
| 9 | 10,000 | 2,409,699 | 1,121 / 1,104 / 1,100 / 1,102 / 1,099 / 1,108 / 1,133 / 1,145 / 1,074 | 13 / 1 | `10091894475017822334` |

| 人数 | SELL開始 / 最終解決 | SELL overflow | BUY overflow | overflow events | overflow event checksum |
| --- | --- | --- | --- | --- | --- |
| 2 | 59,744 / 59,744 | 0 | 0 | 0 | `0` |
| 9 | 25,928 / 25,928 | 9 | 0 | 9 | `11148538478604155371` |

seed42の各人数を1 worker/4 workersで実行し、worker数・時間以外の全fieldが一致した。全終了snapshotを復元し、新旧会計boundsを確認した。Release/SAN seed42各1,000試合も非時間fieldが一致し、checksumは2人 `760147037809871428` / 9人 `4393572629372685365`。通常BUYのoverflow0は現profileの空きの不変条件に沿い、fullSELLは専用goldenで候補順を検証する。2人ベンチのSELL overflow0はこのseed/方策での結果で、満杯SELLの到達不能性を示さない。候補分布は公式server未確認の実験規約であり、quiet速度の再計測や版間速度改善の主張はしない。

詳細は[限定実行器v26記録](official-execution-subset.md#v26の検証と取引overflow集計)。

## 夢：真の手札と見かけの手札

現行名は `dream` /「夢」。旧 `ILLUSION` /「幻覚」との対応は効果と名前の対応表で管理し、旧IDの一致を仮定しない。

教典は授かる神器が50%で別神器に見えると説明する。JSでは以下が確定する。

- 手札データ `kY`（3855–3865行）は固有 `id`、`modelId`、`fakeModelId`、`used` を別々に読む。
- `aC.hn`（12853–12874行）は真のモデルを `e`、偽モデルがあればそれを `f` に保持する。通常表示で使う `gL`（12849–12852行）は偽モデルを優先する。
- `ev`、MP合計検査、補助表示は `gL()` を参照する（9478–9508行、9596–9601行、12921行以降）。したがって画面上の選択可否も見かけのカードに依存する。真の神器のカテゴリやMPで先回りして合法手を狭めると秘匿情報が漏れる。
- `submit-command` へは選んだ手札の固有 `itemIds` を渡す（9651–9672行、`jP` 9680行）。見かけモデルIDを効果そのものとして送る設計ではない。
- 使用時の `bZ`（11760–11774行）は真偽が異なると夢の演出を行い、実際のモデルを表示する。解除時も既存の偽表示を取り除く処理がある（11449–11459行、`e5` 12783行以降）。

サーバーが見かけ上合法・実体上不適合な入力をどう処理するか、偽モデルの抽選集合・重み、保持済み奇跡や売買による取得への適用、解除後の配布位置・保持化、同じ真偽モデルを選ぶことがあるかは未確定。真値を使って環境内部で不正手拒否するだけでは公式の夢を再現したことにならない。

## 守護神：旧HP20盾モデルを流用しない

公式 `guardianNames` は10種類を列挙する。教典は敵ターン後の行動確率25%、被害時に去る確率10%を説明する。旧版の30%行動・HP20で肩代わりする方式とは異なる。

現行クライアントの守護神は `fv`（3933–3935行）で10種類の列挙型へ変換され、プレイヤー状態 `tL` の `guardian` もその列挙型である。旧版の守護神HP20に対応するHP値は、このデコード構造に無い。`setGuardian` / `removeGuardian` のイベント表示（11462–11504行）は種類の付替・除去として処理する。これだけでサーバー内部の全防御計算を証明するものではないが、HP20盾を現行公式の確定仕様として入れる根拠はない。

JSON `items[244..285]`（現行ID245–286）は守護神行動42行を持つ。火星〜金星の8種類は各5行、地球・月は各1行。最初の8種類の `guardianAttackRate` は各6/5/4/3/2で、教典UIは `/20` で百分率へ変換する（14861行）。地球は通常の取引/武器/防具/雑貨、月は奇跡を使う旨の説明がある。これらは通常行動の25%発動判定とは別の値である。

現行には夢・霧を付与する守護神の行動もある。正確な助手対応には、それらの観測契約と、地球・月がどの台帳・重みで神器を選ぶかの確定が必要。召喚10種類の抽選重み、再召喚・被害・去る抽選の順序、反撃/反射/死者/天国病/蘇生との交差は未確定。旧助手調査は[旧版の別記録](legacy-assistant-rules.md)として残す。

## 複数人・チーム・終末

HTMLの公式説明は最大9人を明記する。日本語モード説明でduelは2人個人戦。JS `pQ`（4206–4209行）と列挙定義（16797–16802行）は個人戦0とチーム1〜4を持つ。`ce`（13231–13233行）は個人戦なら他人、チーム戦なら異なるチームを敵とする。`aG` はdeadを除いた集合、`dR` はその敵部分集合（13145–13151行）。9座席や4チームを単にduelへ縮約して公式完全対応とは呼べない。

ゲーム状態 `tk`（3752–3786行）は `gf`、任意の `tiebreakGF`、主動作プレイヤー、攻撃リスト、終局、イベント、更新番号を読む。表示側は攻撃リストの先頭を処理する。サーバーの全キュー順序や同時死亡決着を、この構造だけから確定することはできない。

終末について確定できる公開契約は次の通り。

- 教典は、終末以降に捨てる操作が別の「ささげる」操作へ変わると説明する。
- 教典は、終末以降の神器授与で25%の悪魔出現を説明する。
- 終末設定UIには無効、1、50、75、100、150 GFの選択肢がある（12570–12578行）。全モードでこのUIの全候補を設定できることまでは未検証。
- `advanceGF`（10552–10589行）は設定GFへ到達すると終末の表示へ切り替える。
- 現行ID287–296は超常現象10行で、全員への病気/霧/混乱、HP設定、攻撃、回復、資源集合、手札シャッフル、全員の守護神設定等のタグを持つ。発生率・発生条件・選出規則をこの台帳だけから決めてはならない。

終末・悪魔・超常現象の抽選を旧版に寄せて追加するだけでは完全対応にならない。通常の神器台帳、悪魔の出現率、守護神の行動率、現象の契機を別々に検証する必要がある。

## 初期値・手札と既存衝突の整理

クライアントのプレイヤー表示初期値はHP40/MP10/CP20（13204–13209行）。手札表示の初期化 `kH`（12676行）は空枠9個を作る。`bb`（12678行以降）や配置関数 `tv`（3868行）は2段・最大18の表示枠を扱い、使用済み奇跡を端へ寄せるロジックもある（12722行以降）。したがって旧初期手札10/容量16と現行表示には差がある。ただし **空枠9個＝サーバー初回配布9枚、表示上限18＝未使用物理手札の上限18** とまではこのコードから断定しない。C03は表示側の強い根拠を得た段階として扱う。

| 既存衝突 | 今回の一次確認 | 残る確認 |
| --- | --- | --- |
| C01 光への闇防御 | 公式教典とクライアント `kO` は光攻撃の属性防御を拒否 | 権威的サーバーでの特殊反応・混合防御との照合 |
| C02 病気の自然悪化 | 公式教典は固定5%を説明 | tick対象、抽選順・端点、重複病気、終局順 |
| C03 初期手札 | 公式クライアントは初期表示9枠 | 初回配布数、保持と物理手札の上限・補充の動的契約 |
| C04 属性ワンドの追加威力 | 現行台帳の加算可能ATK2/5と `tw` の加算が一致 | 実際のサーバーHP遷移との照合 |
| C05 反撃への防御 | クライアント `ev` は `gf3()` の攻撃分類で普通防御候補を制限 | counterが各分岐へ入る厳密条件・server受理・多段キュー順 |

## 完全対応へ進める条件

まず公式データとJS由来の純粋関数を旧版とは別名前空間へ固定し、公式台帳・MP・価格・属性合成・UI選択を一次ファイルとの対照テストにする。次に、秘匿状態と見かけ状態を区別した観測設計を導入する。最後に、サーバー側が持つ配布、抽選、応答、反撃、死亡、ターン終了、終末の状態遷移について、取得可能な公式説明・許可されたオフライン資料等で根拠を増やす。

現在のJSは認証済み `submit-command` をCloud Functionsへ送信する（4323–4340行、9670行）、ゲーム更新を受信してイベントを表示する。ここまでの調査に権威的サーバー実装や完全なオフラインエンジンの取得は含まれない。未確定箇所を埋めるまでは「公式公開台帳・クライアント規則への対応」と「公式完全再現」を明確に区別する。

## v27：公式台帳v2とclient出典の同期

v27時点では `official-execution-subset-v27`、Snapshot27（旧1〜26は移行せず拒否）、`official-execution-view-v25`。229モデル/重み490（trade3/weapons105/armor76/sundries17/miracles28）と、配布・効果・抽選・取引・死亡・病気の実験規約はv26から変更しない。公開viewに静的な `catalog_version` / `catalog_sha256` / `client_sha256` を加える。これらは配信資料の識別情報で、隠れた手札・token・乱数状態ではない。`catalog_version="official-v2"`、`catalog_sha256` は正規化した `cards.json` のSHA256であり、元JA全体のhashとは区別する。

| official-v2取得物 | bytes | SHA256 |
| --- | ---: | --- |
| 元JA JSON | 49,240 | `91b82c2600eac771c1a94dec80f249758496e2bbcb7befe6db52dacec1b55b02` |
| 元client JS | 440,420 | `5ece06c224b6a7a5d877eb45d22ea18b82a03aaf1b7161cbbdf6a1e23ceab2cd` |
| 正規化cards.json | manifest参照 | `8e465e2d296a2dd039d6e3b6e22956447c1fc4a5572a31e2597cdf20c551dc2a` |

v1とv2の296件を全field・欠損有無・ID順まで比較し、非表示fieldの変更は0。表示差分は19の画像 `chain-sickle→kusarigama`、26の `glaive-classic→naginata-classic`、117の `slate→stone-tablet` と名称 `石版→石板` の3行4fieldで、現行C++/C/Pythonの台帳へ同期した。旧v1のデータ、元JA/JS pins、crosswalk、過去のgolden vectors、legacy37のfingerprintは履歴として保存する。v1のarchive検査は現在のv2 C++生成物との同一性を要求せず、当時の資料自身の整合を検査する。

pure catalog/rulesの識別子は `official-catalog-client-rules-v2`。C ABIの構造体幅・関数は変更せずcatalog/state ABI1、Match ABI2（command184byte、opcode0〜8、purchase phase3/finished2）を維持する。ABI番号だけで出典まで一致したと判断せず、rulesetとcatalog/client hashも確認する。新Snapshot27はv1台帳を使った旧Snapshot26以前を受理しない。

最新版JSを直接オフライン実行した計算・選択50,270件、状態表示22,140件、資源算術7,898件、合計80,308件が一致した。v1の保存vector、新v2の生成vector、旧clientを再実行したvectorはbyte単位でも一致した。3つの抽出scriptは既知hashのv1/v2組合せを識別し、unknown hashと異なる世代の組合せを拒否する。startupやネットワークを実行せず対象関数/分岐を取り出す。状態照合は抽出分岐と描画stubを含み、全UI操作やserver対局の検証ではない。元ファイルの変更に追従して抽出点を識別する必要があり、旧JSの関数名・行番号を新JSへそのまま当てはめない。

表示差分の未反映gapを解消する更新であり、サーバー規約を補う更新ではない。Dreamの見かけと実体の不適合、守護神の生成/行動閉包、終末、自己BUY、94+Wide、通常giftの満杯時挙動は未対応または未検証のまま。旧37profileは保存し、学習器・学習済みモデルは追加しない。v10のnative Flash未登録とv16のCAPI384照会未実行の訂正履歴、および各版の過去数値を維持する。

[official-v2の取得・再生成](../data/official-v2/README.md) / [v1→v2比較](../data/official-v2/upgrade-review.json) / [v1 archive](../data/official-v1/README.md)。

### v27の検証とv26回帰比較

最新版clientの80,308件の照合は上記の範囲で完了した。focused検証はRelease16/16件（3.68秒）・ASan/UBSan13/13件（5.07秒）が通過した。Matchのnative88群/Python63件は同件数、pure OfficialDataには最新表示3行を検査する1methodを追加した。旧v1 archiveとlegacy37のfingerprint保全も検査する。

seed42のv27・worker4で、2人100,000試合と9人10,000試合をv26の同条件worker4へ比較し、ruleset・追加の静的provenance・時間を除く全gameplay/既存counter fieldが一致した。意思決定数は3,988,255 / 2,409,699、checksumは `11173370200104368305` / `10091894475017822334`。他の勝数・draw・打切り・overflow集計もv26表と同じである。これは**v26との同条件worker4回帰比較**であり、v27単独のworker1/4再計測ではない。Release/SAN各1,000試合も一致し、2人/9人のchecksumは `760147037809871428` / `4393572629372685365`。quiet速度の再計測や速度改善比は主張しない。

v27の最終検証でRelease CTest130/130件（54.34秒）・ASan/UBSan90/90件（87.17秒）が通過し、両buildはwarning/error0だった。時間は並行suite実行の経過であり性能比較には使わない。最新clientから3oracleを独立に再実行し、全80,308件のTSVとv2の3manifestがbyte一致することを確認した。出力fileのsymlink6件・hardlink6件を有効client入力で拒否し、companion sentinelとv1入力のhashが不変、通常regular出力への再生成は成功した。未知hash・旧新不一致9件も再確認した。

## v28：互換クラスに限定したDream

`official-execution-subset-v28` はDreamの36/80/183/222を加え、233モデル/重み494（trade3/weapons107/armor77/sundries17/miracles29）を扱う。Snapshot28は旧1〜27を拒否し、公開JSONは `official-execution-view-v26`。Match ABI2の184byte command、opcode0〜8、purchase phase3/finished2、catalog/state ABI1、official-v2の出典3fieldは維持する。残る通常配布4モデルは178のredraw、守護神201/239、超常現象207（終末GFのscheduleも別途未対応）である。旧37profileと過去の記録・訂正を保存し、学習器を追加しない。

### 一次根拠と独自制限

公式JA/ENは、Dream中に新たに与えられる神器が50%の確率で別の神器に見えると説明する。公開clientは個体token・実モデル・fakeモデル・usedを分離し、手札faceと選択/MP表示ではfakeを優先し、送信には個体tokenを使う。使用時は受信した実モデルをrevealし、Dream解除時にもfakeを解除する。これは睡眠・行動skip・confusionによる自動行動ではない。

一方、公式の候補集合・重み、見かけ上合法で実体不適合な入力をserverがどう受理するか、売買・redraw・保持済み奇跡にいつ偽装するかは未確定である。本実装はこれらを推測で埋めず、**同じ選択・MP・target・commandの形を保つ互換クラスだけに限定した実験mask**を採用する。公式Dreamの全再現とは呼ばない。178は自身へのDreamだけでなく全手札redrawを含むため、今回の4種へ混ぜない。

| 追加モデル | 通常role / Dream role |
| --- | --- |
| 36 | 無属性ATK10武器・被害時Dream |
| 80 | 木ATK4武器・被害時Dream |
| 183 | 木防具・純Dream反撃（非attack/power0） |
| 222 | 木・MP6の直接Dream奇跡、used保持/FREE対象 |

36/80の付与は既存の通常targetdamage段階の実HP損失、183は既存survivor counter/FIFO、222は非attackの直接付与として扱い、trigger・死亡・反撃・補充順は既存の実験規約を引き継ぐ。Dreamは病気とは別に重複を持たず、Heavenへの非病気付与で転落させない。自然tickは追加しない。mild解除はDreamを残し、all解除はDreamと現在の全fakeを解除する。

### 互換クラスと抽選

signatureは `category, element, ability, curse, cost, hit_rate, is_plus_atk, atk>0, def>0` の9keyを完全一致させる。v28当時の候補は233 supportedモデルに限り、actualと同じIDは除く。146モデルに別face候補があり、87モデルはsingleton、全体は117クラス・異なるactual/faceの順序付きpairは992組である。数値ATK/DEF/abilityValue/priceの同値は要求しないので、表示威力・表示値と実際に解決される値が異なることはある。選択互換は効果値の同値を意味しない。

通常giftで実モデルを従来の重み付き抽選後、受取人がDream中かつ別の互換face候補がある場合だけbounded2を1回引く。0ならmaskし、候補からuniform boundedをもう1回引く。face候補が1個でもそのdrawは消費する。1ならmaskしない。Dream無し/候補無しは追加drawを行わない。したがって「eligible giftの50%」であり、全モデルが一律に50%変装する実装ではない。候補分布・singleton除外・抽選順は公式serverで未確認の独自規約である。

既存手札を遡及して変装しない。tradeで新たに受け取る商品には新maskを付けない。closed stateが受理するfakeはDream所持者の未使用個体で、actualと異なるsupported互換faceだけ。使用済み奇跡のfakeや互換クラス外のfakeをraw importで持ち込むことはできない。dead/終局の本人tooltipに実体を表示する契約と、手札faceの表示は区別する。

### 選択・reveal・公開境界

`legal` とsamplerはfaceに基づき選択し、実モデルで消費・MP・効果値・価格・payloadを解決する。受理された使用はactualをrevealし、保持される奇跡もunmaskする。失敗したcommandはreveal・抽選・token・状態を変更しない。既存fakeの売却/購入/除去/overflowもactualをrevealしてunmaskする。BUYは商品選出時にunmaskし、NO・CP不足・cutoffで返却しても元tokenはunmaskedのまま返す。このlifetime・売買適用範囲もローカル規約である。

公開 `last_reveals` は `[{owner,face,model}, ...]` として、使用・応答・quote・除去・overflowによる公開itemイベント相当のrevealを同じaction内の順で保持する。all解除で残手札のfakeを解除するrevealは、内部で `owner_only=true` とし、viewerが本人のときだけこの配列へ含める。他viewer・観戦者には該当entryを完全にfilterし、件数・face・実モデルも漏らさない。`removeCurses` が受信するcurse一覧は、他者の全手札を公開する根拠ではない。個体token・slot・元候補数・RNGは出さない。次の受理mainでclearし、失敗入力では保持する。抽選累計 `dream_rolls` / `dream_masks` やreplay anchorは特権snapshotだけに置き、プレイヤー向けviewへ渡さない。

v28時点の公開 `dream_rule` は当時のheaderと同じ次の文字列とする。これを公式server仕様の識別子とは扱わない。

```text
experimental:compatible-face-only;key-category-element-ability-curse-cost-hit-rate-plus-atk-positive-def-positive;supported-other-models-in-id-order;gift-real-before-eligible-only-half-coin-zero-masks-before-uniform-face;singleton-no-extra-draw;face-choice-one-draw-including-one-candidate;no-retroactive-mask;trade-receipts-unmasked;legal-and-policy-visible-models;resolve-actual-values-and-price;use-quote-removal-overflow-reveal;quote-return-stays-unmasked;all-cure-clears-dream-and-reveals-hand-only-to-owner;mild-preserves;no-tick;redraw178-unsupported
```

### v28の互換性照合と検証

[再現script](../scripts/official_dream_oracle.cjs) と [互換クラス報告](../data/official-v2/dream-compatibility-v28.json) を追加した。v28当時の233モデルに対し、別faceを持つのは146モデル（gift重み321）、singletonは87モデル（重み173）。全miracle・tradeと今回のDream付与4種はsingletonで、現在のfake生成対象は非miracle物理品だけである。保持MCをunmaskする共通実装があっても、現クラスで偽装MCの通常到達を検証したという意味ではない。

992順序付きpairについて、開始/既定target1,984件・MP計算14,880件・main追加693,408件・response追加1,111,040件、計1,821,312件のクライアントpredicate比較を報告に記録した。これらは検査したprefix/response contextに対する互換性であり、威力・回復値・DEF・価格の同値や、公式の偽装候補分布・server全入力受理を証明しない。公式の50%説明を満たす対象も本実装では146/233に制限される。台帳・allowlist・predicateが変われば再生成と閉包の再検査が必要である。

CAPI/Pythonのfocused検証2/2件が通過し、Pythonは67件が通過した。v27の80,308件・native88群/Python63件と、その全suite130/90件の記録は前版の証拠であり、今回のDream追加検証として読み替えない。今回の全検証結果は次のとおり。追加drawがあるため、v27の対局checksum不変は要求しない。速度改善比は主張しない。

v28の最終検証はRelease CTest130/130件（20.90秒）・ASan/UBSan90/90件（44.44秒）が通過した。native94群を全てmainへ登録し、Python67件・CAPI固定query384件も通過した。Dream oracleの `--check` で保存reportがbyte一致し、1,821,312比較を確認した。時間はsuiteの経過であり、版間の速度比較には使わない。

| v28・seed42 | 試合数 | 意思決定数 | checksum | eligible coin / masks | 公開reveal / 本人限定reveal |
| --- | ---: | ---: | --- | ---: | ---: |
| 2人 | 100,000 | 3,978,368 | `18145630014188039408` | 80,134 / 39,946 | 28,241 / 1,149 |
| 9人 | 10,000 | 2,408,047 | `11938917261778849633` | 69,094 / 34,635 | 24,346 / 1,633 |

同条件の1worker/4workersはworker数・時間・速度を除く全fieldが一致した。Release/SAN各1,000試合も全fieldが一致し、2人/9人のchecksumは `16592869786979077265` / `14472242390983499012`。coinはeligible giftだけの抽選回数、masksは生成したfake数であり、全giftが50%対象だったという集計ではない。revealの二分類はbench内部集計で、本人限定件数を他viewerの観測へ公開しない。版間の速度改善比は主張しない。

クライアントpredicateとの一致を検証しても、未確定の公式server候補分布や実体不適合commandの受理規約が確定するわけではない。178のredraw・守護神201/239・超常現象207（終末GFのscheduleも別途未対応）の残件と、自己BUY・94+Wide・通常gift満杯の別gapを維持する。

## v29：夢見る帽子178の神器一新

`official-execution-subset-v29` は178を加え、234モデル/重み495（trade3/weapons107/armor78/sundries17/miracles29）を扱う。Snapshot29は旧1〜28を拒否し、公開viewは `official-execution-view-v27`。Match ABI2のcommand184byte・opcode0〜8・purchase phase3/finished2、catalog/state ABI1、official-v2の出典を維持する。残る通常配布3モデルは守護神201/239と超常現象207（終末GFのscheduleも別途未対応）。Dreamの互換クラス外の偽装分布・実体不適合入力等は未確定のままで、公式Dream完全再現とは呼ばない。legacy37・過去の検証値・v10/v16訂正履歴は保存し、学習器を追加しない。

### 公開clientと旧Flashの限界

公式178は木DEF14・`selfCurseAndRedraw` / Dream・価格15・giftRate1で、説明は「使用者に夢と神器一新」。最新版JSの `redraw` は演出、`replaceItems` は受信したplayerId/itemsによる置換である。後者は対象の公開usedMiracles paneをclearし、本人のhandでは空placeholderを保ちつつ全nonempty表示slotをitemsで置き換える。used奇跡を除外する分岐は無い。usedMiraclesNoteの対象外記述はSELL/BUY/Broomについてであり、redrawのused保持を保証しない。これらは置換表示の根拠で、どの品を何枚抽選し、いつDreamを付け、複数帽子をどの順で解決するかを生成するserverロジックではない。

旧Flash相当品は旧ID184（木DEF14・ILLUSION）で、固定sourceのdefenderCommandは消費→補充1予約→自身ILLUSION→DEF減算だけを行う。全手札一新/REFRESHの処理は無く、別配列の保持magicsや帽子以外のpiecesを維持する。旧版の全queue後owner病気・敵助手機会・各alive playerへの補充や、Heaven中の任意harm付与HP0を、現行一新の根拠へ流用しない。

### 実験用の置換・会計規約

応答で選択された品を全て消費した後、選択順に178各copyを処理する。各copyで自己Dreamを付け、その時点の全nonempty実物手札（used奇跡を含む）を同数の新token・unused品へ即時置換する。空の補充予約placeholderは抽選枚数に含めず、既存の `s.gifts` 予約を維持する。抽選は現在の234/495限定プールのweighted drawと、従来のcompatible Dream maskを使う。帽子本体を消費した空きへ先に補充を実行する規約ではない。複数copyは各時点の新たな手札を次のcopyが再置換する。

used奇跡も新札へ置き換わるため元の能力は失うが、既に付与したcurseや実行済み効果を巻き戻さない。選択時のDEF14等は保存し、旧tokenが置換で無くなってもその応答の防御量は変えない。一新はincoming body・復活/死亡確定より前に行い、HP0の内部状態を途中snapshotとして公開しない。同actionの後続responseでは新札を使用できる。discardや売買の商品として動かした178は一新を発動しない。これらの順・枚数・token再発行・新札の即時使用はserver未確認のローカル規約である。

新札が同action中に追加されるため、物理品消費を初期人数×手札上限だけで制限した旧journal会計は流用できない。受理command全体を `max_decisions` で制限し、全action journalから消費・即時draw・MP・予約gift・復活・反撃と後続新札使用をreplayする。通常giftは従来どおり全action後で、即時redrawと別である。

公開 `last_redraws:[ownerID,...]` は全viewerへ一新対象のowner IDだけを知らせる。同actionで複数回なら順に記録し、旧手札の枚数・used数・fakeの有無/face/actualモデルは出さない。次の受理mainでclearし、失敗commandでは保持する。置換だけを理由に既存fakeの `last_reveals` を公開しない。本人は通常の自分のhand viewから新札を見られる。累計 `redraws` / `redrawn_items` とRNG/token/journalは特権状態で、public viewへ出さない。all解除の残fake revealは引き続きowner-onlyであり、一新通知と混同しない。

### v29の公開規約と検証

`redraw_rule` は次の文字列である。

```text
experimental:response-only178;consume-entire-selection-first;selection-order-self-dream-before-each-redraw;replace-all-current-nonempty-including-used-miracles;same-count-fresh-unused-tokens;immediate-weighted-draw-with-compatible-dream-mask;reserved-gifts-excluded-and-preserved;before-incoming-body-and-revival;no-old-hand-models-or-counts-public;owner-id-only-redraw-notice;same-action-new-items-usable;no-trigger-on-discard-or-trade;max-decisions-bounds-journal
```

`dream_rule` は末尾を `redraw178-same-mask-policy` へ変更する。

```text
experimental:compatible-face-only;key-category-element-ability-curse-cost-hit-rate-plus-atk-positive-def-positive;supported-other-models-in-id-order;gift-real-before-eligible-only-half-coin-zero-masks-before-uniform-face;singleton-no-extra-draw;face-choice-one-draw-including-one-candidate;no-retroactive-mask;trade-receipts-unmasked;legal-and-policy-visible-models;resolve-actual-values-and-price;use-quote-removal-overflow-reveal;quote-return-stays-unmasked;all-cure-clears-dream-and-reveals-hand-only-to-owner;mild-preserves;no-tick;redraw178-same-mask-policy
```

9keyの互換制限は維持し、178はsingletonとして追加する。146 eligibleモデル（重み321）・88 singleton（重み174）・118クラス・992順序付きpairで、全234モデルが50%変装する規約ではない。v28の1,821,312比較・native94/Python67とその対局値は前版の証拠として保持し、今回の一新を検証した値へ読み替えない。新版の[redraw再現script](../scripts/official_redraw_oracle.cjs) / [client報告](../data/official-v2/redraw-client-v29.json) は、実 `fH.lI`・実item decoder・実 `j9.jF` をオフライン抽出し、DOM/widgetだけstubに置き換えた。通常置換223例・不足配列1例・used pane19例を記録する。非empty slotにはused奇跡も含み、空placeholderを維持すること、used paneを全clearすることを照合した。不足配列でthrowし、余剰配列は未使用になるというrendererの挙動も含む。受信itemsのtoken/model/fake/usedを使う表示処理であり、serverの置換枚数・new token発行・used=false再配布・帽子/curse/body/復活/giftの順を証明しない。各helperを個別に呼ぶため、実際の全viewerへのevent fanoutや全UI操作の検証とも区別する。

新版の[Dream再現script](../scripts/official_dream_v29_oracle.cjs) / [互換報告](../data/official-v2/dream-compatibility-v29.json) は234 IDs/118クラス/146 eligible/88 singleton/992 pairを記録する。開始/既定target1,984件・MP14,880件・main追加696,384件・response追加1,111,040件、計1,824,288比較で、検査したclient選択・MP・targetの互換性を照合する。威力/価格/回復値の同値や公式偽装分布は保証しない。旧v28 script/reportは当時の233モデルの証拠として保存する。

seed42の大規模対局では、1worker/4workersで時間・worker数・速度を除く全fieldが一致した。

| v29 | 試合数 | 意思決定数 | checksum | redraw / redrawn items / notice | Dream coin / masks |
| --- | ---: | ---: | --- | ---: | ---: |
| 2人 | 100,000 | 3,994,686 | `12366315977385056288` | 4,304 / 21,660 / 4,304 | 119,719 / 59,481 |
| 9人 | 10,000 | 2,411,205 | `14932456998465571650` | 3,433 / 21,517 / 3,433 | 109,228 / 54,853 |

9人のtrade overflowは6件。redraw/itemsは特権bench集計で、各viewerへ旧手札枚数を公開するものではない。noticeの公開内容はowner IDだけである。これらの対局結果と、最終native/bridge/full suite検証は別に記録する。速度改善比は主張しない。

今回のnative検証は100群を全てmainへ登録した。使用済みTurbulence→自己bounce→帽子の順で、保持されていた反応carrierが一新で消えてもreplayできる接合を含む。nativeのRelease/SAN単体、Python70件・CAPI/Python focused2件・固定query384件が通過した。Release/SAN各1,000局も全fieldが一致し、2人/9人のchecksumは `10532187165089695254` / `6271775957059974581`。これらは独自規約の内部整合・来歴再生の証拠であり、公式server順を確認した証拠とは分ける。

v29の最終全suiteはRelease **130/130件**（17.20秒）、Sanitizer **90/90件**（47.06秒）が通過した。native100群はすべてmainへ1回ずつ登録され、Python70件、CAPI/Python focused2件、固定CAPI照会384回を実行した。新redraw oracleとDream v29 oracleの `--check` は保存reportとbyte一致し、旧v28 oracle/report・旧v1 archiveは変更していない。大規模1/4worker比較とRelease/Sanitizer各1,000局の非時間field一致は上記のとおり。これらはv29のローカル規約の検証であり、公式serverの配布枚数・抽選順・手番scheduleの同値性を証明しない。

## v30：自己BUYの入力と商品照会

`official-execution-subset-v30` は同じ234モデル/重み495（trade3/weapons107/armor78/sundries17/miracles29）のまま、自己BUYを実験規約で受理する。Snapshot30は旧1〜29を拒否し、公開viewは `official-execution-view-v28`。Match ABI2のcommand184byte・opcode0〜8・purchase phase3/finished2、catalog/state ABI1、official-v2の出典は維持する。新しいopcodeは追加しない。旧版の数値・v10/v16訂正履歴・legacy37は保存し、学習器を追加しない。

### 入力の一次根拠と実験規約

現行clientのtarget選択は自己paneを選べ、通常BUYのsubmitは個体tokenとtargetPlayerIdを送る。BUYの自己targetをUIが常に拒否するという根拠は無い。ただし、自己BUYが選ぶ商品、資金条件、応答・送金・移動・補充のserver処理を確認したことにはならない。既知のalive本人IDを明示targetにすることと、target省略は区別する。

固定clientの通常クリック経路も確認した。main開始の `fF` は `h8.aG()` の全alive playerへ `si1(true)` を呼び、本人を除外しない。各playerのtarget overlayに結ばれた `A.lz.$0` は選択先を `jO.aT` へ渡す。BUY選択時のenemy指定は初期値で、その後のalive本人クリックを禁止する分岐は無い。このcaller確認は固定sourceの静的追跡による根拠で、108ケースの実method試験と合わせて入力経路を確認した。

補助資料の旧Flashは、操作BUYを消費し補充1を予約した後、自己なら自動空防御から物理手札を1個選び、COMMANDへ選択品を載せて即終了する。商品を動かさず、後段の資金判定やYES/NO待ちへ到達しない。旧版は保持magicsが別配列であり、現行のused個体形式と同一ではない。この旧経路は今回の実験規約を選ぶ参考で、現行server一致の証明ではない。

主手番の `Command(actor, kind="buy", items=(buy_token,), target=actor)` は本人の未使用物理5一個を使う。allocation・MC/FREE混入・play5は許さず、MP費用0。物理5を消費後、その時点のunused自手札をinventory順に列挙し、一様bounded抽選を1回行う。1候補でも1draw、空なら0draw。使用済み奇跡は候補外で、別のBUYやpassive品・未使用奇跡は候補になり得る。選択品のactualを公開してunmaskし、操作5を消費した後の在庫で同じslot/token/used=falseを保ったまま置く。操作消費による通常のindex詰めを禁止する意味ではない。移動・escrow・新token配送・再抽選はしない。この抽選集合/分布/順はローカル規約である。

資金条件やCP送金は無く、priceは公開商品の値であり料金ではない。自己response・quote・YES/NOは作らず即時完了し、操作分gift1だけを全action後へ予約する。自己targetはFogによる敵対象変更から除外し、samplerも全alive対象へ本人を含める。対象無しや未知/死亡targetを許す契約ではない。候補空時の0drawは照会本体だけの話で、その後の通常giftによる抽選まで0という意味ではない。

### 結果・privacy・cutoff

非空なら既存の `last_purchase` に `reason="self_inspection"` を記録し、`bought=false` とする。native `PurchaseReason::self_inspection` は既存enumの末尾value5へ追加し、旧reason値は変えない。buyerとsellerは同じ本人IDで、商品model/priceは実体から取る。空なら既存の `empty` 結果（model0/price0）を使い、buyer=sellerを許す。private slot/token/候補数/RNGは公開しない。fake商品の照会は既存の公開item revealと同じローカル規約で `last_reveals` に載せ、本人限定のall解除revealとは分ける。

自己照会のbodyが完了した後にdecision cutoffへ達しても照会結果とunmaskは保持し、予約giftだけを取消す。次の受理mainで結果をclear、失敗入力では保持し、reset/loadではclearする。照会だけでcurseを解除したり、商品をusedへしたり、HPを変えたりしない。その後のowner病気tick・死亡/復活・gift/次ownerは既存の全action完了規約に従う。他者BUYの応答・資金条件・quote/YESNO・escrow返却は従来の別経路を維持する。

今回解消するのは自己targetを一律unsupportedとしていた入力gapである。自己照会の公式server規約が確定したという意味ではない。守護神201/239と超常現象207（終末GFのscheduleも別途未対応）は未着手の残件である。旧助手HP20/30%行動/肩代わり盾を現行へ流用しない。現教典の敵ターン後25%・被害時10%退去、現42行動の表示台帳と、種類の抽選・対象・Earth/Moonから神器/奇跡/召喚/現象へ広がる閉包のserver scheduleは区別し、後者を未確認のまま導入しない。

### v30の検証

[自己BUY再現script](../scripts/official_self_buy_oracle.cjs) / [client報告](../data/official-v2/self-buy-client-v30.json) は108ケースで実target選択 `jO.aT`・実alive click roster `h8.aG`・実team判定 `c5.ce`・実MP計算 `A.ld` と、submit `jS.$0` の最初のawaitまでを抽出実行した。自分/敵/味方のtarget、同じtargetの再選択、ID0/最大safe integer、MP0/CP0/Fog/Dreamを含む。108のRPC payloadをローカルstubで捕捉し、自己targetPlayerIdと物理itemIdsが送信され、このpreflightでCPを参照しないことを記録した。DOM/音/Future/backendはstubで、アプリ起動・ネット接続・server応答経路を実行しない。直接methodのdead/unknown-object検査不足は、通常alive rosterからのUIクリックやserverがそれらを受理する証拠ではない。

このoracleは照会商品の抽選、資金条件、送金、消費・gift、reveal、死亡・cutoffのserver順を証明しない。これらは上記のローカル規約である。v29以前の検証数・対局checksumは、その時点では自己BUYを拒否していた履歴として保持する。新native/C/Python・対局回帰の最終結果は以下に記録した。

公開 `buy_rule` は次のexact文字列とする。MatchのABI幅は据置だが、結果reasonを読むclientは新 `self_inspection` を認識する必要がある。

```text
experimental:one-operation-gift;uniform-unused-commodity-in-inventory-order;singleton-one-draw;empty-no-draw;foreign-cp-only-affordability;foreign-buyer-accept-or-decline;foreign-escrow-return-on-decline-or-cutoff;foreign-fresh-token-delivery;foreign-full-recipient-uniform-existing-item-overflow;self-immediate-inspection-after-operation-consumption;self-public-actual-model-and-unmask;self-same-slot-token-and-unused-state;self-no-cp-check-or-transfer-or-quote;self-result-before-cutoff-and-gift-after-action
```

今回のnative106群を全てmainへ登録し、Release/SAN単体が通過した。Python74件・CAPI/Python focused2/2件・固定query384件も通過し、自己BUYのprimary oracle108ケースはrootの `--check` で保存reportとbyte一致した。Release全CTest130/130件（17.18秒）は通過済み。SAN全suiteの結果は末尾の最終検証欄に記録した。

| v30・seed42 | 試合数 | 意思決定数 | checksum | 自己BUY = 非空照会 + 空 | foreign quote |
| --- | ---: | ---: | --- | ---: | ---: |
| 2人 | 100,000 | 3,991,570 | `9150208344638119859` | 45,014 = 44,283 + 731 | 42,051 |
| 9人 | 10,000 | 2,400,812 | `5747068493126455233` | 10,861 = 10,717 + 144 | 49,744 |

同条件1worker/4workersはworker数・時間・速度を除く全fieldが一致した。Release/SAN各1,000試合も全非時間fieldが一致し、2人/9人のchecksumは `127032000236014493` / `11541973377553558042`。BUYは既存trade categoryであり、対応プール234モデル/重み495を増やす更新ではない。official-v2のpinsとv29のDream/redraw oracleは変更しない。これらの対局集計は実験規約の内部整合の検証で、未対応scopeを実装済みとしたり、公式serverと同じ購入動作を証明したりするものではない。速度改善比は主張しない。

v30の最終全suiteはRelease **130/130件**（17.18秒）、Sanitizer **90/90件**（46.99秒）が通過した。native106群は全てmainへ1回ずつ登録し、Python74件・CAPI/Python focused2件・固定CAPI照会384回を実行した。自己BUY oracle108ケースは保存reportとbyte一致した。これらは実験規約の自己照会と既存scopeの回帰検証であり、自己BUYの現行server処理・守護神・超常現象・終末の公式完全対応を証明するものではない。

## v31：外部指定の守護神42行動

`official-execution-subset-v31` は、通常配布234モデル/重み495を維持したまま、別入口でcanonical guardian model245〜286の42行動を実行する。42種をgiftプールや通常手札へ入れる更新ではない。Snapshot31は旧1〜30を拒否し、公開viewは `official-execution-view-v29`。既存Match ABI2のcommand幅/opcode0〜8/phaseは変えず、24byte `GuardianAction` と追加のguardian入口で公開payloadを受ける。catalog/state ABI1、official-v2の出典、legacy37、過去の数値と訂正を維持し、学習器を追加しない。

これは**外部から選ばれた行動を進める実験基盤**であり、自動守護神scheduleではない。召喚201/239、守護神の種類抽選、敵ターン後25%の行動機会、被害時10%の退去、Earth/Moonによる一般神器/奇跡選出、超常現象207と終末GFは未実装のまま。scene importでguardiansを持つ状態を用意することと、公式の召喚・行動選択を再現することは区別する。特権sceneやsnapshotには他者の実手札を含み得るので、agentへそのまま観測として渡さない。

### 入力・進行の実験規約

native `GuardianAction{source, model, target}` はmain境界でのみ受理する。sourceはaliveで、stateのpersistent guardianが選んだcardのguardian metadataと一致する必要がある。sourceが現在のordinary ownerと同じである必要はない。280だけtarget無し、その他41種は既知のalive targetを明示する。自分・味方を含む指定を受け付けるのはこの外部入力の規約で、公式自動selectorが全ての指定を作るという証明ではない。

1回の外部注入を1decisionとして数え、必要な各responseも従来どおり別decisionとなる。sourceの手札・MPを消費せず、操作分giftも予約しない。相手の応答品・復活・285の補充は既存の予約giftを使う。ATK payloadは自己指定でもresponseを作り、非attackの自己対象と280は即時解決、その他の非attack foreign対象も通常responseを待つ。確率行動はこの明示target一人へ従来のhit抽選/DarkCloud保証を適用し、redirect後に再抽選しない。Fogを理由に外部指定targetを選び直さない。

264/286の `categoryWeapons` だけは実効categoryをweaponsへ変える。他40種はguardiansであり、全てを武器や奇跡へ置き換えない。通常ATKには属性に沿う普通DEF/filterと190、nonattackには190が適合し、264/286では武器専用反応もその属性条件に従う。metadataのATK/属性/curse/量を使い、効果の反射/反撃/死後FIFO・178の新札・復活は既存action journalで再生する。元guardianを表示するmetadataと、redirect後のcurrent source/targetは分離する。

全外部actionと反応が完了した後にalive分のgiftを処理し、ordinary ownerの病気tickやGF進行は起こさない。元ownerがaliveならそのmainへ戻り、死んでいれば次のaliveへ戻す。自然終局/decision cutoffは従来の優先規約を使う。これらのstage・応答・gift/owner復帰はserver未確認のlocal scheduleである。

### 効果の一次根拠と未確認selector

| model | 外部payloadの解釈 | 根拠と限界 |
| --- | --- | --- |
| 280 boostCPOfEverybody | 全aliveのCP+1、支払元無し、target無し | 受信handlerはalive pane列へbc(+cp)だけを適用。誰にいつ発行するかは別 |
| 281 boostCPToEnemy | 明示current targetのCP+5、source支払無し | 教典「敵にあげる」とboostCP受信のtarget加算。敵selectorの分布・許容対象は未確定 |
| 282 takeCP | current targetから名目3を減算→current sourceへ名目3を加算 | 受信takeCPと同じ順。clip99/不足CP→MP→HPを含み、実残高だけに請求を制限しない。自己aliasでも逐次 |
| 275 / 276〜279 | 全curse解除 / HP5・MP5・HP10・MP10のcurrent target回復 | 現台帳と受信効果。旧助手表のHP/MP順を使わない |
| 285 addItem | current targetへ通常gift1を予約、満杯ならgift draw0 | 一個付与の説明・addItem受信は根拠。候補選出/即時配送ではなくdeferredにするのは独自規約 |
| 286 categoryWeapons | 無属性ATK10の武器として実行 | clientがcategoryWeaponsを反応判定へ適用。Moonによる任意奇跡の選出/合成を実装した意味ではない |

他の固定行動も現42recordを使う。通常ATK、HP吸収、被害時Fog/Dream、純Dream/Flash/DarkCloud付与などの意味は既存bodyへ接続する。guardianから生じたという理由で物理品/保持奇跡を取得・再使用した扱いにしない。恒常guardian state、attack payloadのguardian、187/188/235/236等のcard metadataは別の情報である。201/239には固定guardian fieldがなく、10種uniform召喚の根拠を与えない。

現教典の25%行動・10%退去と、旧Flashの30%行動・HP20肩代わり盾は異なる。現MARSのATK3〜7、SATURN2/4/6/9/15やcategoryWeapons、NEPTUNEのHP/MP順、PLUTOの純curseなども旧表と異なる。旧Moon/Earthの一般神器再抽選、旧Heavenに任意harmで即死、旧Mysteryの木星ILLUSION・金星全員99等を現行へ流用しない。外部固定42を扱えても、召喚からEarth/Moon→201/239/207→現象へ広がる全選出閉包を実装したことにはならない。

### 公開境界・規約・検証

公開 `last_guardian` は受理された外部指定のsource/model/guardian/targetだけを示し、待機payloadにもguardian identityを表示する。未来の種類/行動/対象抽選やtoken・RNG・私的journalは公開しない。persistent guardianの状態欄は既存のFog可視性に従い、非表示になる場合がある。実行済み行動のguardian表示と、persistent状態欄・将来の抽選状態は分離する。`guardian_legal` もmatching guardian等の特権状態を参照する外部制御APIであり、player観測だけに基づく照会ではない。player向けvisible-only selection APIと同じ非干渉性を主張せず、特権scene/制御をagent観測へ混ぜない。入力失敗は従来どおりatomicで、応答待ち中に別の外部行動を差し込むことはできない。

v31時点の `guardian_rule` は次の文字列とする。

```text
experimental:externally-supplied-canonical-model245-286;main-boundary-only;source-owns-matching-guardian;explicit-alive-target-except280-all-alive;no-summon-move-target-departure-or-opportunity-sampling;metadata-values;categoryWeapons264-286-only;no-source-items-mp-fee-or-operation-gift;single-target-percent-roll-and-darkcloud-no-reroll;self-nonattack-and280-immediate;ordinary-response-counter-death-redraw-and-gifts;no-owner-tick-or-gf-advance;resume-owner-or-next-alive;one-decision-per-injection;earth285-one-deferred-gift;earth-moon-ordinary-model-selection-unsupported
```

[守護神再現script](../scripts/official_guardian_oracle.cjs) / [client報告](../data/official-v2/guardian-client-v31.json) は、実 `A.p6` attack decoder8ケース、実 `fs.fu` 受信frame・`gf2/iI` attack表示・`tz/lc/ld` 算術・`eu` 応答追加などの664投影を記録する。内訳は42原record×4条件の168、通常239モデルの単品239、仮Earth/Moon compound239、動的表示18。10種のguardian表示のreplaceとclearも個別に照合し、DOM/widget/CSS/text表示先はstubに置く。application startup・通信・serverの行動選出/phaseは実行しない。

通常239モデル単品や仮compoundの入力は、公式selectorがそれらを生成するという証拠ではない。decoderが任意DTOのatk/hitRate/mp/cpを読み、表示関数がoverrideを使うことと、実行器がそれを受理することも区別する。今回のAPIは245〜286のcanonical一モデルとmetadata値だけで、DTOの任意overrideやEarth/Moonの一般神器/奇跡合成は未対応。clientのtarget欠損が0へdefaultする挙動を、実行APIのtarget省略可否へ自動的に移さない。after-filter応答maskも、filter prefix自体の合法性を満たす場合だけ完成した応答として使える。

native `guardian_legal` / `guardian_step` とC `gfo_match_guardian_legal` / `gfo_match_guardian_step`、Python `GuardianAction` / `Match.guardian_legal` / `Match.guardian_step` を追加する。scene入口はC `gfo_match_load_scene`、Python `Match.load_scene(OfficialState, owner, seed)`。同じlibraryのlive stateをscope検査後に独立deep copyし、既存Configを維持してRNG/counter/pending workを初期化する。これはsnapshot checkpointの復元やserverイベント再生ではない。Pythonはrawのpending attacksを拒否し、callerはquiescent sceneを渡す。通常の `sample_action` は守護神を自動選択しない。

新native8群を含む114群が通過した。守護神専用bench `./build/godfield_guardian_bench 420 9 42 2` のsmokeでは42モデルを全てcoverし、955decisions（420外部指定＋535responses）、70rolls/41hits/60forced、99redraws、checksum `7872010414762406687` を記録した。これは外部入力の局所試行で、守護神を自動抽選する通常対局の統計ではない。大規模回帰と最終suite結果は以下に記録した。速度比較は主張しない。

clientの受信表示を実行しても、召喚・selector・各stageを生成する権威的serverと同じと証明するわけではない。

primary oracleはrootの `--check` で保存reportとbyte一致した。native114群のRelease/SAN単体、Python78件・CAPI/Python focused2件・固定query384件が通過した。最終全suiteは末尾の最終検証欄に記録した。

通常対局のseed42・2人100,000試合/9人10,000試合は1worker/4workersで全非時間fieldが一致し、rulesetと新guardian_ruleを除くgameplay fieldはv30とも完全一致した。通常checksumは `9150208344638119859` / `5747068493126455233`。Release/SAN各1,000試合も一致し、checksumは `127032000236014493` / `11541973377553558042`。通常samplerが守護神を自動起動した結果ではなく、既存対局へ影響を与えていない回帰確認である。

| 外部守護神・seed42 | 試行数 | decisions / responses | covered models | terminal / cutoff | rolls / hits / forced | redraws | checksum |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 2人 | 100,000 | 227,967 / 127,967 | 42 | 2,967 / 0 | 16,667 / 10,052 / 14,286 | 24,125 | `2684282905704164173` |
| 9人 | 10,000 | 22,826 / 12,826 | 42 | 0 / 0 | 1,668 / 1,021 / 1,430 | 2,412 | `4158330987123714582` |

専用試行も1worker/4workersで全非時間fieldが一致した。Release/SANの420試行smokeもchecksum `7872010414762406687` が一致した。専用試行の42coverageや終局件数は外部指定sceneの検証で、公式の25%行動機会・種類/対象抽選や通常自動対局を評価した結果ではない。速度改善比は主張しない。

v31の最終全suiteはRelease **131/131件**（42.29秒）、Sanitizer **91/91件**（100.09秒）が通過した。native114群は全てmainへ1回ずつ登録し、Python78件・CAPI/Python focused2件・固定CAPI照会384回を実行した。新しい `official_guardian_benchmark` CTestも両buildで通過した。READMEのPython最小例をそのまま実行し `main 0 0 25` を確認した。これらはcanonical42行動の外部実行と既存対局の回帰検証であり、公式の召喚・自動guardian選出・超常現象・終末scheduleを再現した証明ではない。

## v32：召喚とcanonical守護神行動の通常対局接続

`official-execution-subset-v32` は201守護封印のつぼと239＜解放＞を加え、236モデル/重み499（trade3/weapons107/armor78/sundries18/miracles30）を通常配布する。Snapshot32は旧1〜31を拒否し、公開viewは `official-execution-view-v30`。Match ABI2の幅/opcode/phase、catalog/state ABI1、official-v2のpinsは維持する。外部 `GuardianAction` の入口も残す。残る通常giftモデルは超常現象207であり、終末GFのscheduleと同一視しない。legacy37、過去の数値・v10/v16訂正は保存し、学習器を追加しない。

### 一次事実と今回の独自selector

現教典は敵のターン後の行動率25%と、ダメージ時の退去率10%を説明し、公開台帳は10種の守護神と42行動を持つ。最初の8種の5行動はguardianAttackRate6/5/4/3/2（各合計20）で、clientの教典表示も20を分母にする。Earth/Moonの285/286には同じ重み表が無く、公開clientは種類/行動/targetを選ぶserverを実装していない。旧HP20の盾・30%行動・旧Earth/Moonの一般神器filterを持ち込まない。

今回の召喚はenum順に並ぶ10種からuniformに1種を選び、既存と同じtypeへの置換も許す実験規約である。201はsundryの非attack、239はMP15のmiracleで、通常の消費・used保持・FREE・反応規約を使う。守護神identityをcardのguardian metadataから固定して召喚する処理ではない。種類分布、同type許容、抽選/置換のstageは現行serverで未確認である。

通常ownerのmain actionが全て終わり、既存のowner病気tick/死亡・自然終局/decision cutoffの優先規約を処理した後、自動守護神のstageへ進む。aliveな敵guardian sourceを入力seat順に扱い、それぞれ25%のbounded抽選をする。成功した場合、最初の8種は公開の6/5/4/3/2でcanonical行動を選び、Earthは285、Moonは286を固定候補にする。候補一つでもmoveの抽選を1回消費する。全10typeを残すが、これは**canonical42に限る独自selector**であり、Earth/Moonの一般神器/奇跡選出を実装したという意味ではない。

ATK>0、純curse、わいろ281、没収282はalive敵からtargetをuniformに選び、他の単体効果はselfを含むalive味方から選ぶ。280はtarget抽選無しで全aliveへCP+1。これらの対象集合・分布・抽選順はローカル規約である。単体対象の抽選は候補一人でも1回消費する。選ばれたpayloadのhit/response・反射・反撃/死後FIFOを解決してから次sourceへ進み、main ownerが死亡したら次の自動sourceへは進まない。全てが終わってからalive分のgift・次owner/GFへ進む。未完了途中にgiftを引かない。

25%・move・target・命中など内部抽選は新しいdecisionを増やさず、応答commandは従来どおりdecisionとして数える。別入口の外部 `guardian_step` は従来の1decisionの注入で、自動stageを連続起動しない。自動種類・行動選択を方策から自由に指定できるplayer commandへ変更する更新でもない。

### 退去とHP算術

直接bodyで、sourceへの自傷stageまたはtargetへのdamage stageの実HP減少が正だったとき、その時点でguardianを持つ本人へ10%を1回抽選する。非attackの負の202によるHP−10も、この直接減少に含める。致死被害も復活前の実HP減少で判定する。退去後も既に起動したpayloadのmodel/guardian/powerを変更しない。病気tick、自己病気防具のprefix/Heaven転落、CP/MP不足のHP債務、両替の直接配分だけのHP減少は今回の退去triggerに含めない。raw残余・通常HP損失・死亡/復活のどこを基準にするか、同bodyの各stageで抽選することや除外条件はserver未確認の実験規約である。

自動payloadも275の解除、回復、HP吸収、curse、285の補充、264/286の武器category等をv31と同じbodyへ接続する。sourceの手札・MPを使わず、操作giftを作らない。応答品と復活/285の補充は通常のdeferred ledgerに従う。239の通常cast自体のMPと保持は別であり、guardian行動を使って239を無料取得する処理ではない。

### 公開履歴とreplay

公開 `last_guardian_changes` は `{player,before:null|guardianName,after:null|guardianName,reason:"summon"|"departure"}` の配列である。private `GuardianChange.hidden_viewers` のseat maskにevent発生時のFog可視性を保存し、各viewer向けにentryをfilterする。後からFog解除/付与・死亡/終局が起きたという理由で、当時見えなかった変化の件数やidentityを過去へ遡って公開しない。persistent状態欄は現在の既存Fog可視性に従う。`last_auto_guardians` は既に選ばれた行動だけを公開し、失敗した25%判定・未起動source一覧・未来move/target・cursor・RNG・内部累計を出さない。行動表示として分かったguardianと、状態欄/未来抽選情報を区別する。

病気tick後にguardianがHPやcurseを変えるため、`last_auto_guardians` が非空のときだけ、private last_tickと現在HP/病気の一致条件を緩めてhistorical recordとして扱う。tick自身の静的な数式と、全main/action journal/auto stageを含むfull replayは維持し、当時のtick結果・退去・召喚・抽選・反応・giftを検査する。この再生は明記したローカル規約の整合を確認するもので、任意の完成raw sceneが公式serverで到達可能だと証明するものではない。

### v32の規約と検証

新native/C/Python・通常対局/外部行動回帰の結果は確定後に追記する。v31以前の外部42専用試行や「通常対局不変」の記録は当時の結果として保存し、今回の自動stageを検証した値へ読み替えない。Earth/Moon一般神器/奇跡selector、超常現象207、終末GF、公式Dreamの全候補分布・実体不適合入力は残件である。


公開exact規約はheaderと同じ以下の3本とする。

`guardian_rule`:

```text
experimental:canonical42-only;auto-after-ordinary-owner-tick-before-gifts;alive-enemy-sources-in-input-order;one-quarter-opportunity-draw-per-present-source;weighted6-5-4-3-2-for-first8;earth285-moon286-fixed-singleton-move-draw;uniform-alive-enemy-for-atk-curse-bribe-takecp-otherwise-alive-allies-including-self;singleton-target-draw;280-all-alive-no-target-draw;single-target-percent-and-darkcloud;no-fog-reselection;counter-death-fifo-before-next-source;stop-on-main-owner-death;no-internal-decisions-or-source-fee-gift;external-main-boundary-one-decision-no-auto-followup;earth-moon-general-selection-unsupported
```

`guardian_summon_rule`:

```text
experimental:201-and239-current-target;uniform-all10-in-enum-order;one-draw-including-resummon-same-type;normal-consumption-mp-retention-and-gift
```

`guardian_departure_rule`:

```text
experimental:present-guardian-on-positive-direct-body-hp-loss;source-self-damage-before-target-loss;one10-percent-draw-per-stage;lethal-included-before-revival;include-negative-random202;exclude-disease-prefix-debt-tick-and-exchange;launched-payload-unchanged;event-time-fog-viewers-filter-changes
```

[新版Dream再現script](../scripts/official_dream_v32_oracle.cjs) / [互換報告](../data/official-v2/dream-compatibility-v32.json) は236 IDs・120クラス・146 eligible・90 singleton・992順序付きpairを記録する。開始/既定target1,984件・MP14,880件・main追加702,336件・response追加1,111,040件、計1,830,240比較である。201/239はsingletonとして追加され、旧v29のscript/reportも保存する。これは検査したclient predicateの互換性で、召喚/行動selectorや公式Dream全分布・威力/価格の同値を証明する結果ではない。

native120群を全てmainへ各1回登録し、Release/SAN単体が通過した。Dream v32 oracleの `--check` は保存reportとbyte一致した。通常対局と外部守護神専用試行の大規模各2人数、計4組で1worker/4workersの全非時間fieldが一致した。

| 通常対局・seed42 | 試合数 | decisions / responses | checksum |
| --- | ---: | ---: | --- |
| 2人 | 100,000 | 3,989,071 / 1,356,844 | `13564747736715185154` |
| 9人 | 10,000 | 2,361,978 / 1,062,264 | `17158162465123773162` |

| 通常対局 | summons | opportunities / auto actions | departure rolls / departures | auto records / change records |
| --- | ---: | ---: | ---: | ---: |
| 2人100,000試合 | 14,773 | 98,777 / 24,807 | 32,214 / 3,208 | 24,807 / 17,981 |
| 9人10,000試合 | 10,781 | 314,754 / 78,393 | 34,410 / 3,409 | 78,393 / 14,190 |

通常poolと自動stageが変わったため、v31のchecksum不変は要求せず、v31以前の数値を歴史として保存する。Release/SANの通常各1,000試合も全非時間fieldが一致し、2人/9人のchecksumは `1704475476168731728` / `13129530548541068452`。

| 外部守護神・seed42 | 試行数 | decisions / responses | coverage | departure rolls / departures | checksum |
| --- | ---: | ---: | ---: | ---: | --- |
| 2人 | 100,000 | 227,967 / 127,967 | 42 | 13,188 / 1,345 | `15242932842311417291` |
| 9人 | 10,000 | 22,826 / 12,826 | 42 | 1,338 / 140 | `678603818888237652` |

外部専用の2人/9人はterminal2,967/0、cutoff0/0、rolls16,667/1,668・hits10,052/1,021・forced14,286/1,430・redraw24,125/2,412。専用420試行smokeもRelease/SANで全非時間fieldが一致し、checksumは `12692595741725906978`。専用試行を公式の種類/行動selector評価と混同しない。

benchの202 body HP・198 body MP・187 counter MPは、捕捉したbody量と開始時resourceからclip/minで求め、同じstep内の自動Neptune回復を混ぜないよう集計を修正した。これは測定の区分修正であり、ゲーム状態遷移の変更ではない。集計値は特権bench用で、失敗opportunity・未来行動やprivate cursorをplayer向けJSONへ公開しない。速度改善比は主張しない。

v32の最終全suiteはRelease **131/131件**（135.99秒）、Sanitizer **91/91件**（199.86秒）が通過した。native120群は全てmainへ各1回登録し、Python82件（Release）・CAPI/Python focused2件・固定CAPI照会384回も通過した。C APIの全42外部行動と新しいauto/departure境界を維持し、READMEのPython最小例はそのまま実行して `main 0 0 25` を確認した。これらは明記したローカル規約と既存機能の回帰検証であり、公式serverの種類/行動/対象選択・処理順・全分布を再現した証明ではない。
