# 死亡時効果付き武器の通常使用 v37

`legacy-dying-weapon-subset-v37` は旧ID117を追加するduel限定profile。
CLI `dying-weapon` と `g37_*` は全拡張を有効にし、231種・旧重み合計485で動く。
現行公式の完全再現ではなく、旧非公式Flashサーバーを参考にした部分実装である。

[一次処理の調査](legacy-dying-weapon-rules.md)、[台帳と固定出典](../data/legacy-dying-weapon-v1/README.md)、
[前段の状態/解除奇跡](legacy-status-miracles-profile.md)を参照。
既存36台帳のhash・保持番号・旧216価格表を固定し、対象プール内で旧重みを正規化する。
公式の配布確率や現在の神器名/IDとは扱わない。

## 117の全metadataとduel制限

| 旧ID / local index | 種別 / 属性 | 通常ATK / 命中率 | 死亡時威力 | 価格 / 重み |
| --- | --- | --- | --- | --- |
| 117 / 0 | WEAPON / 光 | 1 / 75% | 30 | 10 / 1 |

原行は `WEAPON,ATK,DYING_ATTACK,,,LIGHT,1,30,75,10,1`。
subValue30は死亡時攻撃の威力であり、MP費用や追加ATKではない。
`DyingWeaponCard{Card,Element,hit_rate,dying_amount}` に両方の役割を保持する。
Cardのboost/DEF/HP/MPは0、手動DISCARD対象にはしない。売買や240強制除去は通常の物理品として扱う。

2人戦ではDYING_ATTACKの死亡時発動は到達不能である。原典の死亡走査は次の順に動く。

1. HP0でも既deadなら処理せず、not.dead人数が2以上かつ244所持なら、最初の244を消費してHP10へ戻しcontinueする。
2. 244で救われなければ当該人物をdeadにする。
3. その後、not.dead人数が2以上かつ117所持なら死亡攻撃を予約する。

`getAliveCount()` はHP正人数ではなくnot.dead人数を数える。
2席では手順2の直後にnot.deadは最大1となり、手順3の条件を満たせない。
244で救われれば手順3へ進まない。両HP0・複数244・自然病気・Heaven転落・反撃・新配送244でも同じ論証になる。
117所持者がdeadになっても117を自動消費/補充せず、死亡時用の30や75%を別途予約しない。

原典の3人以上戦なら死亡時raw30をforced FIFOへ積むが、このprofileはそのqueueを実装した主張ではない。
複数人戦へ拡張する場合は、死亡source・残存人数・全敵展開・勝敗処理を改めて実装する必要がある。
追加効果を剥がして一般カードとして扱うのではなく、duelでのみ到達不能とする範囲を固定する。

## 通常使用と既存の応答

通常ATTACKは手札117を1枚消費し、配布予定+1を得る。MPや保持化は無い。
75%武器なので指定targetを生存敵へ正規化し、duelでは相手1人へ1回だけ命中判定する。
対象がDarkCloud中なら既存規則で強制命中し、その命中乱数を消費しない。通常missでも開始eventは立つ。
追加boost/setter/MAGIC/DOUBLE/WIDE/FREEは付けられず、通常rawは1のままである。

命中後は光攻撃へ適合するDARK属性の通常防御で削減できる。
195を消費すればNE化し、その後の反応では命中率75%の来歴を保つ。
％物理に20/39を直接使うには既存の属性/率制限を満たす必要があり、195後ならNE反応が可能。
215の武器遮断も属性/率の条件に従い、先頭196ならそのまま万能反射できる。通常反撃も既存規則を使う。
MAGIC専用の反射/跳ね返し/遮断は117に発動しない。
反射/跳ね返しは残余前のraw1・命中済み・来歴を保持し、再抽選/追加支払はしない。
派生counterは独立項目であり、117死亡payloadを継承しない。
117自身を防具代わりのzeroDEF応答で消費することはcanonical対象外とする。
原典weakguardは適合するLight zeroDEF消費を受理し得て、手札交換に戦略的意味もあり得るが、
公式UI未照合で他の非防御WEAPONと同じ除外を維持する。

`Config::dying_weapon` はデフォルトfalseでcelestialを必要とする。
新Action/Effect/Pending marker・history・抽選tape・保持配列・死亡queueは増やさない。
`StepResult::dying_weapon_attack` は通常117の開始時だけtrueで、命中/ミス双方を含む。
売却/購入は物理117だけを移し、117の効果を使用しない。価格10を新flag時だけfallbackに加える。

## 観測・Snapshot・C ABI

共通C++観測は1368次元、Snapshot version37。
Snapshotは同じビルド・Config内のfixture用で、旧形式の永続checkpointを自動移行しない。
新 `g37_*` はABI37・1368特徴・41語mask・88byte行動で、kind0〜19/65参照/tail@17を維持する。
旧 `g36_*` は1345特徴/40語、g35以前も従来幅のまま。v36では末尾23特徴が0になる。

| 位置 | 内容 |
| --- | --- |
| 1345 | dying_weapon有効なら1 |
| 1346〜1361 | 自分の手札slotが117なら1 |
| 1362 | response中の命中済み117保留なら1。MAGIC/COUNTERは除外 |
| 1363 | LastChance.idが117なら1 |
| 1364 | 117の最後の命中判定がhitなら1 |
| 1365 | 117の最後の命中判定がDarkCloudでforcedなら1 |
| 1366 / 1367 | 117の最後のchance source / targetが現在actorなら1 |

