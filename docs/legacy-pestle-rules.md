# 旧版の杵99 PESTLE・臼245 MORTAR

調査日: 2026-11-01。Igoorxの非公式Flashサーバーエミュレータ公開masterの一次source調査。
現行公式/Flash UIの完全再現とは区別する。serverが受理するduelの意味状態遷移を対象にする。

## 台帳・取得・消費

[旧CSV][csv]では99はWEAPON/ATK30/PESTLE/LIGHT、MP0・率0・price5・weight1。
245はSUNDRY/attackKind空/attackExtra=MORTAR、属性空欄/NE、全数値0・price0・weight1。
両方ともdefenseKind/defenseExtraは空。正重量なので通常の手札配布で取得する。
245は保持奇跡や設置物ではなく、手札内のphysical copyである。

[queueAttack][queue]は99を1枚だけの攻撃列に制限する。
普通boost・属性setter・追加MAGIC197/217/214/209・FREEを付けられない。
99の使用だけが手札1消費・deal+1となり、MP費用や保持化はない。
245は通常USE不可（SUNDRYのattackKind空）、攻撃extraやFREEでもない。
99の探索・被害解決・244蘇生により245を自動消費したりgiftを与えたりしない。

## 対象選択は最初に一度だけ

queueAttackはroom.playersの固定参加順（duelではseat0→1）で、not.deadの245所持者を探す。
最初の所持者と、その手札の最初の245を選び、targetをその人物、rawを999へ設定する。
245が複数あっても効果を加算せず、両者が持てば常にseat0を先に選ぶ。
検索はHPではなくdeadを使うので、HP0・dead=falseも検索対象、既deadの245は無視する。
選択したCommandPiece参照をmortarへ保存する。99の最初のrequested targetは上書きされる。

245が誰にもなければ[Room.getRandomAlive][alive]を引数なしで1回呼び、raw30とする。
候補はnot.dead全員で、me=Noneなので使用者自身も含む。敵だけの抽選ではない。
候補1人でもrandom.choiceの呼出しは省略しない。通常のduel主動作では両者aliveの2候補となる。
臼発見時には対象選択の乱数を消費しない。
その後に99本体の属性LIGHTを設定し、率0なので全敵攻撃にはしない。

[doAttack][attack]のchance0はPython比較の短絡により命中randを呼ばない。
DarkCloudの有無でこの保証命中の乱数消費は変わらず、新しいLastChance相当の抽選イベントも作らない。
Fogによる事前target変更はこのsubset未対応であり、requested target別のFog乱数まで再現する契約にはしない。

## 「防御不可」ログと実際のserver制御

**serverはmortar指定の他者raw999への応答を禁止していない。**
doAttackはselfだけ空応答で自動解決し、他者ならhumanの応答を待つか、botのonDefenseTurnを呼ぶ。
mortarがあるから自動で空防御を呼ぶ分岐は存在しない。

[defenderCommand][response]は通常どおり、選択品消費・195・特殊反応/普通DEF・counter予約・
inflictDamageを実行する。mortarによる早期returnは**特殊未発火の分岐の内部、被害適用後**にある。
そのためordinary防御のCOMMAND broadcastを省略してitem終了するが、被害適用前に防御を拒否するものではない。

### 原コードの空白数による確認

raw turn.pyのweb表示行番号は0始まり（GitHubの通常行番号は各+1）。
以下は取得テキストの行頭spaceを数えた結果であり、表示上の折返しから推定したものではない。

| web表示行 | 文の役割 | 行頭space |
| --- | --- | --- |
| L584 | 特殊未発火のif | 8 |
| L603 | inflictDamage呼出し | 12 |
| L604 | self判定 | 12 |
| L607 | mortar有無判定 | 12 |
| L609 | mortar時return | 16 |
| L611 | COMMAND builder作成 | 8 |

従ってmortar returnはnon-special ifの中であり、反射/跳ね返し/遮断では通らない。
特殊発火なら通常のCOMMAND/最終属性検査/次応答へ進む。
このserver挙動を、ログ文言やclientに防御UIが見えるかどうかと混同しない。
Flash clientが自動空COMMANDを送るか、選択UIを表示するかは、このPython sourceだけでは確定しない。

LIGHTなので元状態のNE防具は通常適合しないが、195でNE化すれば普通DEF/190等が使える。
196先頭は属性を免除して反射でき、195後なら20/39/215も分類に従って使える。
MAGIC専用40/210/BLOCK_MAGICはWEAPON99には一致しない。
counter予約自体もserver上は可能であり、botがcounterを避ける既存C05とは分ける。

## 反射・queue・死亡・244

[clone][clone]はraw・属性・mortar参照を維持する。
反射後に臼を再検索したり、原臼所有者と現在targetが違うから999を30へ戻したりしない。
195の属性消去は残り、特殊前の普通DEF減算は元rawへ戻る。
自己対象/跳ね返しselfは空応答で自動解決する。
臼由来LIGHT999はDARK即死ではなく通常の999被害なので、適合防御による減算やcounter残余計算がある。

