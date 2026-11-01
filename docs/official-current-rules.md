# 現行公式公開クライアントの一次調査

この文書は 2026-11-01（JST）に通常のHTTP GETで取得した公式公開ファイルの調査記録である。ログイン、匿名認証、部屋への参加、対戦、Cloud Functionsへのコマンド送信は行っていない。旧Flashエミュレーターの仕様を現行公式の根拠として扱わない。

現行公式は[公式サイト](https://godfield.net/)に配信される Dart コンパイル済みJavaScriptとローカライズJSONから、神器台帳、教典、画面の選択制約、表示処理を検証できる。ただし、これだけで全ゲーム状態遷移や乱数処理まで取得できたという根拠はない。以下の「未確定」は仕様の推測で埋めない。

## 取得物と固定方法

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

これを根拠として、限定実行器v2は物理反射38/39/156/167/174/190と消去189を加えた139モデル/重み319に限定し、v3でblockMiracle8種とblockWeapon233を加えた148モデル/重み328へ進めた。v4はさらにFREE・setter・倍化8種を加えた156モデル/重み337。v5は跳ね返し8種を加えた164モデル/重み351。v6は単純確率攻撃21種を加えた185モデル/重み372。v7は攻撃反撃181/184を加えた187モデル/重み374。v8は資源反撃187/188を加えた189モデル/重み376。v9はHP吸収/同被害5種を加えた194モデル/重み381。v10は閃光付与/反撃/解除7種を加えた201モデル/重み404。v11は暗雲付与/反撃3種を加えた204モデル/重み407。v12は除去2種を加えた206モデル/重み409。v13はMP武器58と二回攻撃14を加えた208モデル/重み411。v14は全敵化232を加えた209モデル/重み412。v15はHP増減202を加えた210モデル/重み414。現行の[限定実行器v16](official-execution-subset.md)は自動復活208を加えた211モデル/重み415。閃光/暗雲以外の災いcounterや233/234以外の反応奇跡等を含めず、反応source/target・元payload・各応答者の消費と抽選来歴を分離する。サーバー同等性を確認した全面反応実装ではない。

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

最終bodyはatomicで除去途中snapshot無し。pending replayは除去前の消費/反射/RNG/payload上限を検査し、完了後任意rawstateの公式到達履歴を証明しない。決定的replayではbody抽選と後続giftの一致を検証する。公開remove event表示に対し、Match.observeの履歴投影は未対応として残し、他者unused削除model/slot/RNGを観測へ追加しない。本人手札と公開used_modelsの現状態だけを反映する。

## 病気・閃光・暗雲

公式 `texts.reference.diseasesNote` は、病気の自然悪化を毎ターン5%と説明する。`diseaseUpgradeNote` は病気を重ねると悪化すると説明する。旧版の初期0%から増加する `worseChance` を公式実装と同一視できない。**C02の公開仕様上の差は確定**したが、5%抽選のタイミング、重複時の比較規則、乱数端点、複数人時のtick対象は公開文言だけでは未確定。

`texts.curseDescriptions` の継続量は風邪1、熱病2、地獄病5の被害、天国病5の回復。天国病の悪化はHP0と説明される。閃光は防御時の使用神器が1つに制限される。クライアント `ev`（9478–9481行）は閃光がある応答で2枚目の選択を拒否する。これは旧bot由来の制限しかなかった状態より強い現行一次根拠である。

暗雲はja.jsonでは「全体攻撃」、en.jsonでは `%ATK` の受け手が必中になる説明である。文言だけから追加魔法WIDEに限定してはならない。JSの `hit` イベント処理（10934–10948行）は対象の暗雲を参照し、命中演出を分ける。命中を決めるサーバー抽選や、必中時に乱数を消費するかはこの表示処理から確定しない。

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

v16は全Release CTest129件/17.77秒・ASan/UBSan89件/47.15秒が通過した（各suiteを `-j4` で同時実行）。全source/test最終freeze後のfull build/全検証で、release/sanitize/bench/SAN bench/native/bridgeの6ログはwarning/error0。focused Release3件/0.62秒（native0.08/C API0.01/Python0.53秒）・SAN2件/1.21秒（native0.90/C API0.31秒）が通過した。Python MatchはAST確認31ケース、native零引数46関数は全てmainへ明示登録を確認した。公式assetsのfresh取得hash/296モデルfactsも一致した。 独立速度と大規模集計は[限定実行器v16記録](official-execution-subset.md#v16の検証と集計)へ分ける。

## 霧：何が見えなくなるか

公式教典は、霧の使用者が敵へ神器を使うとき、敵の中から使用先が無作為に選ばれると説明する。敵という集合はチームに依存し、味方・自己まで同じ規則で無作為化するとは書かれていない。

現在のクライアントでは、以下を直接確認できる。

1. `tL`（4021–4067行）はプレイヤーのHP/MP/CP、災い、守護神、dead、items等を読み込む。霧表示があることと、通信データが全て伏せられていることは同義でない。
2. 状態再構成 `ft.fs`（12352–12357行）は、生存している自分が霧を持ち、終局していない場合、敵の表示を `em(true)` で覆う。同チームの表示はこのループでは覆わない。個人戦では自分以外が対象になる。
3. `em`（13257–13273行）はプレイヤー情報コンテナ `z` を取り外し、青い半透明の覆いへ置き換える。`iT`（13173–13211行）、`c5`（4154–4176行）、`ox`（4078–4098行）をたどると、このコンテナには名前・チーム記号・HP/MP/CPの領域が含まれる。災いと守護神も同コンテナへ追加される（`dA` 13245行、`sd1` 13295行以降）。座席の存在そのものを取り除く処理ではないが、名前を可視情報として残せる根拠にもならない。
4. 自分に霧が付く/解除されるイベントは `bC(true/false)`（11743–11759行、11359–11377行、11444–11451行）を通じて敵の覆いを切り替える。
5. 敵候補の `dR`（13149–13151行）は生存者から敵関係 `ce` で絞る。UIは敵が複数の場合に霧の音・演出を分ける（9562行、10910行）。この音の条件はサーバーの乱数生成そのものではない。

公式profileの観測では、内部に真値を保持していても、霧の本人へ敵HP/MP/CP・災い・守護神等をそのまま渡してはならない。どの過去イベントの記憶を保持できるかは別の知識状態として扱う必要がある。合法手マスクにも、覆われた敵情報を利用した識別結果を出さないこと。

未確定なのは、再選択の厳密な実行段階、選択前後の公開イベント、敵1人時の乱数消費、特殊・全体・守護神由来の例外、病気や守護神の演出による部分的情報開示である。既存旧版の `not forced` 条件を現行へそのまま採用する根拠はまだない。

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
