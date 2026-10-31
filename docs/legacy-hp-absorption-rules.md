# 旧版のHP吸収 ABSORB_HP

調査日: 2026-10-31。Igoorxの非公式Flashサーバーエミュレータ公開masterの一次sourceに基づく。
現行公式の完全再現とは区別し、v28候補は26/42/103/221の正重量4種とする。

## 台帳と基本効果

[旧CSV][csv]でABSORB_HPを持つ正重量神器は次の4種。全て重み1、defenseKind/defenseExtraは空。

| ID | type | attackKind | attackExtra | 属性 | ATK/value | 率 | MP | 価格 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 26 | WEAPON | ATK | ABSORB_HP | NE | 7 | 0 | 0 | 10 |
| 42 | WEAPON | ATK | ABSORB_HP | NE | 11 | 0 | 0 | 15 |
| 103 | WEAPON | ATK | ABSORB_HP | TREE/wood | 3 | 75 | 0 | 10 |
| 221 | MAGIC | ATK | ABSORB_HP | LIGHT | 10 | 0 | 10 | 15 |

[queueAttack][queue]はABSORB_HPをAttackData.extraへ追加する。
[inflictDamage][effect]は普通防御後の残余正を条件に、現在attackerへ残余全量を先に回復し、
その後現在defenderへ同量を適用する。回復は[Player.increaseHP][player]の99上限。
対象の残HP、実際に減ったHP、回復した実量で請求量をclipしない。
残余0以下、miss、遮断では吸収しない。最終DARKは別の即死分岐なので吸収を通らない。

例: sourceHP40、targetHP2、残余11ならsourceHP51、targetHP0。
sourceHP95なら99まで回復してからtargetへ11を適用する。
自己対象も回復→被害を省略せず、HP95/raw11は99→88となる。
sourceへmin(targetHP,残余)だけ回復する実装や、self時に何もしない実装は誤り。

## 分類・反射・死亡順

[AttackData.clone][clone]はextraとrawを維持し、[反射][reaction]はcurrent source/targetを交換する。
受益者を元turn_ownerや最初の使用者へ固定しない。
先行普通防具の減算は特殊反応時に元currentAttackのcloneへ戻るため消えるが、
prefixの消費・病気・195による属性消去は残る。
195は吸収来歴を消さず、相手が最終的に受ける正残余で吸収を計算する。

26/42/103はWEAPON、221はMAGICのまま。26/42のNEには20/39/215/196、
103は属性と元%規則（必要なら195）に従う物理反応、221には40/210/BLOCK_MAGIC/196が適用される。
221へ物理反応を使うことはできない。103の命中済み反射・221の反射で再抽選やMP再支払をしない。
103は75%の全alive敵攻撃なのでduelでは初回の指定selfも相手へ正規化され、初回self指定を自己吸収の到達例としない。
ただし195後の物理跳ね返しでcurrent source=targetになれば、自己吸収の順序で自動解決する。
221の自己対象は保証命中なので空応答で即時解決する。

正残余からの[counter予約][reaction]は原被害/吸収より前に起きる。
派生counterは反撃神器を基底とする新AttackDataで、原攻撃のABSORB_HPを継承しない。
吸収回復はharmではなく、Heavenを落としたり解除したりしない。
反射chain中のsourceがHP0・dead=falseでも、吸収が先に回復すれば死亡確定前に救われ得る。
同じitemのtargetへの被害まで終えてから244蘇生/死亡scan、残queue、owner病気tick、配布へ進む。
吸収でsourceが生きたかどうかを、回復前に確定してはいけない。

## 追加神器・型・属性の閉包

[isValidAttackItem][valid]はfirst WEAPONかつbase.hitRate0のときだけ、
INCREASE_ATK/DOUBLE_ATK/WIDE_ATK/ADD_ATTRIBUTEを追加に許す。
MAGICを使った後のMAGIC_FREEだけは別例外。

| base | 許される追加 | 吸収の扱い |
| --- | --- | --- |
| 26/42 | 普通boost、65/85、197/217、214、209、MAGIC後のFREE | 基底由来の吸収を完成payload全体へ維持 |
| 103 | 追加なし | 木75%の元payloadだけ |
| 221 | FREE carrierだけ | MAGIC/light/10を維持し、費用だけ免除可能 |

吸収武器自身や221は追加役割INCREASE_ATK等を持たないので、別baseのextraにはできない。
57MAGICALも追加役割として許されない。したがって現4種poolでは**57と吸収を同一攻撃に併用できない**。
57のMP再計算規則を吸収baseへ適用したり、57へ吸収武器を通常boostとして足したりしない。

