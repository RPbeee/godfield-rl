# 旧版の魔法反射4種

調査日: 2026-10-31。対象はIgoorxの非公式Flashサーバーエミュレータ。
v16は40/171/174/178だけを追加する。公式完全再現ではなく、
Fog188と、選択順で万能性が変わる196は未対応のまま残す。

[旧台帳][csv]では40がNE武器ATK10、価格15・重み1。
171/174/178はNE防具DEF9/10/11、各価格10・重み1。
全4種のdefenseExtraはREFLECT_MAGIC。他の攻撃追加効果、MP費用、保持化はない。

[defenderCommand][defense]はincoming先頭神器のtypeがMAGICなら反射を発火し、
currentAttackのsource/targetを入れ替える。通常DFSの処理より先にbreakするため、
171など自身のDEFはその魔法を軽減しない。発火を任意に省略する経路もない。
APIではMAGICへ使う4種をdefend.baseに指定し、普通slotsとしての選択を禁止する。
非MAGIC入力なら171/174/178は通常のDEF防具、40は防御用途を持たない武器となる。

前置防具は消費され配布予定を増やすが、反射後は元currentAttackのcloneに戻るため
威力の減算が失われる。195の属性消去は元currentAttackも書き換えるため反射後に残る。
前置175による病気付与など、先行して実状態へ適用した効果も残る。
本profileで既存採用しているGloryの1枚制限を適用し、前置防具と反射の2枚選択を許さない。

特殊後の属性・DARK・counter確認はMAGIC入力ならskipされる。
したがって四属性・LIGHT・DARK・％攻撃魔法だけでなく、216回復・220入金なども
反射でき、前置防具の属性適合も不要。PROTECTOR来歴のcounterには発火しない。
純回復などは反射で量や効果を変換せず、そのまま新targetへ解決する。

反射は[doAttack][attack]を呼び直さず、新targetにもう一度応答を求める。
命中済み％魔法の再抽選、MP再支払、保持奇跡への追加は発生しない。
反射元が新sourceとなり、元の攻撃者が新targetとなる一方、通常手番のownerは維持する。
反射返しにも同じ規則を使い、解決後の死亡・FIFO・owner tick・配布は既存順序に従う。

受入確認は通常プールから4種へ到達すること、MAGIC時の強制発火と普通slots禁止、
非MAGIC時の印字DEF、raw威力・195・Glory・暫定HP0・％命中の保持、回復/入金の
対象逆転、旧15profileの固定観測境界を含める。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[defense]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L510-L643
[attack]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L317-L380