普通被害を終えてから既存の死亡scan/244、queue pop、元owner病気tick、配布、終局へ進む。
245が残っていても死亡済み人物を蘇生する効果はない。
244が所持者を救えば245は残るので、次の99も同じ最初のalive所持者を狙う。
counterは新しい神器由来のAttackDataとなり、mortar/raw999の来歴を引き継がず、残余だけを倍率へ使う。
例えば195→190なら残余に従う反撃が、死亡済みsourceから生存targetへ実行され得る。
その反撃を196で既deadへ戻したときの取消しも既存規則どおり。

99は通常main攻撃なので、現在のduel subsetから既dead sourceが新しく99を使う経路はない。
forced sourceの汎用機構があるからといって、117やcounterを99へ変換してはいけない。
counter上界は最小profileでは少なくとも999<<32を含める必要がある。
full profileのmagic_free_damage_limitは999より十分大きいので、その既存上界は拡張不要。

## 245の手動破棄・強制除去

[DISCARD解決][discard]は245を実消去せずskipする。ただし選択コマンド自体をrejectしない。
245だけを選んでも、手札不変・gift0で手番を終えられる。
他の選択品と混ぜれば245以外の対象だけが消える。
Card.discardable=falseだけで245を候補maskから落とすと、このpass相当の行動を失う。

v30で採用する意味表現は、245をDISCARD選択候補に含め、実consume maskから245を除く方法。
新kindを増やさず、0消去でも通常のowner病気tick/手番終了へ進める。
一方、[240の強制除去][remove]はgetRandomPieceで選んだ実品を直接discardするので245も消える。
245は価格0だが、売買による移転は今回のscopeに追加しない。

## 公開XMLと秘匿範囲

[初回COMMAND][attack]はpiece99、commander.name、選択後target.name、mortarがあればそのitem245を書く。
raw999をdecidedValueに新設して送る処理はなく、mortarの存在と元99から効果を表す。
他者の臼所持がこの攻撃によって初めて公知になり得るが、他の245所持者や枚数・手札slotは載らない。
臼なしではmortar fieldがなく、ランダムに決まった実targetが公開される。

反射COMMANDは新targetと反応列を伝え、初回のmortar情報を再掲しない。
特殊未発火のmortar解決では前述の早期returnにより防御列broadcastを省略する。
Nativeで選択時の臼あり/なしと原対象を公知provenanceとして保存するのは意味契約とし、
相手の全手札・未選択の245枚数・将来の対象乱数を観測へ追加しない。
原mortar参照は取得時点の履歴なので、後で実手札から消えたことを理由に現在rawを再計算しない。

## 受入条件

99/245の2種を一緒に追加し、245だけの無意味な保持物にしない。
旧29profileはflag=falseでRNG・台帳・ABIを固定する。
臼0/1/複数/両seat/既dead/暫定HP0、raw30/999、self含む対象乱数と命中RNG無し、
requested target無視・追加神器拒否、245非消費・245だけDISCARDと混合DISCARD・240除去、
他者999への普通応答/195/特殊/COUNTER、反射後raw/mortar履歴維持、
244とowner病気・deadsource反撃・上界・公開provenance/相手残手札秘匿を確認する。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[queue]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L249-L286
[alive]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py#L92-L101
[attack]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L318-L380
[response]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L584-L644
[clone]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L58-L77
[discard]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L389-L396
[remove]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L423-L429

## v30確定APIと閉包条件

Config.pestle_mortarはcelestialへ依存し、新g30 ABI30は88byte Action/kind0..14/65th tailを維持する。
99は既存ATTACK kind1、baseは手札99slot、target0/1は両方受理して選び直す。
slots/extrasは0。245はDISCARD候補として選べるが、そのbitだけ実consumeから除外する。

obs1175は旧1135prefixを維持し、1135enabled、1136..1151手札99、1152..1167手札245、
1168pending杵、1169pending臼、1170最終選択event存在、1171初期source==actor、
1172初期target==actor、1173初期臼lock、1174初期raw/999を追加する。
mask30のword28/29はmain手札99/245membership。反射後のcurrent source/targetは通常pendingで表し、
この選択履歴を上書きしない。臼の全所持者、枚数、将来選択を追加観測しない。
新script_pestle_targetsは最大64個の0/1seatを渡す秘匿fixtureで、臼lock時には消費しない。

現duelの合法mainは両者not.deadなので、無臼で二者を均等選択する実装は原getRandomAliveと同値になる。
原sourceのsingleton呼出し規則は記録するが、既dead sourceのforced99など未対応経路のfixtureは作らない。
また既存subsetは無意味なSUNDRY防御犠牲を受理しないので、245をchain中に消費する合法Actionがない。
この閉包ではactive restore時に最初の臼所持者が初期選択と整合することを確認できる。
将来245防御prefixや新たなchain内強制除去を許す場合は、その整合条件を再検討する。
