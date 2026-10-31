# 旧版238 SUNDRY INCREASE_ATK

調査日: 2026-11-01。Igoorx非公式Flashサーバー公開masterの一次資料を対象とする。
v35は238単種の追加boostとして導入し、現行公式との完全一致は主張しない。

## 台帳と攻撃role

[CSV][csv]の238はSUNDRY、attackKind空、attackExtra=INCREASE_ATK、属性NE。
value0・subValue10・price15・weight2、MP費用/命中率/防御/他statusは0。
[Item.getAtk][item]はSUNDRYのINCREASE_ATKをsubValueから読むので、追加ATKは10となる。
subValue10を奇跡のMP費用10として誤読しない。

[isValidAttackItem][valid]は先頭SUNDRYかつattackKind空を拒否する。
したがって238の単独基底・USEは不可。武器ではないので、238を持つだけでは祈りを禁止しない。
手札の別slotである物理copyを、元hitRate0のWEAPONへ追加できる。
MAGICを基底にした追加や、元から％付きWEAPONへの追加は不可。
209で合成後の命中率が100になっていても、元基底の印字hitRate0なら後続238を追加できる。

[queueAttack][queue]は各238をusePieceで消費し、gift+1した後にgetAtkの10を加算する。
MP支払い・保持取得・追加抽選はない。複数copyは各10を加算するが、同slotを二重に使わない。
attack typeは先頭WEAPONのまま維持し、追加SUNDRYをMAGIC反応判定の型にしない。

## 倍化・属性・57との順序

普通のNE boostとして加算後に属性を合成する。火→238はNE、NE→238→火setterは火。
同じ選択集合でもsetterを最後にするか、238を最後にするかで結果が異なる。
順序付き列は既存ATTACK_ORDEREDで扱い、MP付き魔法がない選択集合なら既存setter結果kindでも表現できる。

基底ATK1なら、238→214は(1+10)×2=22、214→238は1×2+10=12。
209→238はWEAPON/率100のままATK11となり、duelでは生存敵へ対象を正規化する。
属性Lightの元攻撃へNE238を足せば通常合成でNEになり、Darkでも属性混合でNEになり得る。
属性setterによる上書きは既存の追加ATK非加算規則を保つ。

[57の再計算][queue]は**後続MAGICのMP支払い後、かつそのMAGICが有料**のときだけ起きる。
238自体はSUNDRYなのでMP再計算を発火させない。開始MP10の場合:

| 列（先頭57以降の追加順） | 最終威力 | 追加MP費用 | 最後に57が燃やすMP |
| --- | --- | --- | --- |
| 238→197 | (10−2)×2+2 = 18 | 2 | 8 |
| 197→238 | (10−2)×2+2+10 = 28 | 2 | 8 |
| 238→197→FREE | 10×2+10+2 = 32 | 0 | 10 |

有料197の前に加えた238は再設定で消える。無料197は再設定しないので前の238が残る。
238だけを57へ追加すると10×2+10=30、MP0なら0×2+10=10となる。

## FREEの隣接指定

FREE事前走査は最初の非先頭FREEの直前1品だけを無料指定する。
直前が238ならMP費用0の物理SUNDRYを無料指定しただけであり、さらに前のMAGICへ対象を移さない。
MAGICをまだ一度も使っていない列では、FREE自体の追加条件を満たさない。

例えば基底ATK1・初期MP10で、197→238→FREEはATK13・197費用2・saved MP0。
238→197→FREEはATK13・197費用0・saved MP2。
前者でも先に197を使っているので後続FREEは合法に消費され、giftを増やしてcontinueする。
238にはMP/保持効果がないため、FREEを追加して新たな保持種や無料castを作ることはない。

## 防御・型・売買

合成攻撃の被防御条件は、元WEAPON型・最終威力/属性・元chance/209来歴による既存規則に従う。
238を追加したことだけで魔法反射40・魔法跳ね返し210/物理6種・魔法遮断を発火させない。
20/39/215等の武器用反応は、最終属性・195・元％武器/209の既存条件で判定する。
反応後も合成済みraw payloadを維持し、238を再消費・再加算しない。

238自体にDFSや特殊防御roleはないので、このcanonical subsetでは応答に選べない。
ただし[旧defenderCommand][defense]には一般的な防御role検査がなく、適合するNE攻撃等へ
238をゼロDEFとして消費しgiftを得る入力を受理し得る。これは手札交換の戦略的意味を持ち得る。
この受理差を「効果がない」として隠さず記録し、UI/公式での受理が未照合のため今回採用しない。
既存profileが除外する他の非防御SUNDRY応答と同じ境界とする。

任意DISCARD、SELL商品、BUY抽選/購入の対象にはなる。売買価格は印字price15。
商品として動かすときは攻撃加算・MP支払い・保持取得を発火させない。
旧216種価格表は固定し、新flag時だけ238の価格15を独立fallbackに追加する。

## v35と受入条件

新Action/Effect/Pending/tape/holding/historyは不要。既存boost_slotsと攻撃extrasへ統合する。
新maskはmain手札238の候補だけで、response/purchase/doneでは0。
手札の238 membership観測は全phaseで本人にだけ見せる。
新step集計は攻撃extrasで実消費した238枚数であり、DISCARD/商品利用では増やさない。

boost10は既存の普通boost最大15を超えず、合成raw/32段counterの既存上界を拡張しない。
保持22種類・忘却順・16bit保持maskも維持する。旧34profileはflag=falseのまま台帳/ABI/RNGを固定する。

受入条件: 通常pool到達、単独/USE/元％/MAGIC基底拒否、祈り、通常/順序付き追加、複数copy、
setter/214/209/57の順序、FREE対象をすり替えないこと、被防御型維持、canonical防御除外、
売買価格15、実消費とgift、旧35profileの出力境界・バッチ原子的拒否。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV#L239
[item]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/item.py#L111-L126
[valid]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L78-L95
[queue]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L96-L289
[defense]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L510-L643
