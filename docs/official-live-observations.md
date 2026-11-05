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
