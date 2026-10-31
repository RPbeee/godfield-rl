# 旧版の所持金反撃194と、霧・幻覚の分離

調査日: 2026-10-31。Igoorxの非公式Flashサーバーエミュレーターをwebで読んだ結果。
公式の現行仕様を実測した資料ではない。可変`master`のため、台帳は取得物・hash・取得範囲を記録する。
v15は194だけを追加し、188霧・189幻覚は秘匿と行動表現を別途調査する段階へ残す。
予約と順序の基礎は[反撃メモ](legacy-counter-rules.md)と[拡張メモ](legacy-counter-extensions-rules.md)を参照。

## 194の正確なデータ

[旧CSVの0始まり行194][csv]と[Item.fromData][item]から確認する。

| 項目 | 値 |
| --- | --- |
| type / attackKind / defenseKind | PROTECTOR / ABSORB_YEN / COUNTER |
| attackExtra / defenseExtra / attribute | 全て空。属性は無属性 |
| value / subValue / hitRate | 空から0。通常ATK/DEFなし、MP消費なし、必中sentinel |
| price / weight | 10 / 1 |

武器や奇跡ではないためmainの攻撃には使わず、保持化・MP支払・保持再使用もない。
手札から通常DISCARDできる。

## 予約・対象・順序

[defenderCommand][defense]は普通DFS後の残余r>0で、特殊が発火しなかったときだけ指輪を列順に予約する。
194の`decidedValue`はr。sourceはcounter使用者、targetは現在のincoming sourceであり、自己MP回復193の分岐ではない。
実HP減少やtargetの所持金で予約値を小さくしない。

指輪の消費・配布予定+1は最初の応答時に一度だけ。
元の被弾を解決し、死亡確定・先行キューの後で194を起動する。起動時に再消費しない。
元攻撃でcounter使用者がdeadになっていても、[doAttack][hit]はsource.deadを理由に止めない。
target.deadなら項目をスキップし、入金も請求もしない。194は率0なので命中乱数を消費しない。

混合順序は既存の16byte Actionのcounter_orderを使う。
例えばtargetが金0/MP0/HP5、r=10なら[194,190]は194で入金後にtargetが死亡し、190はスキップ。
[190,194]は190で先にtargetが死亡し、194をスキップするため入金が起きない。
同じ無順序maskへ戻すことはできない。

## 入金が先、請求は全額

[inflictDamageのABSORB_YEN][effect]は次の順で処理する。

1. counter sourceへ要求額rを`increaseYen`で加算し、上限99へ制限する。
2. counter targetへ要求額r全額を`decreaseYen`で請求する。

実際にtargetが持つ金額だけを移す処理ではない。
sourceが上限近くでもtargetへの請求は減らず、sourceがdeadでもtargetがaliveなら先に入金される。
上限を超えた入金分は失われ、両者の資源総量が保存される保証もない。

[Player.decreaseYen][player]は金が足りなければMP、さらに足りなければHPへ不足を回す。
初期targetを金y/MPm/HPhとすると、以下と等価。

```text
金の減少 = min(y, r)
残り請求1 = max(0, r-y)
MPの減少 = min(m, 残り請求1)
残り請求2 = max(0, 残り請求1-m)
HPの減少 = min(h, 残り請求2)
```

| source初期金 | target初期 金/MP/HP | r | source最終金 | target最終 金/MP/HP |
| --- | --- | --- | --- | --- |
| 20 | 20/10/40 | 5 | 25 | 15/10/40 |
| 20 | 3/4/40 | 10 | 30 | 0/0/37 |
| 98 | 20/10/40 | 10 | 99 | 10/10/40 |
| 20 | 0/0/1 | 10 | 30 | 0/0/0 |

不足HPの処理は`takeDamage`ではなくHPを直接下げる。
守護神を含む将来の範囲で通常攻撃の吸収と同一視しない。現在の守護神なしprofileではその区別の結果は同じ。
int64の要求額をintへ狭めてから減算せず、不足額を広い整数で計算して最後に各資源へclampする。

## 防御と他効果

194の生成項目は通常ATKではない。普通DFSは応答として使えても、請求額rを減らさない。
195で無属性化しても請求は消えず、元から無属性なので属性も変わらない。
防御時病気・消費・配布予定等の副作用は通常どおり起きる。

請求によるHP減少から新たなcounterを予約しない。
counter予約は`atkData.damage>0`が条件だが、194のdamageは非ATKの初期-1で、普通DFSを使っても0になるだけ。
194への応答で別指輪を消費しても、請求でHPが減ることを理由に反撃しない。
HP不足で死亡したら項目終了境界でdead確定し、後続キューのtarget.dead判定へ反映する。

