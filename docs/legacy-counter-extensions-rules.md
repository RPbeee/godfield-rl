# 旧版の反撃拡張187・191・192・193と順序付き応答

調査日: 2026-10-31。Igoorxの非公式Flashサーバーエミュレーターのwebソース読解。
公式の現行実測ではない。可変`master`を参照しており、台帳では取得物とhash・範囲を記録する。
元攻撃・死亡確定・190の基礎は[反撃メモ](legacy-counter-rules.md)を引き継ぐ。
今回の候補は既存190に187/191/192/193を追加する5種。188霧、189幻覚、194所持金吸収は除外する。

## 正確なデータと予約条件

[旧CSV][csv]では全てPROTECTOR/COUNTER、通常DEF0、価格10・重み1。MP消費・保持化はない。

| 旧ID | 固有属性 | attackKind / extra | 予約する値と対象 |
| --- | --- | --- | --- |
| 187 | 火 | ATK、hitRate75 | 普通防御後rの威力。生存している全敵へ1回ずつ |
| 190 | 土 | ATK | 威力2r。現在のincoming sourceへ |
| 191 | 光 | ADD_HARM / GLORY | 直接Glory。現在のincoming sourceへ |
| 192 | 闇 | ADD_HARM / DARK_CLOUD | 直接DarkCloud。現在のincoming sourceへ |
| 193 | 無属性 | INCREASE_MP | MP+2r。counter使用者自身へ |

全ての予約条件は[defenderCommand][defense]の共通ブロックにある。
普通DFS・防御時病気・195を処理し、属性適合を確認した後、残余r>0なら選んだcounterを列順に予約する。
191/192も予約にはr>0が必要。直接効果だから元被弾0でも予約する、という意味ではない。
特殊応答が発火した場合はcounter予約ブロックを通らず、prefix指輪の消費・配布だけが残る。

全counterは同じ応答の最終残余rを共有する。普通DFSが列の後ろでも、先の指輪だけ減算前威力を使うことはない。
元攻撃の効果適用前に予約し、元被弾による死亡を確定してからキューを順に起動する。
反撃起動時に指輪をもう一度消費したり配布予定を増やしたりしない。

## 187: 予約と命中判定は別時点

[queueAttack][queue]はATKかつ正のhitRateなら、forced経路でも全体攻撃へ展開する。
187の威力r・率75は予約時に決まるが、命中は[doAttack][hit]でキューを取り出した時に調べる。
1..100の整数で75まで命中、76以上はmiss。missなら防御応答を求めず次項目へ進む。
同じ対象への連撃ではない。

1対1の通常状態では相手1人へ正規化する。ただし生存敵の列挙は予約時のdeadフラグを使う。
例えば、死亡済みsourceから来た反撃を受けた人が187で返す場合、そのsourceは生存敵ではない。
敵が他にいないduelなら187のキュー項目を生成しない。
190/191/192は現在sourceを直接targetにして予約し、起動時にdead targetをスキップする経路なので区別する。

予約済み187はsourceが後でdeadになっても実行する。
targetのDarkCloudは起動時に確認し、あれば必中でhit RNG消費0。
したがって、先行192による付与が後続187の抽選へ影響する。

sourceの命中式はtarget.dead確認より前で、死者targetの項目も通常はRNGを消費する。
ただしdead targetならその後すぐreturnし、COMMAND XMLは作らない。
この非公開のhit/missを新しい公開LastChanceへ出すと、旧sourceで公開されない乱数結果を漏らす。
RNGだけ進め、公開イベントは前のままにする等の扱いを明記する。

## 191・192: 直接状態付与と普通防御

[inflictDamage][effect]のADD_HARMは、攻撃ダメージの有無と別に対象へ状態を付ける。
191/192の生成項目は通常ATKではなく、現在のモデルでは直接harmとして区別する必要がある。
普通DEFを多く使っても、直接付与を弱めたり打ち消したりしない。
195は属性を無属性へ変えるが、Glory/DarkCloudそのものを消さない。

192は固有属性が闇でも、残余ATKを持たない直接状態付与なので999ダメージを発生させない。
Glory/DarkCloudの重複・Heaven転落は既存[harms規則](legacy-harms-rules.md)を使う。
直接付与をHeavenへ実行すれば、重複排除より先に転落が起こる。

通常防具・195を受理するかは旧serverの属性適合に従う。
光191への通常防具は現プールでは適合しにくいが、195を含む応答は可能。
反撃への防御を選ばないbotとの差C05は、v13と同様にserver受理を採る場合も未解消として記録する。
5種ともPROTECTORなので、20/39/210/215のWEAPON/MAGIC向け特殊を使えるようにしてはいけない。

## 193: 自己対象MP回復

[defenderCommandのtarget分岐][defense]はINCREASE_MPだけtargetを元defenderにする。
したがって生成AttackDataのsourceもtargetもcounter使用者自身である。
incoming sourceへ外部の防御応答を求めてからMPを回復する経路ではない。

使用時に予約値2rを記録するが、MPを増やすのは元被弾・先行キューの後、193が起動した時。
自分がtargetなので[doAttack][hit]の空応答で即解決し、[Player.increaseMP][player]が上限99へ制限する。
queue payloadの2r自体を通常MPコストの範囲へ切り詰める必要はない。