26/42のordinary/ordered/属性setterを経た攻撃はWEAPONを維持する。
197/217を追加してもMAGIC反射の対象には変わらず、221の保持を取得することもない。
214は完成途中の威力を倍化し、209は率100/全敵化とNE化を行い、
いずれもextraリストに既にあるABSORB_HPを取り除かない。
FREEは直前MAGICの費用免除であって、吸収の重ね掛けや回復量追加ではない。
209を複数使ってもduelで吸収回数が増えるわけではなく、完成itemを相手へ1回解決する。

現poolの26/42はNEから開始し、ordinary LIGHTは合成の単位元、65/85のsetterは火/水、209はNE。
そのため最終属性はNE/火/水の範囲で、LIGHT/DARKには到達しない。
103は木（195後NE）、221は光（195後NE）である。
sourceのDARK吸収skip規則は一般則として保持するが、現scopeでDARK吸収fixtureを合法到達例にしない。
restoreにはabsorb来歴と57/不適切な属性・counter来歴との同居を拒否する根拠がある。

## 221の初使用・再使用・FREE

[Player.usePiece/useMagic][magic]は初使用で221を保持リストへappendし手札から除去する。
再使用は保持存在を確認してMP10を払い、保持copyは減らさない。
各実使用にdeal+1があり、初使用は手札1、保持再使用は手札0の消費となる。
FREE付き初使用/保持再使用はMP0でも可能だが、保持存在の確認は免除しない。
221を先頭に他の攻撃MAGIC・普通boost・setter・214/209を続けることはできない。
旧serverが無意味なMAGICを防御犠牲として受理する経路は既存subset同様に対象外とする。

手札240で未使用221を消しても保持は増えず、忘却241は取得済み221も抽選対象に含める。
全copyを忘れた後は、有料CAST/無料保持CASTとも拒否する。
別の手札221を後から初使用して取り戻すことは可能。

## 既存保持番号と忘却抽選の移行案

既存oldmain15配列（216/220＋13攻撃MAGIC）、防御2、追加2、214/209の番号と台帳hashを固定する。
221を古い13種sorted位置へ差し込むと、222/223/226のcast indexがずれるので避ける。
v28確定案は旧total_miracle_count=15/total_retained_miracle_count=17を固定し、
新main_ability_capacity=16でPlayer.abilitiesだけ末尾を1個増やす。
221にmain CAST base15/abilitymask bit15、FREE保持CAST base47（32+15）を割り当てる。
旧0..14、32..46は不変で、Action88byteやuint16 maskを拡張せずに済む。
spell/holding helperはindex15を221へ明示的に対応させ、旧13種台帳の範囲外参照を避ける。
Config.hp_absorptionはmiraclesとcelestialに依存し、offensive_miraclesは必須としない。
従って旧offensive=false時の有効main indexは0,1,15の非連続集合となり、単純なlength3ループにしない。
初使用の合計上限はoldmain＋防御＋221を合わせてmax_decisionsのまま扱える。

忘却は既存21種のcanonical ID順を保持し、221を220と222の間へ加えた22種を新profileで使う。
取得順・旧XML abilityIndex・Python同seedの結果一致を再現しない契約は従来どおり。
旧flag=falseでは221の所持を0に固定すれば、母集団総数・既存IDの累積重み・乱数消費は変わらない。
既存forgetting.hppの21種台帳を改変せず、追加catalog/新helper側に22種順を持たせると履歴を保てる。
観測も旧保持座標を動かさず、新suffixへ221所持数だけ追加する。

## 受入条件

- 正残余全量/HPclip非依存/99cap/完全防御0/self上限非相殺を検証する。
- 反射後current source、195、103の75%とmiss、221の光とMAGIC反応を区別する。
- 26/42の通常boost・setter・197217・214209・FREEで吸収来歴を維持し、57/吸収extraを拒否する。
- counterに吸収を継承せず、Heaven暫定HP0の回復と244より前の効果順を保つ。
- 221初使用/重複/再使用/MP10/FREE/忘却22種と、旧全index・ABI・分布乱数を検証する。
- 公開する回復eventは実量と要求量を区別し、未来RNGや相手保持全リストを追加観測しない。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[queue]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L226-L316
[effect]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L483-L495
[clone]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L58-L77
[reaction]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L511-L644
[valid]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L78-L94
[player]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L94-L111

## v28 API・公開観測

g28 ABI28は既存88byte/kind0..14/65th tailを維持し、mask27のword26を4種の手札membershipにする。
新obs1110は旧1085prefixを固定し、1085enabled、1086..1101手札local/4、
1102自分221保持/maxdec、1103pending吸収ID/363、1104最終event存在、
1105source==actor、1106target==actor、1107event神器ID/363、
1108最終残余/magic_free_damage_limit、1109実回復量/99を追加する。
残余0の吸収付きdamage解決もeventとして記録し、miss/blockでは新eventを作らない。
これは元XMLの完全再現ではなく、公知の解決量を明示した学習用観測契約である。

[magic]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L276-L301
