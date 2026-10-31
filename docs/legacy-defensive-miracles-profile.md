# 防御奇跡と保持17種 v10

`legacy-defensive-miracles-subset-v10` は防御奇跡2種と、保持からの防御応答を追加する限定profile。
CLIの `defensive-miracles` と `gb_*` はv10までの拡張を有効にし、172種・旧重み合計360で動く。
旧非公式エミュレーターを参考にした実装で、現行公式の完全再現ではない。

## 対象と出典

[旧版の防御処理の調査](legacy-defensive-miracles-rules.md)と
[台帳・出典・指紋](../data/legacy-defensive-miracles-v1/README.md)を参照。
旧ID・旧数値で扱い、既存9台帳は変更しない。限定プール内で重みを正規化し、公式の出現率とは扱わない。

| 旧ID | local index | 効果 | MP | 旧重み |
| --- | --- | --- | --- | --- |
| 210 | 0 | MAGIC由来の効果を跳ね返す | 5 | 1 |
| 215 | 1 | 無属性武器を遮断する | 6 | 1 |

210は攻撃奇跡だけでなく、相手対象の回復216・所持金220も跳ね返せる。
光・闇・確率付き奇跡も対象。通常のHP/MP回復アイテムや薬など、奇跡でない効果は対象外。
215は今回の通常合成で最終無属性となった武器攻撃だけが対象で、闇や4属性・光のままの武器と奇跡には使えない。
旧版の型判定は命中率を制限しないが、今回の台帳に無属性の確率武器は存在しない。

## 行動・MP・保持・補充

両方とも応答専用で、通常手番のuse/castには使えない。手札から捨てることはできる。
初使用ではMPを支払い、その手札1枚を消費して防御奇跡の保持数を1増やす。
保持からは同じMPを払い、手札と保持数を減らさず再使用する。どちらも配布予定+1。
既存15種の保持slotは変更せず、防御2種を別配列・別maskに分ける。合計17種類を保持できる。

| 操作 | kind | base | slots | target |
| --- | --- | --- | --- | --- |
| 手札の防御奇跡 | defend=2 | 手札位置 | 先に使う通常防具のmask | 255 |
| 保持からの防御奇跡 | defend_cast=6 | 防御local index 0/1 | 先に使う通常防具のmask | 255 |

slotsは手札の位置、保持baseは種類indexなので、同じ数字でも別の意味を持つ。
Glory中は先行防具と特殊応答を合わせて1枚まで。保持使用も1枚として数える。
この制限は旧botを根拠に採用しており、旧サーバーの入力受理との差は残す。

防具prefixの消費と病気は先に処理する。跳ね返す量は通常防具で減らした残余ではなく、元の効果量。
210を使う応答は最終属性検査を省くため、その攻撃を普通には防げない属性の防具もprefixへ入れられる。
防具だけの応答には通常の最終属性検査が必要。この違いを消すために候補maskを狭めない。
遮断では本体のダメージと被弾時付与を止めるが、先に使った防具の病気まで取り消さない。
手番末の自然tickは元の通常手番所有者に1回だけ適用する。

## 非攻撃への防御と反応連鎖

v10では回復・MP増加・所持金増加・解除への応答にも通常防具を使える。
旧版はそれらの効果量を元の神器値から適用するため、防具を使っても回復量などは減らない。
防具を使うだけの応答でも同じ。消費・配布予定・防具の病気は通常どおり発生する。
この拡張はv10だけで有効にし、既存profileの合法手を変更しない。

跳ね返し先は2人の生存未確定の座席から抽選し、自分へ戻れば即解決、相手へ向けば次の応答を待つ。
元の奇跡ID・初使用/保持使用・命中率を保ち、攻撃MP・初回保持・命中判定を繰り返さない。
命中イベントの座席は元の記録のまま、現在の効果source/targetだけが入れ替わる。
奇跡を保持する元の使用者はturn_ownerであり、跳ね返した人物がその攻撃奇跡を保持しているとは限らない。

保持からの応答は手札を減らさないため、配布予定は16を超え得る。
`Player::retained_deals` で保持使用による配布予定を別に数え、連鎖終了まで補充しない。
満杯への配布も抽選は消費し、追加分を捨てる。死亡確定した座席へは配らない。
手札だけから導いた32回の連鎖上限は設けず、MP支払と意思決定上限を扱う。
今回のMP上限99・防御MP最小5では、攻撃奇跡後に両者で38回の保持跳ね返しが可能。

HP0と死亡確定は分ける。Heavenの人物が防具175で暫定HP0となった後に210を使い、
回復奇跡を自分へ跳ね返して、死亡確定前に回復する旧サーバー経路も扱う。
旧クライアントUIで選択可能だったかは未検証。任意のHP0から通常手番を始められる規則ではない。

## C++・観測・API

```cpp
godfield::legacy::Env env({512, 10, true, true, true, true, true, true, true, true, true});
env.reset(42);
```

末尾の `Config::defensive_miracles` はデフォルトfalse。有効化にはoffensive_miraclesとreflectionが必要。
`Player::abilities` の15枠とmainの `Options::ability_slots` は維持する。
新しい `Player::defensive_abilities[2]` と `Options::defensive_ability_slots` はlocal index順。
手札の防御奇跡は既存のreaction_slotsに含める。
defense_slotsには210のprefix用の不適合防具も含み得るので、maskから構成した行動は `legal()` で検証する。
Snapshotの共通versionは37。同じビルド・設定のfixture用で、旧形式を自動移行する永続checkpointではない。

