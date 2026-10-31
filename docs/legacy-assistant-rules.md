# 旧版の助手: 導入前提と閉scopeの検討

調査日: 2026-11-01。固定commit `fcc83740a3d3cd9968e6e5cd2b7ed781f53abe80` の
取得済みassistant.py / AssistantItemData.CSV / player.py / item.py / attackData.py / room.py / turn.pyを一次資料とする。
現行公式との完全一致は主張しない。v37の231種485weightへ通常召喚242だけを加える案は、
**未対応の情報効果・生成神器へ推移するため、今回は採用しない**。
この文書は将来の実装条件を記録し、助手の一部分を実装済みにはしない。

## 召喚神器と使用時の抽選

通常[CSV][csv]のSET_ASSISTANTは2種。

| ID | type | MP | 価格 | weight | 保持 |
| --- | --- | --- | --- | --- | --- |
| 225 | MAGIC | 20 | 20 | 1 | 初使用で取得、保持再使用可 |
| 242 | SUNDRY | 0 | 5 | 3 | なし |

両方とも攻防数値/率/属性/他extraは0/空。
[queueAttack][queue]は使用消費/gift+1後、10typeから一様に選んだtypeをdecidedAssistantへ保存する。
自己/相手どちらも指定でき、isActionで自己へ上書きしない。結果はCOMMAND.commandChain.assistantTypeで応答前に公知。
抽選は使用時一度だけで、反射等で振り直さない。debug用forceNextAssistantが設定済みならそれを使って消すが、
通常のプレイヤー入力としてtypeを自由選択する規則ではない。

SET_ASSISTANTは早期breakのため属性Noneを保つ。
242には先頭196だけが反応でき、225は元MAGICなので魔法用反射/跳ね返し/遮断も使える。
普通防具/195はNoneへの通常条件では不可。
最終[inflictDamage][effect]が**targetをsummonerとした新しい助手**をHP20で代入する。
既存助手を置換し、残HPを継承せず、複数同時所持にしない。本人HP/MP/病気は回復・解除しない。

## FREEは他の単純奇跡と消費順が異なる

SET_ASSISTANTの列長は`1 + (先頭が無料指定なら1、そうでなければ0)`だけを許す。
225→所有済FREE1枚ならMP0・保持取得/gift1だが、最初のSET_ASSISTANTでbreakし、FREEは未消費。
この分岐へ既存の216等の「後続FREEを全消費」処理を流用してはいけない。
242→FREEも事前走査で先頭が無料指定となり列長2が通る。後続FREE自体の通常追加検査へ進む前に
breakするため、MPsaving0/未消費carrierの入力を旧serverは受理し得る。
複数FREEや任意boost等を足すと列長条件に反する。所有変換は列全体へ先に実施する。

## 助手の種類と42行の攻撃台帳

[Assistant][assistant]のtypeはMARS/MERCURY/JUPITER/SATURN/URANUS/PLUTO/NEPTUNE/VENUS/EARTH/MOON。
最初の8typeは[専用CSV][attacks]に5行ずつあり、記載順の重みが6/5/4/3/2（合計20）。
EARTHとMOONは各1行・重み1を持つが、通常は専用の生成処理へ進む。

| type | 専用行の内容（記載順） | 未実装/追加前提 |
| --- | --- | --- |
| MARS | 火ATK2/3/4/5/6、全75% | 助手由来の確率武器 |
| MERCURY | 水ATK1+Fog50%、ATK2+Fog必中、ATK2 50%、ATK3必中、ATK6 50% | Fog |
| JUPITER | 木ATK1/2必中、ATK1+HP吸収75%、直接Illusion、ATK2+Illusion75% | Illusion |
| SATURN | 土ATK3/5/7/10/16、全必中 | 助手由来の通常武器 |
| URANUS | 光ATK2必中、ATK4 25%、直接Glory、ATK5+HP吸収必中、ATK10 75% | 公知Gloryの助手来歴 |
| PLUTO | 闇ATK1+DarkCloud25%、ATK1/2/4/8必中 | 闇/状態の助手来歴 |
| NEPTUNE | 全解除、MP+5、HP+5、MP+10、HP+10 | 助手由来のutility |
| VENUS | 全員YEN+1、敵YEN+5、YEN吸収3、YEN+8/20 | SCATTER_YEN/非counter吸収 |
| EARTH | ADD_ITEMの補助行 | 全通常神器の生成/取引 |
| MOON | NE ATK10の補助行 | 全MAGICの再抽選/追加魔法 |

