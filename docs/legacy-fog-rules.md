# 旧版Fog: 確定するサーバー挙動と、未確定の表示秘匿

調査日: 2026-10-31。対象はIgoorxの非公式Flashサーバーエミュレータの
公開master。公式の現行仕様や、旧Flashクライアントの完全再現とは区別する。
v16への188追加は保留する。霧の対象変更だけなら実装できるが、学習者へ渡して
よい情報の範囲を一次資料から確定できていない。

## 根拠の強さ

| 資料 | 確認できること | 確認できないこと |
| --- | --- | --- |
| [旧README][readme] | 作者自身がFogの実装不完全と記載 | どの部分が不完全か、正解の秘匿仕様 |
| 旧Pythonサーバー | 状態変化、対象変更、送信XML | Flash画面上で実際に隠れる項目 |
| [公開client/game][client] | godfield.swfなどの配布物が存在 | ActionScriptの復元ソースは確認できていない |
| [公開client/scripts][scripts] | swfobject.jsのみを確認 | ゲーム内の霧表示ロジック |
| [現行公式サイト][official] | 公式の入口 | 今回のweb取得では本文を抽出できず、霧の項目別仕様は未確認 |

現行公式main.dart.jsは今回のwebツールではJavaScriptのcontent typeにより
取得できなかった。これは解析未完了という制約であり、公式に仕様がないという意味ではない。
第三者の攻略説明や現行client抽出物だけで、旧UIの秘匿範囲を確定した扱いにはしない。

## 対象の再選択と乱数

[queueAttack][queue]は、通常行動で、本人がFOGを持ち、指定先が自分以外のとき、
元の対象が敵か味方かに応じて生存候補を選び直す。自己対象とforced処理は対象外。
したがって188を含む反撃の予約自体はFogで対象が変わらない。

[Roomの選択関数][random]は候補リストにrandom.choiceを呼ぶ。敵は敵全員、
味方は本人を除く味方、判定はHPではなくdeadフラグ。確定死亡前のHP0も候補になり得る。
候補1名でも選択関数を呼ぶため、1対1で対象結果が同じでも乱数呼び出しは省略されない。
Python内部の乱数消費回数を固定1回と解釈してはいけない。

対象変更は神器の検証・消費より前。後から全敵向け％攻撃の展開や、行動の自己対象化、
TO_ENEMYの対象変更が起きるため、最終対象が同じでも途中の選択呼び出しがあり得る。
Fogが命中率を変更する分岐はない。1対1だけを理由に「霧の効果なし」とはできない。

## 188の来歴と解決

[台帳][csv]の188はPROTECTOR / ADD_HARM / FOG / COUNTER / WATER。
印字DEF・値・MP費用・命中率はいずれも0、価格10、重み1。

[防御処理][defense]では、通常防御後の残余が正で、反射・跳ね返し・遮断が
発火しなかった場合だけ、選択順でcounterを予約する。188の対象はその時点の
incoming source、sourceは防御者。188自身は通常属性合成へWATERとして参加する。
選択時に手札消費と配布予定+1、forcedでの予約時に再消費や配布追加はない。

予約後は元攻撃を解決し、死亡を確定してからFIFOを実行する。sourceがdeadでも
対象がaliveなら実行し、target.deadならスキップする。188は純ADD_HARMであり
ATKではない。実行時の普通防具で値を減らしてもFOG付与は消えない。
普通防御は最終属性適合が必要。PROTECTOR来歴なのでWEAPON/MAGIC限定の
反射・跳ね返し・遮断は使えない（万能反射196は今回の範囲外）。
純付与を受けたこと自体から、さらに残余ダメージ起動counterは発生しない。

## 付与・解除・死亡

[Player.addHarm][harm]では、通常のFOG重複は所持リストを増やさず、病気の
自然悪化カウンタも変更しない。ただしHeaven判定が重複判定より先であり、
HeavenへのFOGはHPを0にして悪化カウンタを0へ戻す。FOGを追加する分岐には進まない。
このHP0とdead確定の間は従来の反応・item終了規則に従う。

