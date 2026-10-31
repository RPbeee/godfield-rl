# 旧版の自傷 DAMAGE_TO_SELF

調査日: 2026-11-01。Igoorxの非公式Flashサーバーエミュレータ公開masterに基づく。
現行公式の完全再現とは区別し、v29の追加は53単種とする。

## 台帳と適用順

[旧CSV][csv]で正重量DAMAGE_TO_SELFは53のみ。
WEAPON/ATK14、attackExtra=DAMAGE_TO_SELF、属性空欄/NE、price1、weight1。
MP・率・subvalue・defenseKind・defenseExtraは空。通常の武器baseで、boostや防具役割はない。

[inflictDamage][effect]の正被害処理は、最終DARKならtarget即死を適用して別分岐を終える。
それ以外は、吸収があればsource回復、自傷があればsource.takeDamage、最後にtarget.takeDamageの順。
自傷量は普通防御後の**正残余全量**で、source/targetの残HPや実HP減少で量を切り詰めない。
各takeDamageが[Player][player]のHP0下限を適用する。
残余0、miss、遮断では実ダメージを与えない。

| 条件 | source処理 | 続くtarget処理 | 最終状態 |
| --- | --- | --- | --- |
| 別人、source40/target2、残余14 | 40→26 | 2→0 | source26、target0 |
| 同一人物HP40、残余14 | 40→26 | 26→12 | HP12、実損失14/14 |
| 同一人物HP10、残余14 | 10→0 | 0→0 | HP0、実損失10/0 |

self aliasを1回の被害へまとめたり、最初の実損失10だけを次へ適用したりしない。
二つの被害の間に死亡確定/244蘇生を挟まない。
source側の自傷はharm付与ではなく、Heaven転落や病気解除を起こさない。
未対応のassistantによる被害吸収はこのduel subsetへ持ち込まない。

## 反応・counter・所有者

[clone][clone]はextraとrawを保持し、[反射/跳ね返し][response]はcurrent source/targetを変える。
自傷先を最初の使用者や通常turn_ownerへ固定しない。
39/196で返せば新sourceが自傷を受け、20でselfへ跳ね返れば同一人物へ2回適用する。
普通prefixの減算は特殊発火時に元rawへ戻り、prefix消費・病気・195属性消去は残る。
215遮断はsource自傷とtarget被害の両方を停止し、MAGIC用40/210/BLOCK_MAGICはWEAPON53に一致しない。

195は自傷来歴を消さない。53自体はNEだが、setter付き完成攻撃をNEへ戻して物理反応する場合も同じ。
[counter予約][response]は原攻撃の残余が正の時、被害適用より前に行われる。
派生counterは反撃神器から作る新AttackDataであり、53の自傷を引き継がない。
自傷で原sourceがHP0になっても、原target被害まで終えてから死亡scanと残queue処理へ進む。

## 追加MAGIC・FREE・属性の閉包

[初手/追加検証][valid]では、53は保証命中WEAPONなので普通boost、65/85setter、
197/217追加ATK、214倍化、209全体化を使える。MAGIC後のFREEも従来どおり許可される。
全て完成途中のraw/属性へ作用し、53のDAMAGE_TO_SELFは完成payloadのextraとして残る。
同じ完成itemで自傷回数を追加するものではない。
WIDEでもduelでは相手1人への完成itemであり、命中後に一度自傷と一度target被害を解決する。
反射時に命中を再抽選しない。FREEは直前MAGICの費用免除で、自傷免除ではない。
MAGICなしで53→FREEだけを選ぶことは追加検証が拒否する。

53と26/42/103/221は互いに有効extra役割を持たず、現poolで自傷＋HP吸収を同居させる合法列はない。
57MAGICALもextraとして許されず、53を57へ追加することもできない。
したがって53＋吸収markerや53＋mana57 markerを、source一般分岐の存在だけで合法fixtureにしない。

53の初期NEから、普通LIGHTは合成の単位元、setterは火/水、WIDEはNEへ作用する。
現poolの最終属性はNE/火/水の範囲で、DARKには到達しない。
一般則としてDARKでは自傷をskipするが、現scopeでDARK53の到達例を捏造しない。
53の初回自己指定は保証命中なのでそのまま自分へ空応答で解決する。

## 両者死亡・244・病気

[死亡処理][death]は両方の被害を終えたitem境界で固定seat0→1へ進む。
「sourceへ先に被害を与えた」ことは「sourceを先に死亡確定する」意味ではない。
双方HP0で244がseat0にだけあればseat0はHP10へ戻り、seat1がdeadとなる。
244がseat1にだけあればseat0が先にdeadとなるため、seat1の244は発動せず両者deadになる。
両者が持てば両者HP10。使用者がseat1でもこの走査順は変えない。

自己対象で2回被害を受けた場合も、最後にHP0なら1hookで244を1枚だけ消費してHP10へ戻す。
途中で蘇生して2回目の被害を受ける処理は誤り。
原item後に244で救われた者は後続counterのtargetになり、dead確定なら既存どおりskipされる。
source死亡済みcounter実行、193自動MP、194入出金、196のdeadtarget取消しは既存順を変えない。
全queue終了後に元ownerの病気tick、必要なら別の死亡hook、配布、終局へ進む。
手番owner以外の反射sourceに新たな病気tickを追加しない。
53や各追加成分の消費/配布予定は従来通りで、自傷によるMP費用・保持copyはない。

## 245/99との比較とv29契約

245MORTAR単体には発火効果がなく、99PESTLEとの組で対象検索・999raw・無臼時のself含む乱数を必要とする。
245の手動DISCARDは実消去をskipするがコマンドをrejectしないため、単に捨て不可maskを出すだけでは
旧serverの全意味を表せない。client/serverの応答可否も別検証が要る。
53単種は既存武器/追加/反応/死亡hookで閉じ、対象乱数や新保持番号を追加しないので先に採用する。
213種/重み427の新poolとし、旧28profileの配布系列とABIを固定する。

新g29 ABI29は88byte Action/kind0..14/65th tailを維持する。
Config.self_damageは独立。最小Configのdeadflag追跡は既存counters/revivalに従い、
両HP0終局をdraw/報酬0とする既存学習規約を新flagでも許す。
Pending.self_damage_card=53を完成・反射payloadへ保持し、counter派生では0にする。
公開eventはraw残余と、source部分/target部分それぞれのclip実量を区別する。
残余0の解決もeventとして記録し、miss/blockや未解決の反射中には新eventを作らない。

obs1135は旧1110prefixを維持し、1110flag、1111..1126手札53、1127pendingID/363、
1128event存在、1129source==actor、1130target==actor、1131eventID/363、
1132raw/magic_free_damage_limit、1133source実損失/99、1134target実損失/99。
mask28のword27はmain手札53membershipで、完全な合法手条件ではない。

受入条件は、残余/HPclip、自分2回・低HPclip別計測、反射source、prefixraw復元、215停止、
setter/195/197217/214209/FREE、吸収/57不正混在拒否、counter非継承、
双方HP0と244の4通り・seat1使用者・自己被害間で蘇生しないこと、旧ABI範囲とbatch原子性を含める。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV#L54
[effect]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L483-L495
[player]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L94-L111
[clone]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L58-L77
[response]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L511-L644
[valid]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L78-L94
[death]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py#L369-L494
