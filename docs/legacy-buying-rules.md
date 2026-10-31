# 旧版 BUY4 の抽選・購入承認

調査日: 2026-11-01。Igoorx非公式Flashサーバーの公開masterを対象とし、現行公式完全再現とは区別する。
SELL3の調査は[legacy-trade-rules.md](legacy-trade-rules.md)も参照。

## カードと起動

[CSV][csv]の4はTRADE/BUY、price5、weight20。他の攻撃値・属性・MP・率・extraは空。
[queueAttack][queue]は手札4の1枚だけを許し、実消費してgift+1、MP/保持取得なし。
早期breakで属性None・chance0となる。相手には応答を求め、自己指定は空応答で即解決する。
Noneへの通常防御・195・普通prefix付き196は不可、先頭196だけが返せる。
196往復中は抽選せず、最終的に空防御を受け入れた時点のcurrent target手札を抽選する。

## 抽選・公開・待機

[getRandomPiece][player]は空ならNone、それ以外はphysical copyの均等choice。
同種重複も別copy、1枚でもchoiceを呼び、245やMAGIC・価格による除外はない。
抽選では削除しない。4や反応196の消費が先なので、それらは候補に残らない。

[inflictDamage][resolve]は選んだ実CommandPieceをcurrentAttackにも保存し、
COMMANDのcommandChain.pieceで[商品ID等][piece]を全体公開する。
通常slot番号・母集団・乱数ticketは公開しない。商品のCSV価格はIDから判明する。

| 最終局面 | 次の処理 |
| --- | --- |
| self | 抽選結果の公開だけで終了。支払い・移転・承認なし |
| 非self、空手札 | 承認なしで終了 |
| 非self、買い手YENが商品価格未満 | 商品は抽選・公開するが承認なしで終了 |
| 非self、買い手YENが商品価格以上 | current source本人のYES/NOを待つ |

**価格0も非空なら承認待ち**。YEN0でも0以上なので、245の無料購入を自動成立させない。
抽選前に資金で候補を絞ったり、資金不足を理由に抽選を省いたりしない。
selfは[defenderCommand][response]のinflictDamage直後の早期returnがBUY待機判定より先にあるため、この例外となる。
196後は元turn ownerではなく入れ替わったcurrent sourceが買い手・承認actorである。

[Room.endInning][room]は入力待ちなら戻る。待機中にowner病気tick・死亡確定・gift配布・
手番交代を挟まない。既存counterを生成するdamageではなく、承認自体にも追加抽選はない。

## YES/NOの効果と商品同一性

[playerBuyResponse][approval]はcurrent source本人・BUY・選択商品あり・
current targetがその実copyをまだ所持、を確認する。同種IDの任意別copyを再抽選しない。
YESかつYEN十分のときだけ、次の順で処理する。

1. 売り手(current target)から選択済み実copyを削除。
2. 売り手へ価格全量を入金、99上限。
3. 買い手(current source)へ同IDのfresh copyを直接配布。
4. 買い手から価格全量を支払う。

資金判定はYENだけなので、正常成立時にMP/HP不足cascadeは発生しない。
売り手の99上限で入金が減っても買い手の支払額を減らさない。245も直接削除・購入できる。
[dealItem(...,True)][player]はIllusion抽選・通常DEAL通知を省くが、手札16なら失敗する。
汎用serverは失敗をrollbackしない。ただし本duelの買い手は4または196を先に消費して最大15枚なので、
承認時の配布失敗は通常到達しない。MAGIC商品はMP不要・保持化なしのfreshな手札になる。
商品削除・直接配布それぞれにgiftを追加しない。

NOは取引を実行しない。4と反応カードの消費・予定giftは取り消さない。
server関数はYESでも当日のYEN不足なら例外を投げず、未成立BUY結果を送って終了する。
正常な承認待ちには資源変更がないため、この再検査不成立は外部改変等なしには到達しない。
商品NoneではYES/NOとも先頭assertで不正となり、「空なのでNOする」という追加行動はない。
成立/辞退後に[Session][session]がnextInningを呼び、通常の終了処理へ一度だけ進む。

## 入力契約と移植時の境界

[Session.buyHandler][session]はdoBuy要素の**存在**を真とする。文字列falseでも存在すればYES。
nonce・商品ID・価格・承認phaseを入力から受け取らず、serverに保持したcurrentAttackを使う。
[User.parseXml][user]もXMLをSessionへ渡すだけで、別の購入nonce検査はない。
元serverの弱いcommand guardを再現する必要はなく、新環境では承認phaseでYES/NOだけを許す
有限のcanonical Actionを採る。商品選択後の再防御・別main行動・重複承認は原子的に拒否する。

承認中の保存にはcurrent source/target、選択実slot・ID・固定価格、抽選済みフラグが必要。
slot/母集団/ticketは内部情報として保ち、公知の選択ID・価格・承認actorと区別する。
相手の未選択手札を観測へ追加しない。選択商品は承認中に無変化だが、restoreはIDだけでなく
保存slotの実copyと一致することを検証する。

受入条件: 空/T1/重複copy抽選、資金不足でも公開、価格0承認、自己抽選だけ、196後actor、
YES/NOと重複入力拒否、245/MAGIC移転、売り手99cap、gift1/反応gift以外追加なし、
待機中のHP/病気/手番/RNG不変、拒否時のバッチ原子性、旧profileの観測幅とRNG維持。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV#L5
[queue]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L85-L156
[resolve]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L418-L422
[response]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L603-L651
[approval]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L50-L70
[player]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L205-L254
[room]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py#L426-L502
[session]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/session.py#L265-L273
[piece]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/commandPiece.py#L48-L57
[user]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/user.py#L61-L92
