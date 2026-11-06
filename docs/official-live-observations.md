# 公式サイトでの観測（2026-11-04）

v52で[完了した公開遷移3件](../data/official-v2/live-transition-witnesses-v52.json)、v53で[Dark自己死亡1件と反射剣の経路観測1件](../data/official-v2/live-transition-witnesses-v53.json)を追加し、自己攻撃の実装差を修正した。詳細は末尾の追記を参照。以下のv51記録は、取得できなかった当時の範囲として保持する。

[公式サイト](https://godfield.net/)をFirefoxで開き、教典とコンピュータ相手の修行を確認した。[公開ファイルの再取得](../data/official-v2/live-review-v51.json)と[UI観測記録](../data/official-v2/live-ui-observations-v51.json)を別に保存した。公開GETの本文hashは、ブラウザが同じ本文を読み込んだことの直接証明ではない。

教典では、夢の授与時50%、終末後の悪魔25%、守護神の敵ターン後25%・被ダメージ時退去10%、病気の自然悪化5%を確認した。Earthの取引・武器・防具・雑貨、Moonの奇跡という説明も既存資料と同じだった。これらは説明された範囲・確率であり、サーバーの抽選実装や複合効果の順序までは確定しない。

| 修行で見えた局面 | 確認できたこと | 未確認 |
| --- | --- | --- |
| 2人・終末なし、CPU先手G.F.1 | 両者HP40/MP10/所持金20、本人手札9枚。CPUが「売る」＋「精霊の杖」（価格20）を本人へ提示し、空応答の「許す」が表示された | 空応答後の資源、配送、補充、次手番 |
| 別セッション、本人先手G.F.1 | 同じ初期資源・本人手札9枚。「打撃の鉄板」を選ぶと攻5とCPUへの矢印が表示された | 攻撃の受理、CPU防御、実被害、消費、補充、次G.F. |

応答・確定ボタンの操作を複数方式で試したが、この確認では完了した状態遷移を得られなかった。UI操作・セッション・通信のいずれが原因かは確定しておらず、公式がコマンドを拒否したとも判定しない。見えている自分の9枚から、隠れたCPU手札の枚数を推定しない。資源・枚数の2例も、全モードの普遍的な初期条件とは扱わない。

**今回取得した完了済み遷移goldenは0件。** 現行のC++ルール・乱数・ABI・Snapshotは変更していない。売却後のCP40/0など、ローカル実行器が予測する結果を観測結果へ埋め込まない。

次の対局確認では、通常攻撃と防御、208/112の死亡・終局順、病気tick/補充/守護神、178の終末中一新、94＋232の順を優先する。各例に「操作前の公開資源と関連カード」「確定した選択」「相手の公開応答」「解決後の資源・関連カード・手番」を揃え、乱数による未観測結果と決定的な効果計算を区別する。受信したdamageをそのままエンジンへ注入するだけの一致は、効果計算の適合テストに数えない。

現行の主要残件とDreamの条件付き偽装率は、[仕様差分表の冒頭](official-spec-gap.md)に整理した。画面投影の拡張とは別に、対局本体の実遷移照合を進める。

## v52：確定操作の解消と公開遷移3件

固定clientの `A.ut` は `j(height,left,top,width)` を使い、確定操作用の透明領域を高さ300・top55・幅310として作る。前回は300をy座標と取り違え、領域の下にある威力・「許す」の表示文字を押していた。ゲーム内では大きな攻撃欄の中央を押すことで確定できた。通信障害やサーバー拒否を裏付ける結果ではなかった。

| 公開された操作と結果 | C++が事前計算する比較部分 |
| --- | --- |
| G.F.1、CPUのスタースタッフ75（光3）へ空応答。本人HP40→37、両MP10/所持金20は維持、G.F.2本人手番 | 先頭カードと属性から攻3、DEF0、本人HP37と次ownerを予測 |
| G.F.4、本人HP37でパワーハルベルト32を自分へ使用。防御操作を挟まずHP28、G.F.5はCPUの次攻撃待ち。未選択の守7も所持 | 既存のC++は防御入力を待っていた。通常mainの明示自己武器をDEF0・1commandで解決する経路へ修正 |
| G.F.5、CPUの45＋124＋34で攻26、本人が150で守7。本人HP28→9、CPU HP40→59、MP/所持金は維持、G.F.6本人手番 | 台帳のATK12＋5＋9とDEF7から残余19、吸収による相手HP59と本人HP9を予測 |

[機械可読の記録](../data/official-v2/live-transition-witnesses-v52.json)には前後の公開資源・選択順・観測していない情報を保存した。途中のG.F.2→4はCPUの防御とMP回復操作を取り逃したため、テスト対象へ数えない。追加の4例目もウインドウが利用できなくなる前に確定できず、3件のままとする。

`tests/official_live_trace_test.cpp` はこの3例を既存Matchへ入力し、未転記の在庫と任意seedを変えた216通りで公開結果を比較する。手札差し替えは結果の非依存性を調べる合成局面で、実際の絵柄や隠れた在庫を復元するものではない。自己攻撃後の比較はCPUの次行動前の局面を使う。実画面はその後の物理攻撃26が未解決の時点で、そのlaunchはMP費用0のため、比較する資源は同じである。

full backend event列は取得していない。自己攻撃が内部で直ちに解決されるか、自動で空防御を供給されるかは区別できていない。吸収の中間heal/damage順、死亡・復活・補充・病気・守護神の順、特殊な自己攻撃や反射後の自己対象への一般化も、この3例から確定しない。

## v53：Dark自己死亡と反射剣

前回の検証用対局へ再接続できたため、終末なしG.F.6、本人HP9/MP10/CP20・CPU HP59/MP25/CP20から81「さよならの剣」を本人へ使用した。追加のdefend操作なしで本人HP0・CPU勝利となり、G.F.6を維持、81が消えて手札9→8、補充は観測しなかった。本人の9枚は名前を確認した8モデル（81を含む）とDEF5の未同定品1枚で、208/112が無かった。この例は死亡後にそれらが働く場合の順序までは確認しない。

次の終末なし修行はCPUの初回回復を取り逃し、G.F.2で終了した。さらに修行2人・終末G.F.1で開始し、本人HP40/MP10/CP20・CPU HP50/MP10/CP20のG.F.2で38「反射剣」を本人へ使用した。追加の本人操作なしでG.F.4本人mainへ進み、本人HP30・CPU HP55、両MP10/CP20、38消失を確認した。

後者では終末中の授与・悪魔とCPUの次行動を記録できていない。CPUの回復カードや通常grantを逆補完せず、HP30を38単独の資源遷移goldenとしない。確定したのは、38の主動作自己使用が追加の本人応答を必要とせず完了したという経路である。防御時の反射能力を持つ主動作38を、常に自己defend待ちにしていたC++を修正した。

[v53記録](../data/official-v2/live-transition-witnesses-v53.json)は、累計の完了遷移4件と経路のみ1件を分ける。nativeの4件288投影比較とは別に、38を合成sceneのbody後grant phaseへ停止させる72回の経路検査を行う。そのgrant停止点・HP30・RNGはローカルテストの期待値であり、実UIの未取得の中間状態ではない。その他の特殊自己攻撃、一般の悪魔置換、一新、死亡・補充順の全同等性は引き続き未確認である。


## v54：大木の自己使用・空防御・使用済み奇跡の再使用

終末なし・2人CPU修行の別セッションで、連続した画面とアクセシビリティ表示から次の3件を記録した。両者の数値はHP/MP/CPであり、CPUの非公開手札は再構成しない。

| 操作 | 操作前 | 確認した完了結果 |
| --- | --- | --- |
| G.F.1、本人が213＜大木＞（木6、MP3）を自己使用 | 本人40/10/20、CPU40/10/20、本人手札9・未使用213 | 追加defend無し、本人34/7/20、CPU維持。G.F.2 CPU鎖ガマの未解決攻撃を確認。手札10、213は使用済みとして保持 |
| G.F.2、CPU19鎖ガマ（無5）へ空防御 | 本人34/7/20、CPU40/10/20、手札10・使用済み213が1個 | 本人29/7/20、CPU維持、G.F.3本人main、手札10・使用済み213維持 |
| G.F.5、使用済み213を自己使用 | 本人23/4/20、CPU30/30/10、手札11 | 追加defend無し、本人17/1/20、CPU維持、G.F.6 CPUmainを次の攻撃前に捕捉。新しい攻4の神器が加わって手札12、同じ使用済み213を保持 |

大木の両使用ではMP減少、6ダメージ表示、HP更新の順に画面へ現れた。これは表示順の観測であり、未取得のserver内部event順の証明ではない。初回後の新しい75%闇8と、後の攻4の補充モデルは同定しておらず、照合しない。起こした奇跡の表示は初回後に本人1・CPU0だったが、G.F.4以降のCPU使用済み数は未確認である。

G.F.3にも213を自己使用し、MP7→4とHP29→23までは記録した。しかしCPUのG.F.4を取り逃し、後からG.F.5のCPU30/30/10・本人23/4/20・手札11を観測した。この区間は不完全な観測として分離し、CPUの両替等を推定したり、補充の由来を逆補完したりしない。

[v54機械可読記録](../data/official-v2/live-transition-witnesses-v54.json)の3完了例を加え、累計7完了例をnativeの504通りで比較する。反射剣のrouting-only 72検査は別区分。手札差し替え・seed変更は公開投影の非依存性を調べる合成検査で、公式の完全な対局再生ではない。full server event traceは引き続き0件。


## v56：接続復旧後の自己売却・石攻撃・精霊の杖

CLI再起動後、Firefoxのnative接続が復旧した。一時的なpipe切断とcgWindowNotFoundも出たが、利用者に検証ウインドウを前面へ戻してもらい、その後は選択・対象指定・確定・連続capture・修行の終了まで操作できた。接続系エラーを公式のコマンド拒否と判定しない。

終末なし・2人CPU修行の新セッションを本人先手で開始し、以下を連続取得した。数値はHP/MP/CP。本人の初期9枚は取引3・武器3・防具3で、売る4が2コピー、26が1コピー、使用済み奇跡なし。CPU手札と枚数は非公開で、守護神／呪いの表示はなかった。

| 操作 | 操作前 | 完了した公開結果 |
| --- | --- | --- |
| G.F.1、売る4＋なぎなたクラシック26（価格3）を本人へ | 本人・CPUとも40/10/20、手札9、4が2コピー・26が1コピー | 追加本人応答なし、両資源維持、G.F.2 CPUmainを次launch前に捕捉。手札9、4が1・26が1。新gift46を後のtooltipで同定 |
| G.F.2、CPUつるぎ焼き69（石2、価格2）へ空防御 | 本人・CPUとも40/10/20、手札9 | 本人38/10/20、CPU維持、G.F.3本人main、手札9維持 |
| G.F.3、精霊の杖46（無12、cutCost、価格20）を本人へ | 本人38/10/20、CPU40/10/20、手札9 | 追加本人応答なし、12ダメージ表示後に本人26/10/20、CPU維持。46が消え新しい攻13が加わり手札9、G.F.4 CPUmainを次launch前に捕捉 |

[機械可読記録](../data/official-v2/live-transition-witnesses-v56.json)は新3完了例を加え、累計10完了例・独立routing-only1例・full wire trace0件とする。商品の26が再表示されたことは確認したが、server tokenの同一性・再採番は取得していない。売却の中間CP計算順は確定せず、新gift46や未同定の攻13をローカル乱数へ合わせない。nativeの既知copy検査は、合成の補充で4/26/46が再び出てもその新gift寄与を除いて行う。実画面の総model数と合成posthandの総model数を混同しない。


46の単品自己使用に対応してcutCost武器の直接自己経路を追加する。完成列のMP費用・保持・FREEの既存計算は変えず、他のcutCost品や一般の反射後frameへ主動作の省略応答を広げない。観測後は次のCPU攻撃を解決せず検証用修行を終了し、メニューへの帰還を確認した。


## v57：終末GF1の取引・夢・悪魔と再授与

公式サイトの2人CPU修行を終末G.F.1で開始し、次の7操作を前後の公開状態まで取得した。[機械可読記録](../data/official-v2/live-transition-witnesses-v57.json)の記録日は2026年11月5日。個々のcapture timestampは保存していないため、観測日は11月4〜5日の範囲とし、正確な日付を割り当てない。HP/MP/CPはその順で記す。

| G.F. | 観測した操作 | 次main到達時の公開結果 |
| --- | --- | --- |
| 1→2 | CPUが売る4＋精霊の杖46（価格20）を本人へ、本人は空応答 | CPU40/10/40、本人40/10/0、本人手札9→10、46を配送 |
| 2→3 | 本人のとげベルト17（無4）へCPUが空応答 | CPU40→36HP、本人40/10/0・手札10維持 |
| 3→4 | CPUの夢222（MP6）へ本人が空応答 | CPU36/4/40、本人40/10/0にDream、使用済み222を1枚確認 |
| 4→5 | 本人の女神の石けん206へCPUが空応答 | 使用済み222の除去表示、本人Dreamは残る。資源と本人手札10を維持 |
| 6→7 | 本人の46＋見かけ231（オーラ、MP6・無24）へCPUが170＋113（DEF12） | CPU41→29HP、本人MP10→4。その後本人244HP＋10、本人243が135と使用済み231表示を除去、CPU244HP＋10。最終CPU39/4/40、本人50/4/0Dream・手札9 |
| 7→8 | CPUの買う5へ本人が空応答、商品として買う5・価格5を表示、CPUが購入 | CPUのCP40→35、本人0→5。続いてCPU241でHP39→19、CPU243が旧石器ジャベリンと231表示を除去。最終本人50/4/5Dream・手札8 |
| 8→9 | 武器の見えていない本人が祈る | 本人241の20被害でHP50→30、その後新神器1枚。本人30/4/5Dream・手札8→9、CPU19/4/35維持 |

G.F.2で武器17と46を所持したまま祈ると「武器を所持していると祈れない」と表示され、G.F.・資源・枚数は変わらなかった。これは合法性の観測で、完了遷移数には含めない。G.F.5→6のCPU行動は取得できず、CPU36→41HPの理由を推測しない。G.F.6の選択preview後に操作接続が途切れたが、復旧後の確定からG.F.7までは連続取得できたため、今回の完了例として別記する。

206はDream付与前に受け取った品を後でtooltip同定した。231はDream下の見かけと費用・保持挙動が一致するが、真偽を断定しない。購入前のpink取引カードの絵もBuy表示と同じであり、別abilityへ化けた証拠にはしない。初期のgold取引カード、新しいpink2枚、新しいpendantの真モデルは未同定のまま。購入時に表示された商品だけは名前と価格から5へ対応付ける。

祈り1回に続く241と通常神器の連続例は、悪魔の効果後にも元の授与が残る既存規約を支持する。243の除去に新しい補充枠を足す根拠としては扱わない。複合例の表示順は本人244→本人243→CPU244だったが、raw rosterやwireを取得していないため、backendの受取人順・乱数消費順と同一視しない。

累計は17完了例・独立routing-only1例・full wire trace0件。nativeには先の4例と最後の祈りを追加し、15例1080投影と別枠72routingを検査する。合成在庫・通常補充による局所投影と、外部指定241後の再授与を分け、CPUの隠れた授与が通常だったとは主張しない。46＋231と購入の複合2例は記録のみとし、完全なnative再生の件数に含めない。


## v58：一新・使用済み奇跡・悪魔表示の連続取得

[機械可読記録](../data/official-v2/live-transition-witnesses-v58.json)は、2026年11月5日の終末閾値G.F.1の2人CPU修行で得た1完了遷移を保存する。初期手札の夢見る帽子178を保持し、226＜雪崩＞と214＜岩＞の使用済みコピーを残した局面を用意した。途中の探索・準備操作を新しい比較ケースへ加算せず、取得していないCPU行動や旧対局の死亡原因も補完しない。

G.F.11本人mainは本人38/0/20・CPU5/4/20、本人手札10（未使用8・使用済み226と214の2）。守護封印のつぼ201をCPUへ使うとCPUは空応答し、CPUの新しいPlutoが同じG.F.11で271「まばたき」（闇ATK2）を本人へ使った。本人の手札は実物9と空placeholder1となり、178単品のWood DEF14を選択・確定した。

| 表示された経過 | 確認できたこと |
| --- | --- |
| 178応答後「無事」 | 闇2にDEF14を示し、本人HP38を維持 |
| 「夢」→「神器一新」 | 本人にDream表示、使用済み226・214を含む旧手札の表示を置換 |
| 置換・授与の移動アニメーション | 途中frame58では通常品7枚が明瞭で、8枚目は重なりのため識別不能。frame63/64では通常品9枚と一時的な244の姿1枚が並ぶ |
| 244「めぐみの妖精」のCP＋10、続いてMP＋10 | 本人CP20→30、MP0→10。HP38とCPU5/4/20を維持 |
| G.F.12 CPU main、次launch前 | 本人38/10/30・Dream、通常の手札10枚、使用済み印なし。全品の真モデル・fakeは未取得 |

「一新」表示中に悪魔名のpopupは出なかったが、これを置換8結果が全て通常品だった証拠にはしない。受信clientのreplaceItemsは供給された悪魔を排除せず、後続のuseDevilItemでpopupを出せる。並べ替え・非同期移動・重なりがあるため、画面位置をslotの固定識別子へ読み替えない。8通常品の置換＋後続補充の悪魔、7通常品＋悪魔の置換＋後続通常補充の両方が、今回の公開経過と整合し得る。二回の244が同じ枠のretryか別枠かも確定しない。

累計は18完了例・独立routing-only1例・full wire trace0件。今回の複合経過をnativeの次mainまでのgoldenへ足さず、従来15例1080投影と72routingを維持する。別枠72一新コンポーネント検証では、外部指定271に178で応答した場合の防御・Dream・旧使用済みコピーの消失を比較する。原発201の補充枠が無い合成sceneの8fresh通常品は現行仮説の中間状態であり、公式画面で8通常品を直接読み取った結果とは扱わない。

取得後はG.F.14の次のCPU攻撃に応答せず検証用修行を終了し、ホームメニューへの帰還を確認した。今回、一新の悪魔適用・複数帽子・途中の死亡復活・serverのslot/debt対応を確定したとはしない。

その後、別の短いCPU修行でFirefoxのNetwork Monitorを開き、Firestoreの部屋情報の応答をUIから閲覧できることを確認した。ゲームの全イベント列は取得できておらず、前の178経過のwireを遡って復元したとは扱わない。確認用の対局と開発者ツールを閉じ、ホームへの復帰を確認した。新しい完了ケースやfull wire traceの件数には加算しない。

## v59：連続した受信game文書と3完了操作

2026年11月5日の2人CPU修行・終末G.F.1で、Firefox Network Monitorの「別名で応答を保存」から受信本文だけを取得した。HAR・認証header・cookie・送信要求は保存せず、raw本文はpermission600の `/tmp` 作業物に限定する。[匿名化game fixture](../data/official-v2/wire-game-updates-v59.json)は名前・userId・room path・updatedAtを除去し、モデル・受信slot・省略field・events順を残した。viewer1/CPU2は受信game内の数値IDである。

| 受信updateCount | 確定した操作と境界 | 本人HP/MP/CP・状態 | 主な受信結果 |
| --- | --- | --- | --- |
| 1 | G.F.1、本人main・初期実手札9 | 40/10/20 | CPUも実手札9、保留攻撃なし |
| 2 | 23硬いつちを明示自己使用→CPU62攻撃開始 | 34/10/20 | 通常161受領。CPU62（土1）が本人をtarget、CPU実物8＋placeholder1 |
| 3 | CPU62へ空防御→G.F.3本人main | 33/10/20・Cold | CPU手札差分に184、giftイベントは省略。両者実物9 |
| 4 | 199スマイルの貝がらを明示自己使用→CPU32攻撃開始 | 33/10/20・Cold解除 | 通常123受領。CPU32（無9）が本人をtarget、CPU実物8＋placeholder1 |

3操作を完了観測へ追加し、累計21件となる。G.F.4のCPU32への応答は行わず退出し、ホーム復帰とNetwork Monitor閉鎖を確認した。今回のCold付与で被害1、199の解除後は本人HP33を維持したが、一般病気schedulerの全順序や乱数分布へ一般化しない。

23の受信id6は161へ、199のid1は123へ再利用される。これらは恒久的な物理個体IDではない。CPUの `{id:0,modelId:0}` は空slotであり、実物9枚と数えない。CPUの真モデルは修行の受信fixtureに含まれるが、検証専用で、方策のviewer観測や対人戦の秘匿性へ適用しない。

updateCount2には自己攻撃のuse/setTarget/damage/gift/advanceGFだけでなくCPUのuse/setTargetも入る。初期eventsにCPU初期giftは無く、updateCount3にもCPU184のgiftは無い。受信したeventsは全内部処理ではないため、CPU補充時の未観測悪魔列やretryを作らない。受信初期状態1＋後続batch3を取得した有限範囲であり、全対局・全backendイベント列は0件のままとする。

[専用native比較](../tests/official_wire_trace_test.cpp)は通常161・184・123を外部chanceで与え、8seedで4checkpoint＝32比較を行う。seedを観測結果へfitせず、slot対応を管理して実手札・資源・Cold・保留攻撃を照合する。従来15例1080投影、72routing、72一新部分検証は別枠を保つ。[機械可読観測記録](../data/official-v2/live-transition-witnesses-v59.json)に範囲を明示する。

## v60：複合攻撃への二枚防御から補充中の死亡へ

2026年11月5日の2人CPU修行・終末G.F.1で、[開始と終局の受信game](../data/official-v2/wire-game-updates-v60.json)を取得した。viewer2・CPU1で、updateCount1はstartGame・本人初期gift9・CPUの最初の攻撃開始を同じbatchに持ち、updateCount2は防御解決からendGameまでを含む。初回から終局までの連続した受信game列は1対局分となったが、CPUの全gift等の内部イベントを取得したものではない。

CPUの32パワーハルベルトATK9＋51独楽コンバットATK13へ、本人は118革の服DEF2（id3）と142はがねの小手DEF6（id5）を選び、応答ペインで確定した。受信useDefenseItemsと14被害は22−8に一致し、本人HP40→26となる。同じupdate内にCPU243のuseDevilItem、191/id4・206/id6のremoveSomething、本人68/id3のgift、241/id5のgift/use/20被害、242/id5のgift/use/30被害、本人dieとendGameが続く。途中画面で本人HP6と大悪魔30、最終画面で本人HP0・CPU勝利を確認した。G.F.は1のまま、MP10/CP20は双方維持した。

CPUの通常新2品135/id1・172/id4は最終手札の差分で同定したが、giftイベントは無く、取得順は未確定である。初期CPU実物7＋empty2から243で実物2を失い、最終実物7となる。native比較は観測した243に追加補償giftを足さず、2つの通常completion順を両方検査する。未観測のCPU内部retryや乱数列まで復元したという主張はしない。

終局の本人には実物8＋empty1が残る。死亡後のemptyはlive gift debtではなく、empty数から未処理授与やschedulerを推定しない。受信id5は241→242で再利用され、恒久的な物理tokenではない。CPU初回の消費32/id9・51/id1はuseイベントから分かるが、起動直前のCPU配列順や初期抽選は未取得である。

今回1例で累計22完了観測、独立routing-only1例となる。元の15例1080投影＋72routing＋72一新部分検証、v59の8再生32checkpointに、新しい2補充順×8seed・2checkpoint＝32比較を別に加える。公式の全内部処理traceは0のままで、完全な対局実装の証明ではない。[機械可読記録](../data/official-v2/live-transition-witnesses-v60.json)へ境界を固定する。

操作メモとして、下段の「守8」表示へクリックした段階ではsubmit-commandが発行されず、応答ペイン内 `[865,420]` から確定した時にPOST200と状態更新を確認した。下段表示を独立した確定ボタンとみなさない。通信要求を自作・再送せず、応答本文だけを保存した。退出後はホーム復帰とNetwork Monitor閉鎖を確認した。

## v61：Dream探索と比較件数の扱い

CPU修行を3対局探索したが、新たなDreamのmodelId/fakeModelId pairを確定していない。未記録の単発経過や初期手札を、新しい完了witnessやfull比較へ加算しない。探索終了後はホームへ戻り、Network Monitor閉鎖を確認した。公式完了観測22件・native対象19件、独立routing1、完結した受信game列1対局分・全backendイベントtrace0を維持する。

今回追加した `ArtifactGrantAction` の互換pair検証は合成fixtureによるローカル契約の比較である。公式の全Dream候補・50%分布・互換外実体の使用結果が確認できたという記録へ読み替えない。

## v62：終末なし天国草と通常補充

2026年11月5日の2人CPU修行・終末なしで、[連続受信game2状態](../data/official-v2/wire-game-updates-v62.json)を保存した。viewer1・CPU2、updateCount1はGF1本人main、双方HP40/MP10/CP20・実手札9である。198天国草（＋MP20、Heaven）を選び、本人statusを明示targetとして主動作ペインから確定した。

updateCount2の順は、本人198/id9使用→target1→MP＋20→Heaven付与→本人disease→HP＋5→本人191/id9受領→CPUへGF更新→CPU191/id8使用→HP＋5→本人へGF更新である。UIでもGF3本人45/30/20・Heaven、CPU45/10/20・本人手札9を確認した。本人の191はgiftイベント、CPUの新207/id8は受信在庫差分として区別する。同じid9は198→191へ再利用され、恒久的物理tokenとはしない。

新1例は本人の操作からCPU自己回復を経た次mainまでの複合完了例として数え、CPU回復を別の独立例へ重複加算しない。開始〜終局ではなくGF3で退出し、ホーム復帰・Network Monitor閉鎖を確認した。本文だけを保存し、名前/userId/updatedAt等を除去してgame数値・省略field・events順を保持する。

この非悪化Heavenの1例から5%率を測定したとはしない。新external_artifacts modeでは通常の結果を入力できるが、病気tickは既存game RNGのままである。native比較でも既定の固定seed集合を変えず、観測結果に合わせた検索・除外を行わない。[機械可読観測](../data/official-v2/live-transition-witnesses-v62.json)に比較範囲を記録する。

累計23完了観測・native対象20例となる。新wire比較は固定8seed×2checkpoint＝16比較で、従来の1080投影・72routing・72一新部分検証、v59の32checkpoint、v60の32checkpointと別集計である。全8seedは既存のローカルHeaven非悪化枝を通るが、公式の率や一般的な病気処理順を検証するものではない。開始〜終局の受信列1対局分・全backendイベントtrace0は維持する。


## v63：夢の木づちの自己使用と135/135補充

2026年11月5日の2人CPU修行・終末なしで、viewer1の80夢の木づち（Wood4）を本人へ使用した。初期資源はUIで双方40/10/20、curseなし、GF1。主動作ペインで1回確定すると、追加の自己防御入力なしに本人HP36・Dreamとなった。GF2ではCPU2が60つらら（Water1）を本人へ攻撃し、防御待ちに入った。

[受信post状態](../data/official-v2/wire-game-updates-v63.json)のupdateCount2は、use80/id1→target1→damage4→addCurse dream→本人gift `{id:1,modelId:135,fakeModelId:135}` →advanceGF CPU→CPU use60/id2→target1の順を持つ。同一番号fakeの明示存在を確認した初の例で、元からあった135/id4とは別copyである。使用slot1は補充で再利用されるため、固定の物理IDとはみなさない。

初期のNetwork応答はpendingで本文を取得できず、この対局の権威的初期snapshotは無い。別の探索対局の初期記録やgameを含まない162 byteの応答を混ぜない。native比較は受信post在庫に消費80/id1とCPU60/id2を戻した再構成sceneから行い、原初期draw・配列順を証明しない。受信後のブラウザ操作がwindow-not-foundとなり、CPU60への防御や新135の使用結果は未観測のままである。退出やホーム復帰も確認できていない。

1自己使用を新しい完了例として累計24例・native対象21例へ追加する。旧固定8seed×1受信checkpoint＝8比較を別枠で加え、1080投影・72routing・72一新部分検証・v59/v60各32checkpoint・v62の16checkpointを保つ。開始〜終局の受信game列1対局分・全backend内部trace0は維持する。同一番号の受領1例から全候補・50%率・異番号互換外の使用処理を一般化しない。


## v64：同番号偽装の使用と実体197のMP＋15

2026年11月5日、Firefoxの操作が復旧し、v63と同じ終末なしCPU修行を再開した。GF2のCPU60へ空防御して本人HP36→35、GF3本人mainとなった。この先行操作のgame3応答は保存できていないため、以下の4操作とは別に完了件数へ加算しない。

[保存したupdate4〜7](../data/official-v2/wire-game-updates-v64.json)は、本人の通常195自己使用から、同番号135防御、同番号195自己使用、異番号197/fake195自己使用までを保持する。

| 受信update | 本人の確定操作と結果 | CPUの後続・次の境界 |
| --- | --- | --- |
| 4 | 通常195/id3を自己使用、MP10→15。gift195/fake195/id3 | CPU23/id6の無属性ATK6、GF4本人の防御待ち |
| 5 | 135/fake135/id1を防御使用、DEF5・被害1でHP35→34。gift213/id1（fakeなし） | CPU188/id6は在庫差分。GF5本人main |
| 6 | 195/fake195/id3を自己使用、MP15→20。gift197/fake195/id3 | CPU3/id8の両替で40/10/20→30/30/10。CPU196/id8は在庫差分。GF7本人main |
| 7 | 見かけ195「スマイルの花・MP＋5」を自己使用。受信実体197・boostMP15でMP20→35。gift13/fake13/id3 | CPU195/id1を自己使用しMP30→35。CPU202/id1は在庫差分。GF9本人main |

useDefenseItemsには135/fake135、useAttackItemsには195/fake195と197/fake195がそれぞれ明示されている。same-modelでもfake fieldを消さず保持する前版の処理を実使用まで検証する。異番号197/195は共にzero-costのboostMP雑貨で既存9key互換内であり、ここでは回復量が見かけと実体で異なる。一般の互換外pairやコスト・category違い、全50%分布を証明したものではない。serverの独立したrevealイベントは受信列に無く、nativeのlast_revealsは使用時のローカル公開表現である。

初期通信は今回も復元しない。再現用GF3 sceneはupdate4の本人gift195/fake195を消費前の195/Noneへ戻し、CPU空枠を消費23/id6へ戻して構成する。本人35/10/20・DreamはUI、CPU64/id2は既に届いていた受信在庫を使う。原配列順・初期draw・game3は不明で、CPU通常補充の省略イベントやretry列を作らない。recipientごとのlocal補充順をbackend順へ読み替えない。

新4操作で累計28完了例・native対象25例とする。固定8seed×4受信checkpoint＝32比較を追加し、以前の投影・routing・一新部分検証・各世代checkpointは別集計を保つ。開始〜終局の受信game列は累計1対局分、全backend内部traceは0である。GF9から退出し、ホーム復帰とNetwork Monitor閉鎖を確認した。[機械可読の観測範囲](../data/official-v2/live-transition-witnesses-v64.json)へ固定する。


## v65：201召喚後の壁・Neptuneと207からの濃霧

2026年11月5日、終末なし2人CPU修行を8回開始し、2対局の完了viewer操作4件を収録した。その他6対局は初期手札の探索のみで、完了観測へ加算しない。Network Monitorでゲーム状態を含む大きなGET応答を選び、本文だけを保存した。gameを含まない165 byteの探索用応答は根拠へ使わない。両収録対局でupdate1の権威的初期状態も取得でき、以前の再構成初期sceneとは区別する。

[Neptune対局の3更新](../data/official-v2/wire-game-updates-v65-guardian.json)は、初期双方40/10/20・本人mainから201/id6自己使用→setGuardian neptune→本人196/id6補充→GF2 CPU29/id2＋231/id5のATK16を含む。CPUはMP10→4、231をused保持し、実物8＋empty2となった。

本人は233/id4「壁」（MP6、無属性武器を止める）で応答した。update3はuseDefenseItems233→block→attackByGuardian（source1/Neptune/model276）→HP＋5→本人gift123/id10→GF3の順で、本人HP40→45・MP10→4、233はusedで残った。CPU200/id2・19/id10は最終在庫差分で、giftイベントはない。Neptuneの回復が補充より先に届く有限例であり、召喚率・自動機会率・各技の率や一般scheduleを証明しない。

[Fate対局の3更新](../data/official-v2/wire-game-updates-v65-fate.json)は、初期本人の207/id7を使用し、phenomenon288→setCurseOfEverybody fog→本人gift192/id7→GF2 CPU76/id6の闇3という順である。本人は142/id6のDEF6で応答し、safe・HP40維持・本人166/id6補充・GF3を取得した。CPU26/id6は在庫差分で、双方Fogは維持する。UIでも本人の資源は見える一方、相手の資源が隠れた。

既存FateActionへ観測288と通常結果を指定し、固定8seed×3checkpoint＝24比較を行う。Fogのsingleton対象drawは実行したままで、selectorや乱数を合わせない。Neptuneの召喚/自動選出はfull再生せず、276のHP5だけを外部GuardianActionで照合する別枠8component検証へ分ける。post3在庫・used233/231を静的contextに使いHP5のみ戻した合成入力であり、公式の途中状態や補充順を再現したものではない。

新4完了観測で累計32例、うち全体native投影対象27例、複合5例は記録のみとなる。別枠routing1例・一新部分検証・各世代wire checkpointは維持し、新Neptune効果8検査を別集計する。開始〜終局の受信game列は累計1対局分・全backend内部trace0のままである。GF3で両収録対局を退出し、最後にホーム復帰とNetwork Monitor閉鎖を確認した。[機械可読範囲](../data/official-v2/live-transition-witnesses-v65.json)へ固定する。


## v66：リアルゴーストソードと二回攻撃の自己使用

2026年11月5日の終末なしCPU修行3対局を探索し、2対局の自己使用を収録した。残1対局は完了viewer操作がなく、観測数へ加算しない。

[吸収対局](../data/official-v2/wire-game-updates-v66-absorb.json)はGF1本人1/CPU2、双方40/10/20・guardian/curseなしから45/id6を明示selfで使用した。受信update2はuse45→target1→dealDamage12→boostHP12→gift161/id6→GF2 CPU→CPUのBUY5/id7→target1である。追加の自己防御入力はなく、本人HP40/MP10/CP20と手札9を維持した。CPU BUYへの応答はせず、offer・支払・YES/NOは未観測である。beforeの応答はFirefoxでstream中断扱いだったが、完全なgame1文書を保存・decodeできており、transport全体の完結とは区別する。

吸収の受信client表示は40→28→40、現native bodyは先healで40→52→40となる。終点が同じなので、今回の状態比較を内部順の適合証明へ読み替えない。HPがraw量より小さい場合・99上限・死亡/復活を伴う場合は未取得であり、既存の実験規約は変更していない。

[二回攻撃対局](../data/official-v2/wire-game-updates-v66-twice.json)の最初のgame1は既にGF2本人2 mainで、CPU1が197を自己使用した後の40/25/20、本人40/10/20である。14/id6を単品selfで確定すると、update2にdamage3→nextAttack14→target2→damage3→gift19/id6→CPUへGF更新→CPU191/id7自己回復→本人へGF更新が並んだ。本人HP34、CPU45/25/20、GF4本人mainとなった。消費とgiftは1回ずつで、二発の間の自己defend入力もない。CPU47/id7は在庫差分であり、giftイベントは受信していない。

native比較は45のGF1と14の受信GF2をそのまま開始sceneへ使い、既存固定8seedで各2checkpoint＝32比較とする。14の先行CPU197・元CPU初期draw/order、45の途中resource順や後の購入selectorは範囲外とする。新2操作で累計34完了例・native対象29例、複合record-only5例・独立routing1例、開始〜終局の受信列1対局分・全backend内部trace0を保つ。両対局を退出し、最後にホーム復帰・Network Monitor閉鎖を確認した。[機械可読の範囲](../data/official-v2/live-transition-witnesses-v66.json)へ固定する。


## v67：低HP吸収の探索と既存受信HPの検査

CPU修行で吸収27/45/216と両替3を利用できる局面を探したが、内部演算順を区別できる低HP・上限の完了例は今回取得していない。新しいresponse bodyや完了遷移goldenは追加せず、実測累計34例・native状態投影対象29例を維持する。最後に修行を退出し、公式ホーム復帰とNetwork Monitor終了を確認した。

既存v66の受信データだけを使い、独立HP reducerで45の40→28→40を検査する。14の二発40→37→34と後続CPU191の40→45は、呼出側の対象切替を明示して検査する。この表示比較は対局内部の吸収・死亡順を確定せず、新しい公式観測にも数えない。


## v68：低HP自己吸収の識別例と準備操作

同じCPU修行の[受信update1〜6](../data/official-v2/wire-game-updates-v68.json)を取得し、[5操作のwitness](../data/official-v2/live-transition-witnesses-v68.json)を保存した。各gameを持つ受信documentの同一性はprivate sourceで確認し、原名・userId・updatedAt・transport識別情報を除去した。初期状態から連続しているが終局前に退出したため、開始〜終局の対局1件を追加するものではない。

| 操作 | 受信した結果 |
| --- | --- |
| GF1で39月光のオノ＋162鬼のよろいを自己使用 | 被害25、本人HP40→15。113/slot3、166/slot5を補充し、GF2でCPU19の攻5開始 |
| CPU19に空防御 | 本人HP15→10、GF3本人。CPU115/slot8は受信在庫差分で確認 |
| GF3で19鎖ガマを自己使用 | 被害5、本人HP10→5。163/slot1補充、GF4でCPU67の木2開始 |
| CPU67に空防御 | 本人HP5→3、GF5本人。CPU26/slot2は受信在庫差分で確認 |
| GF5で27ゴーストソードを自己使用 | 被害7→本人HP回復7、最終HP7で生存。38/slot7補充後、CPU197自己使用でMP10→25、GF7本人 |

最後の使用直前は本人3/10/20・CPU40/10/20、guardian/curse/復活品なしである。受信damage7とboostHP7をそのまま保持し、実HP損失3へ回復量を縮めない。新しいHP表示検査は3→0→7、Matchの状態比較は1入力後のHP7生存を確認する。従来の先回復では3→10→3となり、終点でも区別できるため、v66のHP40だけの終点一致とは証拠が異なる。中間HP0で直ちに終局する規約もこの例と合わない。

自己吸収aliasの演算を被害→全nominal回復へ修正し、従来の全body後の死亡確定を維持した。27を直接観測し、45/216・redirect/payload等の同じ能力への適用は限定一般化とする。99上限・208/112・guardianを併用した新しい公式例、非self吸収や同被害53の内部順は今回確認していない。

新規5操作で累計39完了例、native状態投影対象34例、複合record-only5例。wireは固定8seed×6状態＝48比較を加えて総224checkpointであり、8つの公式対局を得たわけではない。CPUの省略gift順、server乱数、diseasePlayerId/guardianPlayerIds等の全schedulerを再現したとも扱わない。最後はホーム復帰とNetwork Monitor終了を確認した。

準備の39＋162にも実装差があった。旧direct-self allowlistは38の `reflect_weapon` のみを含み、39の `reflect_miracle` は自己防御を待っていた。今回の1commandで25被害・2補充という受信結果に基づき、武器baseの `reflect_miracle` も直接経路へ追加した。該当武器は39であり、156/167/174等の防具や190の一般経路を追加したものではない。5操作の先頭をこの新しい回帰として含め、件数を別途加算しない。


## v69：非終末一新・能力の異なる偽装・低CP購入

[同一CPU修行の受信7更新](../data/official-v2/wire-game-updates-v69.json)と[6操作](../data/official-v2/live-transition-witnesses-v69.json)を保存した。最初のupdate1はCPU35の攻10への本人応答待ちで、元のCPU初期draw/orderは取得していない。終末は無し、本人はplayer2である。最後のCPU BUYには応答せず、ホーム復帰・Network Monitor終了まで確認した。

| 操作 | 受信結果と境界 |
| --- | --- |
| CPU35に178単品DEF14 | safe→本人Dream→redraw→replaceItems8→gift203/slot9→GF2。本人40/10/20・9枚。全8置換と1補充を区別できたが、終末・使用済み奇跡・複数帽子は未測定 |
| 実19/face27を自己使用 | プレビューはゴーストソード攻7・吸収。useAttackItemsは19/fake27、dealDamage5のみでHP40→35。回復なし。21/fake21補充後、CPU47の攻13開始 |
| 実113/face135でCPU47へ防御 | プレビューDEF5、実DEF1でdamage12、本人HP35→23。195/fake196を補充、CPU105は在庫差分 |
| 両替3 | 23/10/20→23/30/0。exchangeイベントにはhp23/mp30がありcpは省略。213をunmasked受領、CPU35の攻10開始 |
| CPU35に空防御 | 本人HP23→13、MP30/CP0維持。CPU142は在庫差分、GF6本人 |
| CP0・MP30で他者BUY | CPU空応答後、価格5のモデル5をbuy→canNotBuyで返却。HP13/MP30/CP0と相手40/10/20を維持、237補充後、次CPU BUY開始。YES/NO入力・購入支払はない |

replaceItems8には19/fake27、202/fake202、29/fake23、113/fake135、196/fake195を含む。19→27だけが旧9field互換外で、ability none→absorbHPが異なる。202の同番号偽装はsingletonでの受信例でもある。これを8回中のmask率から一般分布を推定する根拠にはしない。生成候補・コイン・draw数は変更せず、受信predicateへ方向限定19→27を足し、既存のclosed-state・明示ArtifactGrant・reveal/private proofへ通す。真値を見た追加の先行合法性gateは導入しない。

一新自体はrecord-onlyである。ネイティブ比較は受信した一新後update2を初期sceneとしてupdate2〜6の5状態だけを既存8seed＝40比較へ展開する。178の8置換抽選を再現したとは数えない。低CP BUYも、複数候補からモデル5を選ぶserver抽選をseed選別で合わせずrecord-onlyへ分ける。CanNotBuyの価格5という条件は既存CP-only affordabilityを支持する有限例であり、全商品・self BUY・一般CP debt順の証明ではない。

新6操作で累計45完了例、native投影対象38例、record-only7例。wire比較は264checkpointとなる。開始〜終局の受信列は従来1対局、全backend内部trace0のまま。固定8seedを8つの公式対局へ読み替えず、CPUの省略gift順・全schedulerを推定しない。


## v70：既存一新記録の条件付き再生へ拡張

新しい公式操作・response bodyは取得せず、v69の記録を再利用する。CPU35/slot2の消費コピーだけをuseイベントから戻したsceneをGF1で作り、CPU35をlaunchして最初の受信応答待ちへ合わせる。元のCPU初期draw/orderは復元していない。

外部結果付き178応答へreplaceItemsの8モデル/fakeを渡し、通常補充は別にCPU5/slot2の在庫差分と本人203/slot9を供給する。これにより一新前update1→一新後update2→既存4操作のupdate6まで6状態を、既存8seedで48比較する。v69の一新後sceneからの40比較も残し、同じ受信状態を別の経路から再検査する範囲として数える。

完了観測は45のまま、178の既存1例を外部結果条件付きのnative対象へ移す。native対象39・record-only6、wire比較総312となる。これは新しい公式対局や新規goldenの追加ではない。低CP BUYの商品選出は引き続き範囲外、完了受信start→endは従来1対局のままである。旧v69の当時のcoverage記録とraw出典は変更せず、新しい検証reportへ分ける。


## v71：既存CP0購入拒否を外部商品指定で照合（新観測なし）

[再生report](../data/official-v2/supplied-purchase-replay-v71.json)は既存v69のupdate1〜7を対象とする。178の8枚を供給するv70経路から続け、BUY応答時のCPU商品5/slot2を明示する。CP0では確認画面へ進まず同じコピーが返り、本人237の通常gift後、CPUが返却品を使った次BUY保留まで比較する。CP/HP/MP、Dream、全正コピーとfake presence、placeholder、GF/owner/response、各private snapshotの復元を検査する。

7状態×固定8seed＝56比較を加え、重複を含むwire累計368比較とする。新規観測0・完了観測45例を維持し、既存BUY拒否の1例を条件付きnative対象へ移して40/record-only5。元v69のwire/witnessと当時のcoverageは変更しない。公式の商品selector分布・乱数器、次CPU BUYへの応答/支払いは再現対象外である。


## v72：六角凶の自己使用・闇防御・自己49・終局

[4受信更新](../data/official-v2/wire-game-updates-v72.json)と[4操作](../data/official-v2/live-transition-witnesses-v72.json)を取得した。41自己使用は単一操作で11被害・DarkCloud・通常49補充、179/DEF15でCPU92の闇10をsafe、49自己使用で13被害、最後はCPU19+43+25の22に空防御して死亡・終局となる。本人HP列は40(UI初期)→29→29→16→0、MP10/CP20維持である。41の不要なself-defense待ちを修正する。

初期の権威的本文は取得できず、最初のpostから消費41とCPU92を戻した再構成sceneからの検証とする。4状態×固定8seed＝32比較、累計観測49/native対象44/record-only5、wire比較は重複込み400。終局後も未使用45を保持しており、45低HP吸収の観測には数えない。新しい完全な受信start→endは0、従来累計1のままである。終局後のホーム復帰・Network Monitor終了を確認した。


## v73：エンゼルアクスと反射された土星の指輪

[4受信更新](../data/official-v2/wire-game-updates-v73.json)と[3完了操作](../data/official-v2/live-transition-witnesses-v73.json)を保存した。authoritativeな初期手札には55があり、自己使用は15被害・184補充へ直接進んだ。続くCPU40/11へ184で応答すると本人25→14、22のStone反撃がCPUへ向かい、CPU190で反射された。165は適合せず未選択で、最後の空防御は22被害→die1→endGameとなった。

初期から終局までの4状態を固定8seedで32比較し、52完了観測中native対象47・record-only5、wire累計432へ更新する。完全な受信start→endは新1/累計2、全backend内部traceは0のままである。初期drawは生成せず受信手札を入力し、CPU207も所持のまま未使用として保つ。初期とpostの取り違えを避けて重複body3件を除外した。terminal画面は途中で終了ボタンへ進んだため撮影せず、死亡/終局の根拠はauthoritativeな本文である。ホーム復帰・Network Monitor終了はUIで確認した。


## v74：21のUI自己使用と23の購入成立

[受信update1と3](../data/official-v2/wire-game-updates-v74.json)と[2操作](../data/official-v2/live-transition-witnesses-v74.json)を保存する。21をself単品で使うUI操作では本人40→35・MP10/CP20維持、追加本人defend無しでCPU BUYへ進んだ。raw更新2/use21/gift44/CPU5 launchは取得できていない。

そのBUYへ空応答すると、受信3はseller23/id6の選出とbuyer23/id9へのsetBought trueを示し、本人CP20→22・CPU20→18、HP/MP維持となった。本人44/id3とCPU13/id10はイベント取得ではなくinventorydeltaとして扱う。buyer10枚目の13とseller8枚を保ち、初期→購入後の2endpointを固定8seedで16比較する。途中quoteを受信stateに加えない。

新completed UI観測2・累計54/native49/record-only5・wire448。欠番2を作らず、新しい完全受信lifecycleは0、従来累計2のまま。重複02/03と別prep-room05を除外し、途中退出は完了操作に数えない。ホーム復帰とNetwork Monitor終了を確認した。


## v75：マジカルステッキ58の単品自己使用

[2受信状態](../data/official-v2/wire-game-updates-v75.json)と[1操作](../data/official-v2/live-transition-witnesses-v75.json)を保存した。本人HP40/MP10/CP20から58を単品自己使用し、追加防御なしでHP20/MP0/CP20・通常23補充となる。CPU221起動でMP7、221はused保持＋空行1となり、Fogはまだ付いていない。応答せず途中退出したため、Fog使用への完了応答や終局に数えない。

固定8seed×2状態＝16比較を追加し、累計55観測/native50/record-only5・wire464。runtime63/Snapshot63/view61は維持し、新しい完全受信lifecycleは0・累計2。重複capture02は除外し、ホーム復帰とNetwork Monitor終了を確認した。


## v76：激烈疾風剣48の自己使用と風邪

[受信2状態](../data/official-v2/wire-game-updates-v76.json)と[1完了操作](../data/official-v2/live-transition-witnesses-v76.json)を保存した。本人HP40/MP10/CP20から単品48を自己使用し、追加defend無しで13被害→Cold→disease1/1被害→通常58となりHP26へ進む。

次CPUは4+168のSELLを起動するが、本人は応答せず途中退出した。課金/配送/Fever付与・58使用は観測に含まない。固定8seed×2状態＝16比較を追加し、累計56/native51/record-only5・wire480、完全受信lifecycle2を維持する。ホーム復帰とNetwork Monitor終了を確認した。


## v77：霧鉄砲72のUI自己使用

[新UI記録](../data/official-v2/live-transition-witnesses-v77.json)では、単品72/ATK3を本人へ使い、追加防御なしでGF1→2・HP40→37・MP10/CP20維持、CPU202待ちへ進んだ。本人FogでCPUの資源欄は非表示となった。

raw受信本文は保存できず、実補充品・CPU真手札・rawイベント順は未取得である。この例はrecord-onlyとし、新たなnative権威的state比較やwire replayを増やさない。累計57/native51/record-only6・wire480。CPU202へ応答せず途中退出し、ホーム復帰とNetwork Monitor終了を確認した。


## v78：邪神の大剣53の単品自己使用

[受信2状態](../data/official-v2/wire-game-updates-v78.json)と[1操作](../data/official-v2/live-transition-witnesses-v78.json)を保存する。本人HP40/MP10/CP20から単品53を自己使用し、14被害2回→HP12/MP10/CP20→通常160補充、追加本人防御なしでCPU19待ちへ進んだ。

CPU19には応答せず途中退出した。初期1/使用後2を8固定seedで16比較、累計58/native52/record-only6・wire496、完全受信lifecycle2を維持する。sourceとtargetが同じなので役割別の減算順や低HP・復活を識別したとは数えない。ホーム復帰・Network Monitor終了を確認した。


## v79：霧鉄砲72のraw自己使用とCPU情報の非表示

[新受信2状態](../data/official-v2/wire-game-updates-v79.json)と[1完了操作](../data/official-v2/live-transition-witnesses-v79.json)を保存する。本人40/10/20から72を自己使用し、3被害→Fog→通常10、本人37/10/20・CPU40待ちとなる。UIではCPUの資源欄が隠れるが、rawにはCPU40/10/20と正コピー8+空行1が残る。

初期1/使用後2を8固定seedで16比較、累計59/native53/record-only6・wire512、完全受信lifecycle2を維持する。v77のUI-only記録は今回の別対局で補完せず保持する。prep捕獲01の別room2状態は除外し、CPU40には応答せず、ホーム復帰・Network Monitor終了を確認した。


## v80：フラッシュダガー70の自己使用

[受信2状態](../data/official-v2/wire-game-updates-v80.json)と[1操作](../data/official-v2/live-transition-witnesses-v80.json)を保存する。本人40/10/20から70を単品自己使用し、追加本人防御なしで2被害→Flash→実gift4、本人38/10/20・次CPU10待ちとなった。UIには「まぶしい」と出る。

初期sourceは2game混在のindex1を採用し、異なるprep documentのindex0を除外する。初期1/使用後2を固定8seedで16比較、累計60/native54/record-only6・wire528、完全受信lifecycle2は維持する。CPU10には応答せず、一枚/二枚応答を公式UIで追加検証したとは扱わない。ホーム復帰・Network Monitor終了を確認した。


## v81：空応答から発火ワンド66の自己使用

[受信3状態](../data/official-v2/wire-game-updates-v81.json)と[2操作](../data/official-v2/live-transition-witnesses-v81.json)を保存する。CPU1/viewer2、最初はCPU29の8攻撃待ち。本人は空応答で40→32となり、その後66を自己使用して32→30・MP10/CP20維持、追加本人防御なしで実gift21を受け取った。CPU196のMP＋10が続き、GF4本人mainで停止した。

CPU196/3の通常補充はsnapshot deltaだけで、giftイベントを作らない。初期02は01の完全重複として除外し、preCPUlaunch native再構成をraw取得に数えない。固定8seed×3状態＝24比較、新2完了操作で累計62/native56/record-only6・wire552、完全受信lifecycle2を維持する。ホーム復帰・Network Monitor終了を確認した。


## 作業世代v82：30の画面観測と既存201召喚の条件付き再生

新CPU修行はviewer2/CPU1、終末なし。初期raw1はGF1 CPU214「岩」の8攻撃保留で、CPU40/6/20・viewer40/10/20、viewer30/id7を含む9枚。空応答を確定するとUIでGF2本人32/10/20となり、30だけを選んで本人へ主動作ペインから確定すると、追加の本人防御なしで19/10/20・GF3 CPU鎖ガマ5保留へ進んだ。CPUの表示名から台帳19は対応するが、use19のrawやslotは取得していない。

[初期受信の文脈](../data/official-v2/wire-game-updates-v82-hell.json)と[2操作のUI記録](../data/official-v2/live-transition-witnesses-v82.json)を分ける。02/03/05は01全体の重複、04はkeepaliveだけであり、post受信状態は0。通常補充とCPU在庫変化は不明のままにする。HP13減少は既存の本体8＋Hell非悪化tick5と一致するが、病気付与や減算のraw列を取得した証拠ではない。再読込で生誕画面へ戻り、Network Monitorを閉じた。通常の退出操作や終局取得をしたとはしない。

別に、既存v65の201召喚1例はNeptuneを外部結果として条件付き再生する。[再生報告](../data/official-v2/summon-replay-v82.json)で原記録と区別し、2状態×8seed＝16比較を追加する。新2観測と既存1例の移行により累計64/native57/record-only7・wire568、完全受信lifecycle2/routing1/internal trace0。


## 作業世代v83：既存Neptune276例の条件付き再生への移行

今回は新しい公式操作や本文取得を行わず、2026-11-05のv65記録を再利用する。2026-11-06付の[再生報告](../data/official-v2/automatic-guardian-replay-v83.json)で、201召喚から壁233→Neptune276→補充までの3受信状態を固定8seedで比較する。元v65 JSONとv82の召喚部分再生reportは変更しない。

CPU2の同一batchでeligibleな守護神sourceはviewer1/Neptuneだけであり、発動あり・276・target1を外部結果として渡す。受信boostHP5はplayerId省略のまま保持し、viewer資源の40→45から対応を確認する。壁233はMP6を支払いused保持、viewer123/id10は明示gift、CPU200/id2・19/id10は在庫deltaとして供給する。内部抽選・省略CPU補充順の証明とはしない。新観測0、64/native58/record-only6・wire592、完全lifecycle2/routing1/internal trace0。


## 作業世代v84：新しい30自己使用で8＋5と175を取得

2026-11-06の修行・2人・終末なしで、最初の配布に30があり、追加の準備操作なしで単品自己使用した。[初期1と使用後2](../data/official-v2/wire-game-updates-v84.json)、[1操作のwitness](../data/official-v2/live-transition-witnesses-v84.json)を保存する。viewer1/CPU2、初期GF1・双方40/10/20から、use30/id6→target1→damage8→addHell→disease1→damage5→gift175/id6→advance2→CPU16/id2→target1が並ぶ。UIでも本人27/10/20・GF2 CPUハチェット4保留を確認した。自己defendイベントは無い。

capture02は初期01の完全重複で除外し、新しいpostは03から採用した。原イベントでplayerIdが省略されている箇所を補記せず、役割と資源からの導出を分ける。本人slot6の再利用は新しい175コピー、CPU16/id1は残存・id2は使用済みではなく消費後の保留である。2状態×既存8seed＝16比較、新1観測で65/native59/record-only6・wire608。病気の一般率や初期抽選・server RNGを同定したとはしない。

前のv82 UI-only例は別対局としてそのまま残し、今回の175やイベント列を遡及して補完しない。CPU16には応答せず通常退出し、ホーム復帰・Network Monitor終了を確認した。終局取得や完全lifecycleの増分は0、累計2のままである。


## 作業世代v85：独立Chromiumで空応答と21＋74自己使用

2026-11-06、headless Chromiumの新しい一時contextで生誕→2人・終末なしのCPU修行へ進んだ。通常Firefoxには触れていない。候補を探した準備対局は途中退出し、採用対局はviewer2/CPU1。最初の受信update1はCPU6/id4の1攻撃保留で、viewer2の両選択を空にして応答側main paneから確定するとHP40→39・GF2となった。footerの「許す」はsummary表示であり、そこへのクリックや一時選択/解除では更新は発生していない。

次に21/id4と74/id9をこの順に選び、本人をtargetにして攻撃側main paneから確定した。追加本人応答なしに8被害でHP31、MP10/CP20保持、118/id4→200/id9補充、GF3/CPU44/id9の12攻撃待ちへ進んだ。[受信3状態](../data/official-v2/wire-game-updates-v85.json)と[2操作の記録](../data/official-v2/live-transition-witnesses-v85.json)を保存する。CPU26/id4はupdate2の在庫差分であり、省略されたgift eventを補わない。

採用sourceは52185/35920 bytesの不変なHTTP response prefixで、どちらも保存時completed:false/capture_error:false。各20/5完全framesを厳密に解析し、Aのframe17とBのframe1/4（0-based）だけを採用、Aの別準備gameを除外する。全prefixのhashを残し、採用gameのfield省略・配列順・ghost行を匿名化以外は保持する。3状態×8seed＝24比較、新2完了操作で67/native61/record-only6・wire632。受信がframe境界で終わっていることをHTTP全体や対局の完了に読み替えない。CPU44未応答のまま通常退出し、メニュー復帰と一時ブラウザ終了を確認した。完全lifecycle2/routing1/internal trace0は維持する。

Release CTest157/157（111.24秒）、変更したwireテストのDebug ASan/UBSan1/1（1.29秒）、新しい受信parserの独立8/8テスト（0.548秒）が通過した。Node構文検査・実headless CPU操作・両buildのwarning/error0・diff-checkも確認した。native259 callbacks/Python187 Match methodsと実行挙動は維持し、全SAN suite・新しいcore/C API SAN・Python SANの実行とは区別する。経過時間を版間の速度比較には使わない。


## 作業世代v86：可視探索で得た83の単品自己使用

2026-11-06、独立headless Chromiumの[有限探索](official-headless-probe.md)を最大12試行で開始し、5試行目に83の可視画像を検出して停止した。前4gameは通常退出とメニュー復帰を確認し、採用対局の手札は操作せずrootへ引き継いだ。別contextの手動準備1gameも通常退出済みであり、これらを新完了操作に数えない。

viewer1/CPU2・終末なし・双方40/10/20から、83/id6だけを選び本人をtargetにして主動作ペインで確定した。追加本人応答なしで5被害→HP35、MP10/CP20維持、実26/id6補充・GF2/CPU223待ちを確認した。[初期と使用後](../data/official-v2/wire-game-updates-v86.json)では use83→target1→damage5→gift26→advance2→CPU223/id8→target1 が並ぶ。damageイベントのplayerId省略は原文のまま保持する。

初期prefixAは128041 bytes/51framesのframe50（0-based）のみ採用し、前4prepを除外する。postprefixBは20318 bytes/4framesのframe2を採用する。両者は保存時HTTP未完で、末尾不足0byteでも全HTTP本文や完全対局とは呼ばない。CPU223はused保持され、全2状態37行（positive36＋ghost1）を確認する。追加keepaliveだけのprefixを新状態に数えない。CPU223には応答せず通常退出・メニュー復帰・ブラウザ終了を確認した。

新1操作・16比較で68/native62/record-only6・wire648。完全lifecycle2/routing1/internal trace0を維持する。34/216や低HP・死亡、選択条件のない自然配布率は今回の記録から確定しない。

Release CTest158/158（132.11秒）、Debug ASan/UBSanのcore/C API/wire3対象3/3（30.19秒）が通過した。新たに登録したparser対象は9テストを含み、別の単独実行も9/9（0.484秒）。finder/probe構文・不正CLI8件の起動前拒否・実探索からの引継ぎ・両buildのwarning/error0・diff-checkを確認した。native259 callbacks/Python187 Match methodsと実行挙動は維持し、全SAN suite/Python SANやfinder全失敗・中断経路の検証とは区別する。経過時間を版間の速度比較には使わない。


## 作業世代v87：自己両替HP0の初期から終局まで

2026-11-06、headlessの2人・終末なしCPU修行で3を探し、回復後contextの2試行目で停止した。viewer1/CPU2双方40/10/20・各9枚、208/112/guardianなし。3/id5選択後、所持金側+10を4回押してpreview0/10/60を確認した。この時点の実statusは40/10/20で、主動作ペイン確定1回だけが完了操作である。

[受信初期と終局](../data/official-v2/wire-game-updates-v87.json)は連続update1/2、GF1/owner1のまま。postイベントはuseAttackItems(3/id5)→exchange(cp60,mp10)→die(player1)→endGame。exchangeのHP/actor省略を保ち、gift/advanceGF/damage/diseaseは無い。viewer0/10/60・dead、CPU40/10/20・生存、viewer残8品＋ghost1、CPU9品維持を確認した。全2状態36行（positive35＋ghost1）を保存する。UIもCPU勝利を表示した。

初期prefix51913 bytes/18framesのframe16、post16122 bytes/3framesのframe1を採用（0-based）。両HTTP未完の完全frameから採用し、全prefixhashと省略/配列順を保持する。採用Aの前1prepは別docとして除外。別contextの48探索・中断前3prepも新観測へ数えず、うち再起動時の3番目の正常退出は未確認である。

新1操作・16比較で69/native63/record-only6・wire664、完全受信lifecycleは2→3、routing1/internal trace0。終局後『戦いを終わる』を試したがメニュー復帰待ちはtimeoutとなり、続くcloseでbrowser終了を確認した。メニュー復帰済みとは記録せず、確定済み終局と分ける。34/216、208/112併用は残件である。

Release CTest158/158（111.60秒）、変更したwireのDebug ASan/UBSan1/1（1.75秒）が通過した。Releaseはparserの9テストを含む。finder構文・不正CLI8件の起動前拒否・trade3の実探索引継ぎ・両buildのwarning/error0・diff-checkを確認した。native259 callbacks/Python187 Match methods、runtime本体・根拠descriptorは維持する。全SAN suite・新しいcore/C API SAN・Python SANや、全UI終了経路の検証とは区別し、経過時間を版間速度比較に使わない。


## 作業世代v88：終局後は修行設定を経由してホームへ復帰

2026-11-06、[headless probe](official-headless-probe.md)の独立Chromiumで2人・終末なしCPU修行を1試行だけ実施した。probe起動からbrowser終了まで65.072秒、修行開始クリックから60.714秒であり、最大3試行・各90秒以内の制限内で終了した。追加の公式接続・探索は行っていない。

初期はCPUの「火の粉袋」75%攻1への応答待ちで、本人40/10/20。空応答後はGF2・39/10/20となった。可視手札の両替3を選び、画面を確認して所持金側+10を4回押すとpreviewは29/10/30→19/10/40→9/10/50→0/10/59となる。実status39/10/20のまま、主動作ペイン(350,250)を1回クリックして確定した。以降の画面で本人HP0・「昇天」、続いてCPUの「勝利」と「戦いを終わる」を確認した。終局前後のraw gameを新しいwire fixtureとして採用したものではない。

以下は起動を0秒とした時刻付きUI記録である。本文は識別名を本人/CPUに置換した主要表示を抜粋し、表示入力欄はprobeの `inputs` を記す。

| 時刻 | 操作・画面stateの本文 | 表示入力欄 |
| --- | --- | --- |
| 47.190秒 | 終局画面。「修行」「教典」「G.F.2」「勝利」「CPU」、本人HP0/MP10/¥59、CPU40/10/20、「戦いを終わる」 | `[]` |
| 50.380秒 | 完全一致の「戦いを終わる」を1回クリック | — |
| 50.397秒 / 50.400秒 | 操作直後と直後のstateは終局画面を維持。「G.F.2」「勝利」「戦いを終わる」 | `[]` |
| 53.674秒 | 次のstateは修行設定。「修行」「教典」「本人」「修行者 2 人」「終末の時」「なし」「戦いを始める」。GF・勝利・終了ボタンは消失。画像でも設定画面と左上の戻る矢印を確認 | `[]` |
| 60.222秒 | 修行設定画面の左上の戻る矢印(240,23)を1回クリック | — |
| 60.234秒 / 60.237秒 | ホーム本文は「教典」「本人」「修行」「コンピュータと対戦」「隠れ乱闘」「友達と対戦」「真剣タイマン」「2人個人戦」。GF・「戦いを始める」・「戦いをあきらめる」は無し | `[]` |
| 65.064秒 / 65.072秒 | closeの `closed:true` とprobe正常終了を確認 | — |

この1例では終了ボタン1回で直接ホームではなく修行設定へ戻り、追加の戻る矢印1回でホーム復帰を確認した。終了クリック後の設定確認は3.294秒後であり、遷移に常にその時間が必要という測定ではない。戻る矢印後は0.012秒でホームstateを取得した。終了ボタンの連打・再読込・再生誕・再ログインは不要だった。設定画面に留めたのは終了クリック後9.842秒までで、それ以上待った場合の自動ホーム遷移は未確認である。v87でホームの文字だけを待つとtimeoutした理由と整合する経路だが、v87当時の未取得stateを今回の別対局で補完しない。

私有の時刻付き本文・入力欄・画像・probe終了時prefixは `/tmp` 配下に0600で保存し、原識別子や認証情報を公開記録へ転記していない。公開wire/witness・native/runtimeの変更は無く、既存の69観測/native63/record-only6・wire664・完全受信lifecycle3/routing1/internal trace0の集計は維持する。今回の追加は終了後UI経路の観測であり、新しいnative比較・完全受信lifecycleの認定は行わない。ホーム復帰と一時ブラウザ終了を確認して調査を終了した。


## 作業世代v89：34単品自己使用の受信・response待ち不一致と修正

2026-11-06、標準入力をFIFOで開いたまま、独立headless Chromiumの有限探索を `--targets 34,216 --max-trials 12` で1回実行した。6試行目で可視34を検出して停止し、再実行はしていない。前5対局は通常退出・ホーム復帰を確認した。採用対局は2人・終末なし、viewer1/CPU2、G.F.1・双方40/10/20、各9枚。候補停止時は手札未操作で、34/id6のみ選択→本人targetの矢印を画像確認→主動作ペイン(350,330)の1回確定と進めた。各操作後にstateを確認し、他のカードや追加の本人responseは入力していない。

[初期1と使用後2の受信](../data/official-v2/wire-game-updates-v89.json)では、use34/id6→target1→damage9→gift4/id6→advance2→CPU79/id6→target1。本人HP40→31、MP10/CP20維持、CPU40/10/20、G.F.2でCPU79氷結ハンマーへの応答待ちとなった。damageのplayerId省略、配列順、ghost行を保持する。初期prefix-Aは156986 bytes/57完全framesのframe55、postprefix-Bは19633 bytes/4完全framesのframe2（0-based）を採用し、同じgame文書であることを私有資料で確認した。両HTTP未完・capture_error:false、末尾不足0byteであり、HTTP全体や終局の完了を意味しない。全2状態36行（positive35＋ghost1）を保持する。

全prefixのgame一括decoderは、採用外の準備対局にある既知allowlist外のevent文字列 `hit` を拒否した。parserの許容範囲は変更せず、厳格なUTF-8/UTF-16 framingで区切った採用frame55だけを既存の `sanitize_game` で検証し、postframe2も既存parserで検証した。未知値を採用fixtureへ混ぜず、準備5文書は除外した。公開資料には原識別子・文書path・認証情報を転記せず、私有出力は/tmp配下0600に保存した。

[操作witness](../data/official-v2/live-transition-witnesses-v89.json)と `tests/official_wire_trace_test.cpp` のv89比較を追加した。nativeは固定seed0で34単品selfを1回実行すると本人HP40・queue1・actor1の本人response待ちとなり、公式の追加responseなしHP31と食い違った。初期受信状態の比較後、補充前のこの不一致でテストを停止する。8seed全件やpost終点の一致を主張せず、新規成功wire比較は0、既存累計664を維持する。エンジンはユーザー指定により修正していない。

新1完了観測で累計70、native対象は63のまま、record-onlyは7とする。完全受信lifecycle3/routing1/internal trace0を維持する。216、34の合成・奇跡を弾く応答・低HP/死亡境界、公式配布率は未確認である。CPU79には応答せず、戻る→「戦いをあきらめる」からホーム復帰をstateで確認し、closeのclosed:true・正常終了を確認した。追加の公式接続・探索は行っていない。

Releaseのwire対象ビルドは成功。build内のRelease CTestは156/158（109.68秒）で、v89の既知不一致による `official_wire_trace` と `official_webchannel_parser` が失敗した。後者は私有出力用umask077がテストの公開directory作成にも継承され、想定した拒否が発生しなかったもの。umask022で同parser対象のみ再確認すると1/1（0.57秒、内部9テスト）が通過した。wire不一致は残したまま停止し、エンジン修正・commitはしていない。全suiteを再実行して157/158を得たとは扱わない。


### タスク4：既存受信を用いた34限定の修正（追加観測なし）

上記は修正前の結果として保持する。過去の41・Cold・Fog・53・66・70・30と同じ通常mainの直接DEF0経路へ、実モデル34/bounceMiracle・明示self・hit0・Wide無しだけを追加した。費用/消費・body・owner tick・補充・次owner/GFは既存処理へ接続し、HP算術は変更していない。既存受信の初期1→使用後2を固定8seedで16比較し、追加本人responseなしでHP40→31・MP10/CP20維持、実補充4、GF2・CPU79応答待ちまで一致した。受信damage9や終点HPをエンジンへ注入したものではない。

同trait全体へ広げずモデルID34で限定した。216は台帳上はbounceMiracleではなく吸収奇跡であり、未観測のまま従来の直接吸収経路を維持する。foreign・redirect・Wide・guardian/payloadの別入口も従来どおり。rules70/Snapshot70/view68へ更新し、Snapshot旧1〜69は拒否する。Match ABI2・既存C構造体幅・State/独立表示componentは維持し、header/C/Python/observeのself_attack_rule契約を同期した。

新しい公式観測は0件。既存v89の1件をrecord-onlyからnative対象へ移し、累計70観測/native64/record-only6・wire680、完全受信lifecycle3/routing1/internal trace0とする。witnessには修正前の失敗証拠を保存したまま修正後の比較を記録する。216、34の合成・低HP/死亡境界・奇跡を弾く応答・公式配布率、HP算術の内部順は未確定である。今回の一致から一般のserver RNGやdeath/tick/gift全順序を証明しない。追加の公式接続・探索・観測、git commitは行っていない。


修正後の最終検証はRelease CTest158/158（109.32秒、Python 41対象を含む）と、Debug ASan/UBSanのcore/C API/wire3対象3/3（30.78秒）が通過した。両buildのwarning/errorは0、diff-check正常。全CTestはumask022で実行し、前回失敗した `official_webchannel_parser` も同じ全suite内で通過（0.58秒）した。coreでは34の単発消費・未選択防具の保持・無bounce draw・補充resume・foreign/redirect/Wide/guardianの除外・216の従来動作・旧Snapshot拒否・cutoff・失敗時rollbackを検査した。全SAN suiteやPython SANの実行とは区別する。


## 作業世代v90：取得済み216単品自己使用とnativeの一致

公式へ接続せず、別セッション取得済みの2人・終末なしCPU修行の[使用前update2と使用後update3](../data/official-v2/wire-game-updates-v90.json)を取り込んだ。viewer2/CPU1、GF2・本人HP30/MP10/CP20から216/id9のみ自己使用し、use216→target2→damage10→boostHP10→gift195/id10→advance1→CPU64/id1→target2を受信した。本人HP30/MP0/CP20、GF3、通常手札9枚＋保持used216の計10copyとなり、CPUは40/10/20で64の応答待ち。damageのplayerId省略、usedの有無、配列順、CPUのid0/model0 placeholderを保持し、2状態37行（positive36＋placeholder1）を採用する。前状態のCPU攻撃への空応答は今回の再生対象へ数えない。

抽出rawは14974/20563 bytesで各1完全frame。元prefixは15118 bytes/3framesのframe1、35807 bytes/5framesのframe3（0-based）、末尾不足0byte。既存parserでrawと元prefixをそれぞれ再解析し、採用gameの一致、同じ私有文書、出典hashを確認した。capture metadataは双方completed:false/capture_error:false。HTTP完了・初期配布・終局の証拠とはしない。原識別子・文書path・認証情報・rawは公開記録へ含めない。提供observationの探索は12＋4＝16試行、採用以外15対局を除外し、ホーム復帰・browser終了は同資料の確認結果として記録する。

[操作witness](../data/official-v2/live-transition-witnesses-v90.json)とwireテストのv90を追加し、固定8seed×2状態＝16比較が既存nativeのまま一致した。HP30/MP0・同tokenのused216保持・1補充予約を単一play後に確認し、実195を明示ArtifactGrantで供給してCPU64を通常playする。各native境界の完全snapshot restore、資源・全model/fake/used・GF/owner/pendingを比較する。受信damageやHPを効果へ注入せず、seed探索・除外もない。nativeの受信イベント列リプレイやbackend内部HP処理順の確定は主張しない。低HP/上限/死亡・FREE/合成・redirect・CPU64解決・分布は残件である。

実行挙動とAPIの修正は不要で、rules70/Snapshot70/view68・ABI2、34とその他モデルの挙動を維持する。v89時点のdescriptor中の216未観測ラベルは履歴として据置き、216の新しい実測根拠は本v90に記録する。新1完了観測で累計71/native65/record-only6・wire696、完全受信lifecycle3/routing1/internal trace0。公式接続・探索の再実行・git commitは行っていない。

最終検証はbuild内のRelease CTest158/158（109.87秒）と、build-sanitizeのDebug ASan/UBSan core/C API/wire3/3（27.60秒）が通過した。両ビルドのwarning/errorは0、diff-check正常。umask022で全CTestを実行し、official_webchannel_parserも同suite内で通過（0.60秒）した。raw/匿名化fixture/C++手札転記の全37行一致も再確認した。全SAN suiteとPython SANは実行対象外である。

## 作業世代v91：取得済み杵94＋全敵化232（臼なし・敵1人）

公式サイトへ接続せず、別セッションで取得済みの2人・終末なしCPU修行を取り込んだ。[wire](../data/official-v2/wire-game-updates-v91.json)はviewer1/CPU2のupdate1/2、GF1→3、本人40/10/20→40/5/20、CPU40/10/20→30/14/0。94はあぶないキネ（杵）であり、臼は209。両初期手札に209はない。

受信順はuse94/id9＋232/id3→danger→target2→defense130/id3→damage26→本人gift177/id9・196/id10→advance2→CPU use3/id3→exchange hp30/mp14→advance1。damageの対象省略、exchangeのcp省略、配列順、used232の保持、CPU最終131/id3を保持する。元prefixをcanonical parserで再解析し、抽出済みJSONとの一致と同一documentを非公開で確認した。原名・userId・updatedAt・document/room識別子・認証情報・ローカルpathは公開しない。prefixの完全frame9/3と採用位置6/1、HTTP未完了を別々に残す。

保存済み公開clientのdanger分岐は2000ms待機だけで、payloadを読まない。parserは観測されたaction-only形だけを許可し、payload追加・未知actionは引き続き拒否する。markerだけから対象抽選・ダメージ・server内部順を断定しない。

実行器には**正確な94→232の2枚、2人roster、生存敵1人、どちらの手札にも臼209なし**の例外だけを加えた。通常のWide対象列を使って唯一の敵へNE30を送り、dangerの全alive抽選を行わない。通常MP5支払い・used232保持・防御130/DEF4・補充・両替を再利用する。臼が本人/敵にある場合、死者を含め3人以上のroster、敵不在、追加カードを含む合成はunsupportedのまま。既存samplerは従来どおりdanger＋Wideを提案しない。臼優先・複数敵・他の合成・redirectの実測根拠を増やさない。

[操作witness](../data/official-v2/live-transition-witnesses-v91.json)とwireテストv91で8固定seed×2状態＝16比較が一致。初期全在庫を読み、通常play94＋232・defend130でCPU HP14を計算し、本人177/196とCPUの中間3を供給、CPU両替30/14/0と最終131を実行する。中間gift3は後続use3と最終在庫から補う再生入力で、受信giftイベントではない。HP14と資源合計44はnative計算で、受信中間snapshotではない。server内のHP算術・補充/防御/両替の内部順、CPU方策、初期配布、RNG同値性は確定しない。全native境界のsnapshot復元と資源・全model/fake/used・GF/owner/pendingを比較し、seed探索・受信damageの注入は行わない。

rules71/Snapshot71/view69へ更新し、Snapshot旧1〜70を拒否。Match ABI2とC構造体幅は維持する。新1完了観測で累計72/native66/record-only6・wire712、完全受信lifecycle3/internal trace0。供給資料では通常退出後メニュー復帰とブラウザ終了を確認済みで、今回接続・探索・git commitは行っていない。

単回のCPU対象観測は全alive抽選の可能性を統計的に排除しない。唯一の敵へのWide経路は、この条件内の限定実行規約であり、serverの対象分布の確定ではない。検証はRelease CTest 158/158（Python関連を含む、38.98秒）、Debug ASan/UBSanのofficial_match・official_match_c_api・official_wire_trace 3/3（45.95秒）、parser 11テストとumask022で実prefix2本のCLI再解析が通過した。出力0600/親0700と元bytes/mtime不変を確認し、Python構文検査・git diff --checkも通過。build警告/エラー0、全sanitizer suiteとsanitizer下のPythonは未実行。

## 作業世代v93：取得済み111対象指定なしmiss

公式サイトへ接続せず、別セッション取得済みの2人・終末なしCPU修行を[wire](../data/official-v2/wire-game-updates-v93.json)・[witness](../data/official-v2/live-transition-witnesses-v93.json)へ取り込んだ。v92はexecution-subsetの合法手列挙で使用済みのため観測番号をv93からとする。viewer1/CPU2、111（プチサターン、命中率25%、atk20）を対象指定なしで単品使用した。本人の明示self-target操作は確認されていない。受信update1→2、GF1→3、本人40/10/20維持・手札9→9。use111/id6→miss(playerId2)→gift180/id6→advance2→CPU use3/id3→exchange30/30/10→advance1。CPUは40/10/20→30/30/10、最終slot3は19。被害イベントや追加本人responseはない。

抽出rawは22435/18669 bytes、各1完全frameのframe0。元prefixは76051/94720 bytes、完全frame29/30のframe26/29（0-based）で、抽出rawの匿名化gameと一致した。同じ私有document、bytes/hash、末尾不足0、HTTP completed:false/capture_error:falseを元captureで確認した。元prefixの準備対局を含む一括game検証は行わず、厳格UTF-8/UTF-16 framingとFirestore decodingで採用frameを切り出し、production sanitizerを適用した。全2状態36在庫行の配列順・省略fieldを保持し、原名・userId・updatedAt・document/room識別子・認証情報・raw・ローカルpathは公開しない。提供資料の探索11試行・ホーム復帰・browser終了は既存セッションの記録であり、今回再実行していない。

公開clientのmissはplayerIdの対象選択と視覚表示だけを根拠とし、parserへ同game内の整数playerIdを持つ既知の2field形だけを追加した。未知payloadとhitは引き続き拒否する。nativeの通常playは内部 `bounded(s.rng,100)` で判定し、external chanceは通常Commandの命中結果を指定しない。既存の外部結果APIでmissを供給できないため、ユーザー指定の停止条件に従いエンジンを変更せずrecord-onlyとする。C++比較はseed0で初期受信状態を照合し、targetless playをexternal chance付きchoice_stepで1回診断して、外部stream不変・内部RNG消費を確認する。seed0の111はlocal missとなったが、この1件だけで指定可能とは扱わない。補充やCPU両替を供給した終点再生、8seedのmiss経路比較、seed探索・hit/miss分布の確定は行わない。

新1完了観測で累計73/native66/record-only7。新規成功遷移wire比較0、累計712を維持する。診断用初期状態比較は成功遷移数に加算しない。完全受信lifecycle3/internal trace0、runtime/API/schema変更なし。追加公式接続・git commitなし。

## 作業世代v94：取得済み108対象指定なしmiss

同じく公式サイトへ接続せず、取得済み[wire](../data/official-v2/wire-game-updates-v94.json)・[witness](../data/official-v2/live-transition-witnesses-v94.json)を追加した。viewer2/CPU1、2人・終末なし。108（魔神の木馬、命中率75%、atk8）を対象指定なしで単品使用し、本人の明示self-target操作は未確認。受信update2→3、GF2→3、本人33/10/20維持・手札9→9。use108/id8のみ→miss(playerId1)→gift212/id8→advance1→CPU use47/id5→target2。CPU40/10/20維持、最後は47への本人応答待ちで、その解決は未観測。使用前HP33は先行するCPUの7攻撃・空応答後の受信状態であり、今回の新しい使用操作へ含めない。

before/after rawは実際には累積prefix41938/61012 bytes、完全frame10/12、採用frame8/10（0-based）。production parserで両prefixを全体解析でき、after内frame8もbefore採用gameと一致し、同documentを私有資料で確認した。双方末尾不足0、元captureはHTTP completed:false/capture_error:false。全36行（positive35＋CPU id0/model0 placeholder1）の省略field・順序を維持する。rawのファイル名から単一frameと推定しない。提供資料の探索9試行、ホーム復帰・browser終了を転記し、原識別子・認証情報は公開しない。

v93と同じ既存API制約によりrecord-only。108はseed0でlocal hit（chance_rolls1/hits1）となり、受信missと食い違った。この1回の違いを確率分布の不一致とは扱わない。seed0の初期状態比較と外部chance不使用の診断で止め、命中判定RNGの差をエンジン修正やseed探索で吸収しない。補充212→CPU47までの8seed終点一致は未検証であり、内部RNGの分布が公式と異なるという認定でもない。新1完了観測で累計74/native66/record-only8・成功wire712、完全受信lifecycle3/internal trace0。runtime/API/schema変更・公式接続・git commitなし。


v93・v94の最終検証：Release全build成功・CTest161/161（50.53秒）。変更対象のDebug ASan/UBSan `official_wire_trace` 1/1（2.41秒）、umask022のparser回帰13/13が通過した。原データ4本をCLIで再解析して採用game/hashを照合し、出力0600/親0700・入力bytes/mtime不変、元captureのhash/completed/capture_errorとC++初期在庫全36行の転記一致も確認した。build warning/error0・git diff --check正常。全sanitizer suite・Python sanitizerは実行していない。このテスト成功はrecord-onlyの2遷移がnativeで再現できたという意味ではない。
