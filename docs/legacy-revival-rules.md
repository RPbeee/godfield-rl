# 旧版の自動蘇生244 REVIVE

調査日: 2026-10-31。Igoorxの非公式Flashサーバーエミュレータ公開masterの一次sourceを対象とする。
現行公式の完全再現とは区別する。Fog/Illusionと117死亡時攻撃は今回の追加対象にしない。

## 台帳と手動操作

[CSV][csv]の正重量REVIVEは244単種。
SUNDRY、attackKind空、attackExtra=REVIVE、属性空欄/NE、value10、price10、weight1。
MP・率・subvalue・defenseKind・defenseExtraは空。ATK/DEF/boostや保持奇跡ではない。
[初手検証][valid]はattackKind空のSUNDRYを拒否するので通常USEできず、追加攻撃にもならない。
FREE carrierでもない。

**手動DISCARDは可能。** [DISCARD解決][discard]で除外されるのは245MORTARだけ。
botの「244を捨てない」という方策を合法性制約にしてはいけない。
既存subset同様、効果のないSUNDRYを任意の防御犠牲に使う旧server経路は別契約とする。

## 死亡境界の順序

[Room.handlePlayersDeath][death]はroom.players（参加順、duelでは固定seat0→1）を走査する。
手番順のシャッフルリストではない。HP正または既deadなら飛ばす。
HP0・dead=falseについて、[getAliveCount][alive]のnot.dead人数が2以上で244を持てば、
最初の244を[自動処理][dying]で直接discardし、increaseHP(10)してcontinueする。
MP、YEN、保持奇跡、病気、harms、worseChanceは変えない。
失敗した場合に初めてdead=trueへ進む。一度deadとなった人物の復活機能ではない。

自動消費はusePieceとdeal+1を通らないので、**補充予定は0**。
通常の被害処理がHPを0へclipする現在の0..99環境ではHP0→10となり、1回の走査で1人1枚だけ消費する。
複数244は後の別の死亡境界で使える。1枚で以後の死亡まで免除するものではない。
蘇生分を補充する、1hookで全244を使ってHP20/30にする、MPを回復する、といった処理は誤り。

原sourceのincreaseHPは加算後の99上限であり「HPを10に設定」ではない。
旧EXCHANGEの無境界入力で負HPを作った場合は-15→-5のようになり、その同じ走査では再確認しない。
この負HP経路はv25から明示しているcanonical 0..99領域の外で、導入しない。

### 同時HP0の非対称

両者のdead=false・HP0から、1回の死亡走査を行う場合。

| 手札244 | seat0処理後 | seat1処理後 |
| --- | --- | --- |
| 両者なし | seat0 dead、alive1 | seat1もdead |
| seat0だけ | seat0 HP10、alive2 | seat1 dead |
| seat1だけ | seat0 dead、alive1 | seat1の244は発動せずdead、手札には残る |
| 両者あり | seat0 HP10、alive2 | seat1もHP10、両者dead=false |

HP>0人数を数える、両者を一括dead化してから蘇生する、手番ownerを先に処理する、
全候補を走査前のalive数で同時判定する、のいずれもこの結果を変えてしまう。

## item・反応chain・病気・配布

[beforeEndInning/endInning][finish]の順序は次の通り。

1. 完了したitemの後に死亡走査（蘇生を含む）。
2. 残queueがあれば次itemへ進み、必要なら相手応答を待つ。
3. queueが空になってから元turn_ownerの病気を1回処理。
4. 病気でHP0なら死亡走査へ再帰。handledDiseaseThisTurnは既にtrue。
5. その他の未対応assistant処理を経て補充、終局確認、次手番。

反射の往復中はitem未完了なので、175でHeavenから落ちた暫定HP0にも244はまだ発動しない。
同じchain内で回復してHP正になれば、244を温存する。
Gloryは自動蘇生の選択枚数制限にならず、DARKによるHP0も同じ死亡境界へ届く。
蘇生はaddHarmではないためHeavenを消さず、再びHeavenから落とす処理でもない。

例: ownerのColdがHP1→0にした後、244でHP10へ戻る。同じCold tickを再実行してHP9にしない。
通常itemでHP0になって蘇生したownerは、その後に予定されていたCold tickを受けてHP9になる。
Heaven継続回復ならHP15になり得る。自然悪化によるHeaven落下でHP0になった場合は、
蘇生後もそのtickの再実行はなくHP10で止まる。
item境界で1枚、続く病気境界でさらに1枚という複数回消費は可能なので、1手1枚に固定しない。

## 既存counter・196・交換との交差

queue開始前に死亡走査があるため、244で救われたtargetはdead=falseとして次counterを受ける。
蘇生失敗でdead確定したtargetは既存通りskipし、244所持をpop時に改めて使わない。
193のself MP counterも、蘇生後なら自動解決、既deadならtarget skipとなる。

[194の入出金][effects]は元の順序を変えない。既dead sourceでも要求全量の入金を受け、
targetはYEN→MP→HPで支払う。その後の蘇生は支払を取り消さず、入金や請求を再実行しない。
duelでsourceが既deadならalive1なので、支払いでHP0となった最後のtargetは244を持っていても救われない。
HP0・dead=falseの両者が同じitem終了へ到達した場合は上の固定seat走査表に従う。

[196の反射後処理][reaction]は反射先deadなら即item終了する。既deadは蘇生対象外なので
「196でdeadへ返す→244で救って効果再開」という経路はない。
反射先が暫定HP0・dead=falseなら応答を継続でき、item完了後に244を判定する。

交換2でHP0へ配分した場合も、即時代入→死亡走査→queue空ならowner病気tickの順。
244を持ちalive2ならHP10へ戻り、交換の公知after値はHP0のまま、現在HPは10以上になり得る。
交換に対する相手応答や、244を明示選択するActionは追加しない。

## 公開通知と117

[playerDyingAttack][dying]はまずDYINGのpiece（ID244）をroom全体へbroadcastしてから消費/HP増加する。
このpayload自体にseat/nameは明記されず、既知の死亡処理文脈と走査順に依存する。
蘇生成功ではDIEを送らず、手札と公知HPが変わる。未来の244配布を公開してはいけない。
Nativeの公知蘇生イベントにseat/ID/消費数を持たせることは、意味的な観測契約として明記する。

117 DYING_ATTACKは[dead設定後][death]にalive>=2を要求する。
duelでは誰かがdead確定した瞬間にaliveは最大1なので、244が何枚あっても発火しない。
244で救われる人物はdead化せず117分岐自体に入らない。この結論は両者同時HP0でも変わらない。

## 最小scopeと受入条件

244単種を通常poolへ追加し、Actionは増やさず死亡走査へ任意の自動hookとして組み込める。
source通りの固定seat順、not.dead数、直接手札消費/gift0、1hook1枚と繰返し境界を保つ。
旧profileはflag=falseで従来の死亡処理・配布乱数を維持する。

受入確認は、通常被害/DARK/Heaven/病気tick/交換HP0、複数244、両者HP0の4通り、
ownerがseat1の走査順、反射chain途中の温存、193と194、196のdead取消し、
既deadの244不発、手動discard可/通常USE不可、補充1枚を誤って増やさないことを含める。
244で減った手札と補充予定の会計を混同せず、restoreの物理消費予算も確認する。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV#L245
[valid]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L78-L94
[discard]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L389-L403
[death]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py#L369-L399
[alive]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py#L100-L116
[dying]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L35-L48
[finish]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py#L426-L494
[effects]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L457-L464
[reaction]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L630-L644
