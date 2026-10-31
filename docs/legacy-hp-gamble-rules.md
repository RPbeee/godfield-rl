# 旧版237 INCREASE_OR_DECREASE_HP

調査日: 2026-11-01。Igoorx非公式Flashサーバー公開masterの一次sourceによる。
現行公式との一致は別途確認が必要。今回の変更候補は正重量の237単種に限定する。

## 台帳と使用条件

[CSV][csv]の237はSUNDRY、attackKind=INCREASE_OR_DECREASE_HP、value10、price1、weight2。
属性・extra・防御・subvalue・hitRateは空、MP費用0。CSV内の同attackKindの正重量群はこれだけ。
通常の手札使用と任意破棄が可能で、奇跡保持リストへは入らない。

[queueAttack][queue]は所有変換・usePiece・gift+1の後、列長1を要求する。
boost/属性setter/追加197・217/DOUBLE214/WIDE209/FREE等を併用しない。
WEAPONを基底とする追加品にもならない（追加役割を持たない）。
[isValidAttackItem][valid]は先頭SUNDRYでもattackKindありなら許可する。
self/相手のどちらも指定でき、isActionによる強制self化はない。

## 抽選と応答前の公開

使用時に[queueAttack][queue]がrandrange(0,2)を一度呼ぶ。
0なら−10、1なら＋10の各50%をdecidedHPへ保存し、直ちにbreakする。
CSV値10と現sourceの固定定数10は一致するが、実処理はCSV値を可変振幅として読む実装ではない。

このbreakは通常の属性合成より前なので、AttackData.attribute=None・damage=−1・chance0を保つ。
[doAttack][attack]の初回COMMANDはcommandChain.hpとして符号付き結果を全体公開する。
従って相手は増減結果を知ってから196反射を選べる。使用前の未来の抽選値は非公開。
selfなら抽選の後に空応答で即適用され、結果もCOMMANDに載る。

命中率付き攻撃ではない。chance0の短絡でhit RNGを消費せず、DarkCloudでも
この独立した増減抽選を省略したり、常に回復へ変えたりしない。
環境へのfixture導入ならhiddenな0/1テープを用い、消費は使用時1回、反射時0回とする。

## None属性と反射

[checkDefense][item]はattackAttr=Noneを常に不適合とする。
通常防具・counter・195は不正で、[defenderCommand][response]の特殊最終検査を回避する
**先頭196だけ**を反射手段とする。SUNDRYなのでWEAPON/MAGIC限定の反射・跳ね返し・遮断は一致しない。
普通prefixの後の196はNone照合で不正。195はNoneの変更前assertで止まる。
196先頭後の未消費suffixは既存canonical応答契約に従い、Gloryの選択数規約も維持する。

[AttackData.clone][clone]はdecidedHPを保持する。196はcurrent source/targetだけを交換し、
増減符号を再抽選しない。反射往復中にHP効果を適用せず、最終空防御のcurrent targetへ一度だけ適用する。
通常合法経路には175等のprefixがないので、237の応答途中に新たな暫定HP0を作る経路はない。

## 効果・死亡・病気・giftの順序

[inflictDamage][effect]は負ならcurrent target.takeDamage(10)、正ならincreaseHP(10)を呼ぶ。
[Player][player]の下限0・上限99に従い、HP実変化は符号付き生値±10と分けて扱う。
例えばHP5に−10はHP0/実損失5、HP95に＋10はHP99/実回復4。
一般serverのtakeDamageは助手の肩代わりを経るが、助手は既存duel subsetの対象外。

damageフィールドが正でないため、通常攻撃のstatus付与・Dark即死・ABSORB_HP・
DAMAGE_TO_SELF・counter予約を重ねない。addHarmを呼ばず、Heavenでも増減だけでは転落しない。
病気・悪化counter・Glory/DarkCloud・MP/YEN・保持奇跡を変更しない。

[Room.beforeEndInning][room]は解決後、死亡/244の確認、元turn ownerの病気tick、通常giftの順。
HP5の人物が−10を受け、alive2かつ244ありならHP0から244を1枚消費して10へ復活する（gift0）。
自己使用ならその復活後に自分の病気tickが一度進み、そこで再びHP0になれば既存の死亡hookへ戻る。
相手使用なら病気tick対象は相手受益者ではなく、従来どおり元turn owner。
死亡確定した人物へ予定giftを配布しない。237使用のgift1と196実消費分のgift以外を追加しない。

## 最小導入案と受入条件

237単種なら新しい承認phase・保持型・複合列・queueを増やさず、既存None/先頭196経路で閉じる。
新effectと237来歴marker、使用時の符号付き結果、公知の解決イベントを追加し、
通常damageへ負値を押し込まない。signed payloadは±10だけをrestoreで許可し、
None/237/非counter/非miracle/非extras等の整合を検証する。
効果解決前の符号は公知だが、未来テープ・内部RNGを観測しない。

受入条件は、両符号とclip、先頭196往復で抽選/効果各1回、結果公開後の防御、self即解決、
DarkCloud非干渉、通常防具/195/型別特殊/複数使用/追加FREE拒否、Glory、
負値による244/元owner病気/gift順、旧profileの観測幅・乱数経路維持を含む。
117DYING_ATTACKは既確認のduel発火不能のため今回も対象外とする。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV#L238
[queue]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L110-L222
[valid]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L78-L95
[attack]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L317-L380
[item]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/item.py#L128-L147
[response]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L510-L643
[clone]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L58-L77
[effect]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L471-L494
[player]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L94-L111
[room]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py#L369-L474