`StepResult::defensive_first_use/defensive_recast/magic_bounced/weapon_blocked` を追加。
既存spell_first_use/recastは防御を含む全17種の使用を表す。
`LastReaction::from_ability` は最後の特殊応答が保持使用だったかを記録する。
遮断のLastReactionはID215・kind=block・source=target=防御者とする。
これは本C++環境の公開イベント表現で、元の攻撃のsource/targetを遮断で入れ替える規則ではない。

共通C++観測は1368次元、`gb_*` ABI 10は先頭356次元を返す。
v10では末尾1012個が0になる。追加部分は[属性付け替えprofile](legacy-attribute-change-profile.md)と[属性消去profile](legacy-attribute-removal-profile.md)と[反撃profile](legacy-counter-profile.md)と[反撃拡張profile](legacy-counter-extensions-profile.md)と[所持金反撃profile](legacy-money-counter-profile.md)と[魔法反射profile](legacy-magic-reflection-profile.md)と[万能反射profile](legacy-any-reflection-profile.md)と[MP武器profile](legacy-mana-weapon-profile.md)と[魔法遮断profile](legacy-magic-block-profile.md)を参照。先頭334個は[攻撃奇跡profile](legacy-offensive-miracles-profile.md)を継承する。

| 位置 | 内容 |
| --- | --- |
| 334 | defensive_miracles有効なら1 |
| 335〜336 | 自分の防御2種の保持数 / 意思決定上限 |
| 337〜352 | 自分の各手札位置の防御奇跡種別。なし0、210は0.5、215は1 |
| 353 | 最後の特殊応答が保持からなら1 |
| 354 | 最後の特殊応答が遮断なら1 |
| 355 | 現在の連鎖回数 / 意思決定上限 |

手札の奇跡flag・MPコストは既存132〜163を使う。相手の手札・完全な保持数と、未来の抽選値・乱数状態は出さない。
既存227の連鎖/32という特徴はその定義を維持するため、v10では1を超え得る。
全観測が一律0〜1と仮定せず、新355も利用する。新拡張が無効のprofileでは追加22特徴は0。

[defensive_miracles_c_api.h](../include/godfield/defensive_miracles_c_api.h) の `gb_*` と
[DefensiveMiraclesBatchEnv](../python/defensive_miracles_env.py) を使う。
6byteの `gl_action` を維持し、新ABIだけkind6を許可する。mask幅は8。
先頭7種類は従来と同じで、8番目が保持から使える防御種類の2bit mask。
旧 `gl/gm/ge/gc/gr/gd/gh/gs/go` の128/164/186/210/236/258/283/301/334次元とmask幅は維持する。
バッチの原子的拒否、固定座席報酬、終了行固定・自動resetなしを継承する。
Pythonバッファーは再使用するので履歴へ保存する際はコピーする。同じハンドルを複数スレッドから同時操作しない。

```sh
python3 python/defensive_miracles_env.py
./build/godfield_legacy_bench 100000 1 42 defensive-miracles
./build/godfield_legacy_bench 100000 4 42 defensive-miracles
```

万能反射・魔法反射の全種類、反撃、追加魔法、吸収、霧・幻覚、守護神、蘇生、終末、複数人戦、
現行公式との動的照合と学習器は未対応。既存の資料間衝突C01/C02は未解消のままにする。

## 検証結果（2026-10-31）

通常CTest 31件、ASan/UBSan 21件が通過。新旧10種類のPython suiteは計82テスト。
手札・保持の210/215、MP境界、Glory、全対応MAGICの跳ね返し、命中再抽選なし、
非damageへの通常防具、属性不適合prefixと通常防御の区別、暫定HP0からの回復、
38回の保持連鎖、配布予定の不正復元拒否、秘匿、1000試合の再現を確認した。
両者の手札が16枚のまま保持応答を続け、元攻撃分を含む39個の配布抽選が最後に消費される局面も検証した。

全機能有効・10万試合・seed42・意思決定上限512で、1/4スレッドの統計とchecksumが一致した。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 3,372,729 |
| 座席0 / 座席1勝利 | 49,504 / 49,936 |
| 勝者なし / 打ち切り | 560 / 0 |
| 防御奇跡の初使用 / 保持からの再使用 | 4,128 / 2,081 |
| 防御奇跡のMP消費 / 新規保持数 | 36,611 / 4,128 |
| 魔法跳ね返し / うち自分への跳ね返り | 643 / 330 |
| 武器遮断 | 5,566 |
| 全奇跡の初使用 / 再使用 | 62,046 / 59,304 |
| checksum v10 | 17954910154238113174 |

魔法跳ね返しは武器反応の統計へ混ぜず、相手への跳ね返し中は元の攻撃奇跡を未解決として数える。
参考速度は1スレッド約356万、4スレッド約1,337万意思決定/秒。
単発測定で、行動生成・合法性検証・遷移・統計・resetを含み、観測転送・モデル推論は含まない。
旧9profileのchecksum式を維持し、同条件の各10万試合で意思決定数・勝敗・checksumが以前と一致した。

共通観測の新しい追加部分は[HP吸収v28](legacy-hp-absorption-profile.md)を参照。旧ABIの幅・行動形式は維持する。