PROTECTOR由来なので20/39/210/215のWEAPON/MAGIC向け特殊は使えない。
未対応196の万能反射を暗黙に許可しない。
Gloryの単枚制限では194も1枚に数え、prefixとの合計で判定する。
反撃への防御を拒否する旧botとの差C05は、引き続きserver受理を採る限定profileとして記録する。

## 公開情報と状態表現

194は公開された反撃使用・要求額・対象を持つ。
旧[COMMAND XML][hit]も`decidedValue`を公開し、[Player.writeXML][player]は金・MP・HPを送る。
現在の霧・幻覚なしprofileなら、必要な資源変化を隠れた状態として扱う理由はない。
待機queueへ公開済みの要求額を出せるが、未来の乱数・未使用手札を出す必要はない。

単純な「targetへ金加算」のEffect.moneyと、source入金＋target請求の194は区別する。
新しい効果enumやcounter subtypeで明示し、normal220の処理を反転・共用して誤適用しない。
元のcounter_card/source/target/originと、旧profileの公開観測を維持する。
実際の入金量がrより小さい場合も、queueの要求額を入金量へ書き換えない。

[2026-09-20の第三者抽出][bible]のVenus Ringには所持金徴収の説明があるが、
それだけでは不足分のMP/HP処理・死者source入金まで現行公式と同じと確定できない。
これらの根拠は今回読んだ旧サーバーであり、現行完全再現の主張はしない。

## 188霧・189幻覚を分ける理由

### 霧188

[README][readme]は霧の不完全な実装を既知問題として記載する。
旧[queueAttack][fog]は霧の本人が他者を指定した通常行動で、敵/味方の区分に従って対象を取り直す。
forced counterにはこの再選択を適用しない。duelで敵候補が1人でも、表示上の秘匿が不要になるわけではない。

必要な別作業は、何を見せないかの公式/旧UI根拠の確定、各プレイヤーの知識状態・履歴、
ターゲット再選択と乱数消費、解除/再入室時の公開更新である。
wireへ送られた真値と画面で利用できる情報も同じとは限らない。
現在の真の相手資源をそのまま観測へ返して、霧flagだけ付ける実装は避ける。

### 幻覚189

[Player.dealItem][illusion]は本物の神器とは別に見かけの神器を保存する。
通常獲得の候補で半分の確率で偽物候補を探すが、別IDが見つからず本物の見かけのままの場合もある。
[Item.getIllusionForItem/isReplaceableBy][item]は種別・防御Kind・確率有無等の互換条件を使う。
同じ状態flagだけではなく、手札ごとの真値と表示値が必要。

[convertPiecesToOwnedPieces][mapping]で表示から所有実体へ戻し、その後は実体のデータで処理する。
単に偽IDを実IDとしてstepすると誤る。
逆に真のATK/属性/効果を観測へ返すと、幻覚を無効化してしまう。
互換条件があるから全ての真値由来マスクが安全、とも言えない。
例えばINCREASE_ATKとADD_ATTRIBUTEは相互候補になり得るため、真のsetter maskを出すと区別できる。
無属性counter193/194の表示と実体が替わる場合も、効果・対象の真値を露出しない設計が必要。

別作業は、手札の二重状態、表示ベースの候補構成と実体解決、合法性照会による漏洩防止、
解除/再配布/再入室の表示更新、同じ見かけで真値だけ異なる状態の観測同値テストになる。
霧・幻覚はどちらも単一counter行を増やす規模ではなく、観測・行動契約を見直す別段階として扱う。

## v15の受入条件

1. 194だけを追加し、188/189を配布・snapshot・合法候補へ入れない。既存16byte順序Actionを維持する。
2. r>0で要求rを予約、r=0/特殊時は不発。使用時消費・配布は一度だけ。
3. source先入金cap99と、targetの金→MP→HP請求を上表で確認する。
4. source.dead/target.aliveでは入金・請求、target.deadでは両方skipする。
5. 普通DEFや195で請求を減らせず、請求HP減少から追加counterを作らない。
6. [194,190]と逆順で入金と死亡skipが変わる局面を確認する。
7. int64の要求額・混合queue・Glory・retained_deals・元owner tick/配布を既存契約と両立させる。
8. 新観測は公開情報だけを追加し、旧14profileのAPI寸法・Actionサイズ・乱数・出力境界を保つ。

[csv]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV#L195
[item]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/item.py
[defense]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L584-L603
[effect]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L457-L464
[player]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py
[hit]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L317-L380
[readme]: https://github.com/Igoorx/godfield-flash#known-issues
[fog]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L85-L95
[illusion]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L166-L225
[mapping]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L71-L84
[bible]: https://github.com/BrandenXia/godfield-bot/blob/main/data/snapshots/2026-09-20/bible.json