専用行は[Item.fromAssistantData][items]でtype=WEAPONとして作られる。
行IDはtypeごとの0..4で、通常神器の物理ID0..4と衝突する。
`assistantType + 専用local index`という来歴namespaceが必要で、物理IDだけへ押し込まない。
回復/解除/入金も専用行ではWEAPON型であり、通常MAGICの型反応へ変換しない。

## 発動手番とqueue

[Room.beforeEndInning][room]は、死亡処理と既存queueを完了し、元ownerの病気tickを済ませてから
助手stageを一度だけ行う。ゲームが既に終了条件ならこのstageを行わない。
元ownerの**敵**でdeadでなく助手を持つ人物ごとに、randrange(100)<30で攻撃機会を発生させる。
自身の助手が自分の手番末に攻撃する規則ではない。

人物2席なら敵助手候補は最大1人だが、相手へ召喚した242はその手番末に新助手が直ちに攻撃し得る。
stageフラグを生成前に立て、生成攻撃/FIFO/counterを解決してもowner病気と助手stageを繰り返さない。
全項目の後に通常giftを配布する。召喚/助手攻撃への応答待ちやBUY承認中に先へ進めない。

生成は[queueAttack(forced=True)][queue]で、実手札の所有変換・消費・保持取得・MP支払い・giftを省く。
攻撃sourceは助手のsummoner、元turn ownerとは別になり得る。
通常被害後のcounterはraw残余から発生し、助手が後で壊れたりsourceが死んでも既存FIFOに残る。
target.deadのgenerated itemをスキップする経路と、公知/非公知の命中結果を分ける必要がある。
助手はPlayerを増やす実体ではなく、人物2席と117死亡hook不発の人数証明を変えない。

## 対象決定と追加乱数

[decideAttackTarget][assistant]は必中ATK/SELL/BUY/吸収/直接状態/除去をランダム生存敵へ、
回復/MP/入金/解除/ADD_ITEM/召喚を本人を含む生存味方へ指定し、それ以外は本人とする。
duelで敵/味方候補が1人でもchoice呼出しは行う。％ATKは後の全敵展開で敵へ向かう。
VENUSのTO_ENEMYは一旦味方指定後、queueの追加効果で再び敵を選ぶ。
命中/型反応の意味は、通常カードと同じ元type/属性/率へ従う。

召喚type・30%機会・対象はPython random、重み付き専用行/通常神器はItemManagerのNumPy randGenを使う。
呼出し順・分布と、元Pythonとのseed完全一致は別の契約として区別する。
将来の機会/行/生成物/対象/命中を先にpolicyへ見せてはいけない。

## HP20の肩代わりと貫通

[Player.takeDamage][player]は先に助手HPを減らし、超過分だけ本人HPへ通す。
ちょうど助手HPぶんの被害なら本人損失0で助手を消す。助手の自然回復/MPはなく、再召喚でHP20に戻る。
通常被害、53の各被害、237の負結果、病気tick、Dark999、Heaven転落999はこの経路を通る。
counter予約とHP吸収の量は、本人が実際に失ったHPではなく盾前のraw残余のまま。
本人が同じ53自己targetなら肩代わりを含め2回順に処理する。

decreaseYen不足の本人HP減算とEXCHANGEのHP代入はtakeDamageを通らず、助手を貫通する。
従って本人HP0でも助手HP20が残り、244で本人だけ復活する状態が到達し得る。
死亡処理は助手を一般的には消さず、dead人物の助手は攻撃機会の対象から外れる。
本人HPの増加は助手HPを回復しない。
現coreの被害/tick/履歴restoreは「被害なら本人HPが減る」前提を持つため、盾による本人損失0へ拡張が必要。

## EARTH/MOONが閉scopeを破る理由