元被弾などでcounter使用者がdeadになった場合は、sourceでもありtargetでもあるため、target.dead検査でスキップ。
死者sourceでも相手がaliveなら実行する190等との違いを維持する。

## 同じmaskでも結果が変わる例

以下はソースの分岐から導いた例であり、旧エミュレーターを実行して採取したfixtureではない。

### 192と187: RNG位置とHPが変わる

無属性ATK1をAからBへ使い、Bが192と187を選ぶ。最終防御属性は無属性となり適合し、両指輪はDEF0。
AはDarkCloudなし・HP40、命中prefixの次の値を76、後続応答を全て空とする。

| Bが選ぶcounter順 | 結果 |
| --- | --- |
| 192→187 | AにDarkCloudが付き、187は必中。hit RNGを消費せずAのHP39 |
| 187→192 | 187は76でmissしてRNG1回消費、その後AにDarkCloud。AのHP40 |

どちらも元被弾でBのHP39、消費枚数・配布予定は同じ。
最終状態だけでなく、抽選prefixの残りや次の行動結果も変わる。

### 191と190: 応答の選択肢が変わる

無属性攻撃へ191→190と選ぶと、先にGloryを受けたAは190への応答で単枚制限を受ける。
190→191なら最初の190へ複数普通防具や195との組合せを選べる。
この途中の意思決定を、最終harmの有無だけにまとめることはできない。

## 行動表現の提案

普通DFS/195 prefixの細かい順序を追加する必要は、現在のscopeではない。
全てのprefixを処理した最終残余rと最終防御属性を共通で使い、counter部分の列順だけをFIFOへ残せばよい。
特殊が発火した場合はcounter順にかかわらず全て予約しない。

異種counter列を既存の無順序maskや最終属性3kindへ圧縮する案は推奨しない。
後続の選択肢・死者判定・RNG位置まで順序に依存するため、属性setterの証明を流用できない。

新しいextendedActionを別ABIにする案:

- 既存の6byte意味部を保持する。
- counter_count:uint8、reserved:uint8、counter_order:uint64を追加し、計16byteのC標準layoutにする。
- orderは16個の手札slotを4bitずつ格納する。slotsのうちcounterに該当する物を重複なく全て列挙する。
- ordinary prefixはslotsの残り。未選択slot・非counter・重複・count不一致・未使用上位bitを事前拒否する。

旧gk等は6byte/190単種を維持し、新profileの別prefixだけ16byteを使えば互換性を保てる。
6byteを厳守するなら「異種counterの同時選択を禁止する」等のsubset制限を明記する。
単純なslot昇順固定は、旧版で選べる上記2結果を一方へ削ってしまう。

## MP回復とretained_deals上限20

このduelの現在のscopeでは、193を加えても1手番の保持奇跡によるgift上限20の証明は維持できる。

1. 元の主動作は1項目で、MAGICの跳ね返し連鎖が終わるまでcounterを起動しない。
2. 193をprefixとして210等へ繋げた場合は特殊が発火し、counter予約は発生しない。
3. 193がMPを回復するのは元項目の終了後。以後のキューは全てPROTECTOR counter。
4. PROTECTORには保持210/215を使用できず、同じ手番中に通常CASTも選べない。

従って、保持210による長いMAGIC連鎖の最中に193がMPを補給する経路はない。
連鎖中のMPは初期99以下から減るだけで、主動作の保持cast1回と、MP5以上の保持防御奇跡最大19回を合わせ20が保守的上限。
193がMP99へ回復しても、そのMPを使うのは通常は次の手番以降で、既存上限が既に初期MP99を許している。

将来、1手番へ複数の主MAGIC項目を積む全体・連撃、MAGIC型counter、反撃を戻せる万能反射等を入れる場合、
この単純な段階分離の証明を再確認する。MPを補給できるという理由だけで無条件に20を緩和することも、
そのような拡張後も20を無検証で維持することも避ける。

## コア注意と受入条件

1. damage/harm/manaとtarget方針を台帳で分離し、191/192をATK化しない。190単種の旧台帳を維持する。
2. 187はalive敵を予約時に選び、hitを起動時に判定する。死者source、死者target、敵0件を区別する。
3. directharmは普通DEF/195で効果を消さず、Heaven・Glory・DarkCloudの既存処理を維持する。
4. 193はsource=targetの自己回復で、死者ならskip、2r加算は上限99。prefix時にはMPを変えない。
5. 混合counterは指定列順で予約し、全counterの残余rは共通。特殊時不発・r=0不発・消費1回を維持する。
6. [192,187]と[187,192]のHP・forced・hit prefix差、および[191,190]による単枚制限の時点を検証する。
7. 防御中暫定HP0と項目間dead確定を区別し、死者targetへ抽選しても非公開結果を観測へ出さない。
8. 全counterは手札消費型で、duelの1枚から生成される項目は最大1件。queue capacity32とint64の保守的上限は維持できる。
9. retained_deals20の適用条件を検査し、旧13profileのAPI寸法・Action layout・乱数を保つ。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV#L188-L194
[defense]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L584-L603
[queue]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L269-L316
[hit]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L317-L380
[effect]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L445-L494
[player]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L94-L111