軽い解除と全解除の両方がFOGを除去し、pendingUnfogを立てる。
解除後の[unfog][unfog]は全員の公開状態と現在のattackerを本人へ再送する。
[beforeEndInning][finish]は死亡確定、幻覚解除通知、霧解除通知の順に処理し、
残queueへ進む。死亡時にも、瀕死攻撃などの例外を除き霧解除通知を予約する。
Fog固有の自然消滅・毎ターン抽選は確認できない。

## wire上の真値と、画面上の知識は別

[Player.writeXML][xml]はHP/MP/YEN、病気、harms、deadなどをそのまま書く。
[Room.addUser][enter]は霧にかかわらず全員のwriteXMLと現在のCOMMANDを送る。
[Session.sendXml][session]と[User.sendXml][user]にも霧による除去処理はない。
COMMANDには神器・source・targetなども含まれる。

したがって「旧サーバーは霧の本人へ真値を送る」は確定する。一方、それを
「霧でも人間には相手HP/MP/YEN・病気・行動が全部見える」と読み替えてはならない。
UNFOGで状態を再同期する設計はクライアント側の非表示・更新停止を示唆するが、
隠れる項目や、直前値の残存、応答中の例外、観戦・死亡後の挙動までは証明しない。
自分の手札や合法性をサーバー側で変換するFog分岐は確認できない。

既存観測では相手資源だけでなく、pendingの神器/属性/威力、LastChance、反応来歴、
FIFO内容、actor/source/target、合法mask/queryも情報経路になる。相手HPだけを0埋めしても
UIと等しい情報集合になったとは保証できない。0埋めする場合は実際の0との区別も必要。

## Fog源一覧と、今回の判断

[通常台帳][csv]の明示FOGは次の4種。防御時に即FOGを自分へ付ける普通DFS神器はない。

| ID | 分類 | 効果 | 属性 | 数値 | 価格/重み |
| --- | --- | --- | --- | --- | --- |
| 72 | WEAPON | 正の被害でFOG | WATER | ATK3、必中 | 10/1 |
| 102 | WEAPON | 正の被害でFOG | WATER | ATK3、50％ | 8/1 |
| 188 | PROTECTOR | 残余正で純FOG反撃予約 | WATER | DEF0 | 10/1 |
| 201 | MAGIC | 純FOG付与 | WATER | MP3 | 10/1 |

さらに243 MYSTERYのMERCURY結果は全員へFOGを付与する。
[守護神台帳][assistant]にもMERCURYのATK1・50％とATK2・必中にFOGがある。
武器の追加効果は正の被害時、201/188のADD_HARMは純付与であり、同じFOGでも
発動条件を混同しない。これらは188を将来追加する際にも自動でプールへ混ぜない。

最小実装を選ぶなら、次のいずれかを明示する必要がある。

1. 旧エミュレータのwire真値を学習者へ渡す独立profile。状態変化は再現可能だが、
   霧のUIや公式ゲームを再現したとは主張しない。
2. Flashの復元コードまたは再現可能な画面検証で秘匿契約を確定してから188だけ追加する。
   現行公式のUIを採用する場合も、旧server準拠との違いを明記する。

今回の推奨は2であり、188は未対応のまま残す。受入条件は、付与前後・解除直前後・
自己/他者の行動・攻撃応答・counter queue・死亡後について、各観測項目と合法手queryが
何を示してよいかを決めること。同じ知識の観測同値テストと、解除時の真値復帰テスト、
通常/forced/自己対象の乱数・対象テストを用意してから対応済みに数える。

[readme]: https://github.com/Igoorx/godfield-flash#known-issues
[client]: https://github.com/Igoorx/godfield-flash/tree/master/client-files/www.godfield.net/game
[scripts]: https://github.com/Igoorx/godfield-flash/tree/master/client-files/www.godfield.net/scripts
[official]: https://godfield.net/
[queue]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L85-L95
[random]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py#L92-L98
[defense]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L510-L643
[harm]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L147-L164
[unfog]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L176-L203
[finish]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py#L369-L449
[xml]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L302-L326
[enter]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py#L118-L177
[session]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/session.py#L47-L48
[user]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/user.py#L53-L59
[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV
[assistant]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/AssistantItemData.CSV