EARTHは**全通常台帳の元重み**で神器を1枚生成する。
非MAGICのATK/BUYならその行動、SELLならさらに商品1枚を生成して取引する。
それ以外はADD_ITEM補助行＋生成物として本人等へ直接配布する。Fog/Illusion武器、225/242/243も生成対象となる。
手札16でも生成BUYが起こり得るため、通常BUYの「買い手が4/196を先に消費して最大15枚」という証明が使えない。
満杯配布失敗でも元品削除・売り手入金・買い手支払いをrollbackしない経路が新たに到達する。
生成57はsource MP×2ではなく、助手特例でraw200（表示costMP100）となり実MPを消費しない。

MOONは全通常台帳からMAGIC以外と210/215を再抽選して除外する。
197等の通常MAGICを無料/保持取得なしで実行し、209/214ならNE補助ATK10＋追加魔法として生成する。
201Fog・213Illusion・225召喚も選択され得る。225はさらに10type抽選へ再接続する。
typeを安全な6種へ絞る、専用行からFog/Illusionだけ削る、通常生成を既存231種へ制限する、
助手を盾だけにする、のいずれも元242の一様召喚分布や後続状態遷移を改変する。

## 公開情報・導入前提

[Player.writeXML][player]は助手のtype/HPを公開し、COMMANDはassistantTypeと選択済み攻撃を公開する。
Fog等のwire真値と旧Flash画面の秘匿は従来どおり未確定であり、wireをそのまま公式のpolicy観測と断定しない。

正確な通常242導入には、Fog/Illusionの情報契約と付与・解除、全生成神器と助手専用行のnamespace、
肩代わり全被害経路、助手stage/生成queue/取引承認、保持不要のforced magic、
再召喚/死亡/244、rawと本人/助手実損失の観測・restoreを合わせて設計する必要がある。
固定MARS等の条件付き実験profileは作り得るが、通常の242そのものの再現と表示しない。
今回はv37の確定scopeを維持し、助手は情報契約を確認した後へ保留する。

## 固定source照合

取得本文のSHA256（実装本文はこの文書へ複製しない）:

| source | SHA256 |
| --- | --- |
| assistant.py | `d4c21da779e57cc9bb0535ab6429f0d8631b216733bfbf335daead96c21e722b` |
| AssistantItemData.CSV | `9475086efac418358ac24a3739cd1301eac9c6fdfc72edc8be5a7a49d89f14bf` |
| player.py | `b658cbef9fad3ba74821d5c997411b5ec7ce0f3d44741e1bd5361d779dcd692f` |
| item.py | `4f36e1062db3f02b52b3600a69baf3c4446f61224fc9175b2ccc7e4416abfe80` |
| attackData.py | `404f55170cfad0e16b7223cf90d009e1af3ae07463494de54ba04e7808074e34` |
| room.py | `ec16b265efe5af9a3a6509440cd1ed1ee5bdc8ac8bb2c59114426230394b75f8` |
| turn.py | `bd25893f1e4b2a64466eb352947d920ace459cc5c691aaec89455a6d05d628b3` |

[csv]: https://github.com/Igoorx/godfield-flash/blob/fcc83740a3d3cd9968e6e5cd2b7ed781f53abe80/server-src/ItemData.CSV#L226-L243
[assistant]: https://github.com/Igoorx/godfield-flash/blob/fcc83740a3d3cd9968e6e5cd2b7ed781f53abe80/server-src/modules/assistant.py
[attacks]: https://github.com/Igoorx/godfield-flash/blob/fcc83740a3d3cd9968e6e5cd2b7ed781f53abe80/server-src/AssistantItemData.CSV
[items]: https://github.com/Igoorx/godfield-flash/blob/fcc83740a3d3cd9968e6e5cd2b7ed781f53abe80/server-src/modules/item.py#L48-L63
[queue]: https://github.com/Igoorx/godfield-flash/blob/fcc83740a3d3cd9968e6e5cd2b7ed781f53abe80/server-src/modules/turn.py#L93-L338
[effect]: https://github.com/Igoorx/godfield-flash/blob/fcc83740a3d3cd9968e6e5cd2b7ed781f53abe80/server-src/modules/turn.py#L407-L527
[room]: https://github.com/Igoorx/godfield-flash/blob/fcc83740a3d3cd9968e6e5cd2b7ed781f53abe80/server-src/modules/room.py#L462-L523
[player]: https://github.com/Igoorx/godfield-flash/blob/fcc83740a3d3cd9968e6e5cd2b7ed781f53abe80/server-src/modules/player.py