1364〜1367は1363がtrueのときだけ値を持つ。LastChanceは解決後も公開履歴として残り、
反射後の現在source/targetと、初回命中判定のsource/targetを混同しない。
追加mask word40 `dying_weapon_slots` はmain手札117の所属で、response/purchase/doneは0。
既存attack maskや `legal()` と組み合わせ、％武器の追加不可を守る。
相手の未使用117・手札・保持・game RNG・未使用ticketを観測しない。

[新C API](../include/godfield/dying_weapon_c_api.h)と
[Pythonラッパー](../python/dying_weapon_env.py)の `DyingWeaponBatchEnv` を使う。
batchは全active行を検証してから更新し、不正な1行があれば全体を原子的に拒否する。
終了行は固定・報酬0で自動resetしない。報酬は固定座席順で、返却バッファーを保存する場合はコピーする。

```sh
python3 python/dying_weapon_env.py
./build/godfield_legacy_bench 100000 1 42 dying-weapon
./build/godfield_legacy_bench 100000 4 42 dying-weapon
```

ベンチは117の開始・hit/miss/forced・敵正規化、応答の195/反応/遮断/反撃、
dead確定時に残る117所持者/コピーと244蘇生時の117所持、物理売買/240除去を数える。
dead/蘇生の所持数はstep後手札を検査するので、同step配送や消費後の実在庫を表す。
命中時のraw1と応答中の再抽選無しを検算し、到達不能な死亡威力30を攻撃統計へ混ぜない。
checksum version37だけへ新event・LastChance・117手札/deadを混ぜ、旧36までの式/loopを維持する。

## 固定出典とv36観測修正

commit `fcc83740a3d3cd9968e6e5cd2b7ed781f53abe80` のCSV・room.py・turn.pyを
authenticated gh APIの固定refで取得し、[新manifest](../data/legacy-dying-weapon-v1/manifest.json)に出典とSHA256を保存した。
CSVは前段の[全364行raw archive](../data/legacy-status-miracles-v1/immutable-item-data.csv)を参照する。
旧36台帳のhashと非重複をgeneratorで検証し、過去manifestの取得事情は変更しない。

| 原典ファイル | SHA256 |
| --- | --- |
| ItemData.CSV | a6b6892d6234656c40d5171b042ce7aeefcd309ae1abcd6c3fa61e55c552f8b1 |
| room.py | ec16b265efe5af9a3a6509440cd1ed1ee5bdc8ac8bb2c59114426230394b75f8 |
| turn.py | bd25893f1e4b2a64466eb352947d920ace459cc5c691aaec89455a6d05d628b3 |

併せて、v36で208の手札属性欄285+slotがNEになっていた箇所を闇の6/6へ修正した。
新statusをelement_ofで参照する修正であり、208のPending属性/純DarkCloud効果は元から正しかった。
他4種はNEのまま、旧g36の固定1345幅にも修正を反映する。旧36ゲーム・乱数・checksumは変更しない。

Fog/Illusion付与10種・助手/守護神・MYSTERY243・終末・複数人戦、現行公式との動的照合は未対応。
合法手のランダム実行とベンチマークを利用できる。学習器と学習済みモデルは同梱しない。


## 検証結果（2026-11-01）

通常CTest114件、ASan/UBSan77件、37種類のPython suite計459テストが通過。
75/76の命中境界・DarkCloudの乱数非消費・自己指定の敵正規化・全extras禁止、
光/DARK防御・195/196・反撃・死亡/244優先・商品の移転/除去・原子的拒否・旧ABI幅を固定fixtureで確認した。
v36の208手札属性欄285+slotもnative/Pythonで闇の6/6になることを明示検証した。
新Python demoは2,048意思決定・58 episode・117攻撃3回で実行した。
既存36profileは各10万試合で、従来の全387項目の決定的出力が基準と一致した。
208の手札属性観測修正後もv36のゲーム軌跡・乱数・checksumは変わらなかった。
全機能有効・10万試合・seed42・意思決定上限512で、1/4スレッドの全403項目が一致した。
ASan/UBSanのRelWithDebInfoビルドでも1万試合の全403項目がReleaseと一致した
（checksum version37: `14032394066372018820`）。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 3,312,797 |
| 座席0 / 座席1勝利 | 49,559 / 49,632 |
| 勝者なし / 打ち切り | 809 / 0 |
| 117通常攻撃 | 5,256 |
| 命中 / ミス / DarkCloud強制命中 | 3,946 / 1,310 / 142 |
| 敵への正規化 | 5,256 |
| 117への応答意思決定 | 3,959 |
| 195消去 / 反射・跳ね返し / 武器遮断 | 76 / 15 / 1 |
| 117から予約されたcounter | 5 |
| dead確定時に117を持つ人物 / 残る117コピー | 1,759 / 1,777 |
| 244蘇生step後に117を持つ人物 | 22 |
| 売却商品 / 購入品 / 240除去の117 | 264 / 127 / 40 |
| checksum version37 | 886317902549404452 |

通常攻撃5,256が命中3,946とミス1,310に分かれ、強制命中は命中数に含まれる。
反射の往復があるため応答数は命中数を超え得るが、117の命中を再抽選しない。
dead後に残る117コピーは死亡時用に消費/配布/予約されず、244で救われた境界も通常死亡queueへ入らなかった。
