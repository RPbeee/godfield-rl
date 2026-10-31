# 光・闇と通常のGlory付与 v8

`legacy-celestial-subset-v8` は光・闇属性と18神器を追加する旧版限定profile。
CLIと `gs_*` はv8までの拡張を有効にし、攻撃奇跡は無効のまま157種・重み合計345で動く。
旧非公式エミュレーターを参考にした実装で、現行公式の完全再現ではない。

## 対象と根拠

[旧版の処理と資料間の衝突](legacy-light-dark-rules.md)、[台帳と出典](../data/legacy-celestial-v1/README.md)を参照。
旧IDと技術ラベルで管理し、現行公式IDとの同一性は保証しない。

| 種類 | 旧ID | 内容 |
| --- | --- | --- |
| 光・闇の確定武器 | 62, 63, 75, 76, 82, 83, 87, 88, 89, 93, 96, 97 | 通常攻撃。62は光ATK1/追加ATK1、89は闇ATK5/追加ATK5の両役 |
| Glory武器 | 70 | 光ATK2、正の残余ダメージでGlory |
| 確率武器 | 101, 104, 111 | 闇ATK2/50%、光ATK3/25%、光ATK6/75% |
| 光防具 | 158, 176 | DEF6、DEF10 |

各重み1。既存7台帳は変更せず、18種を加えたプール内で旧重みを正規化する。
公式の配布分布とは扱わない。COUNTER、死亡時攻撃、倍率・属性変更、光闇の奇跡は対象外。
未対応の付随効果を消した簡略神器は含めない。

## 合成・防御・闇の即死

普通の属性合成では光が代用になる。光+火は火、光+闇は闇、光だけなら光。
光以外の同属性は維持し、異なる属性が混ざると無属性になる。無属性も実際の属性として数える。
光を加えても、既に無属性になった混合を元の属性へ戻せない。
この対象範囲では順序によらずbitmaskで選択できる。ADD_ATTRIBUTEは引き続き未対応。

| 攻撃の最終属性 | 旧版の通常防御として適合する最終属性 |
| --- | --- |
| 無属性・闇 | 全て |
| 火 / 水 / 木 / 土 | 水 / 火 / 土 / 木、または光 |
| 光 | 闇のみ |

**今回の台帳には闇防具がなく、通常の光攻撃を防ぐ神器はない。**
旧版の「光へ闇防御可」と現行抽出記録の「属性で防御不可」は、一般規則では衝突C01として残す。
この18種の追加でその衝突を解消したとは扱わない。特殊反撃などの将来拡張では再検証が必要。

闇攻撃は通常防御で威力を減らし、残余が正なら旧版の999ダメージ相当でHP0にする。
完全防御なら即死しない。無属性等へ合成された闇武器は、最終属性の通常攻撃として解決する。
自己対象でも同じ判定。通常反射・跳ね返しは無属性武器攻撃専用のままで、光・闇へ拡大しない。
最終的に無属性になった攻撃なら、既存の反応連鎖へ入れる。

## Glory・確率攻撃との連携

旧ID70を通常抽選に追加し、Glory付与→次の応答で1枚防御という経路を扱う。
Gloryは残余ダメージが正のときだけ付与。光を他属性へ合成しても付随効果を維持し、
反射された場合は現在の対象へ付ける。
闇へ合成した場合も、Glory付与を処理してから闇の即死を処理する。
Heavenへの付与・重複・薬の解除・1枚制限の根拠は[harms profile](legacy-harms-profile.md)を引き継ぐ。
1枚制限は旧botを根拠に採用し、旧サーバーの手動入力受理との違いを残す。

確率武器3種は既存の確率武器と同様、1対1の相手へ1回判定する。
追加ATKとの併用は禁止。DarkCloud対象なら抽選なしで必中。
missでは応答を待たず、消費分を手番末に補充する。
闇の確率武器でも、命中後は防御で全て止められれば即死しない。

## C++・観測・API

```cpp
godfield::legacy::Env env({512, 10, true, true, true, true, true, true, true});
env.reset(42);
```

末尾の `Config::celestial` はデフォルトfalse。対象神器が既存機構を使うため、
celestialを有効にする場合はelements・chance・harmsも必要。不正な組み合わせは構築時に拒否する。
miracles・reflection・diseaseは個別に選べる。再現には全Configを保存する。
Snapshotの共通versionは37。同じビルド・設定のfixture用で、旧versionの永続互換は保証しない。
`StepResult::dark_execution` は当該stepで正の闇残余による即死を解決したかを返す。

共通C++観測は1368次元、`gs_*` ABI 8は先頭301次元を返す。
v8では末尾1067個が0になる。追加部分は[攻撃奇跡profile](legacy-offensive-miracles-profile.md)と[防御奇跡profile](legacy-defensive-miracles-profile.md)と[属性付け替えprofile](legacy-attribute-change-profile.md)と[属性消去profile](legacy-attribute-removal-profile.md)と[反撃profile](legacy-counter-profile.md)と[反撃拡張profile](legacy-counter-extensions-profile.md)と[所持金反撃profile](legacy-money-counter-profile.md)と[魔法反射profile](legacy-magic-reflection-profile.md)と[万能反射profile](legacy-any-reflection-profile.md)と[MP武器profile](legacy-mana-weapon-profile.md)と[魔法遮断profile](legacy-magic-block-profile.md)を参照。7種類の構成要素maskと6byteの `gl_action` を使う。
従来 `gl/gm/ge/gc/gr/gd/gh` は128/164/186/210/236/258/283次元を維持する。
v8部分の追加18特徴は従来profileでは0。

| 位置 | 内容 |
| --- | --- |
| 0〜282 | 既存観測を継承。旧pending属性one-hot 164〜168は光・闇なら全て0。旧手札属性169〜184は光・闇だけ0とし、以下の新特徴で識別する |
| 283 | celestial有効なら1 |
| 284 | 保留効果の最終属性enum / 6 |
| 285〜300 | 自分の各手札位置の属性enum / 6 |

enumはneutral=0/fire=1/water=2/wood=3/stone=4/light=5/dark=6。
手札の空き位置は0。相手の手札、内部悪化率、未来の抽選値、乱数状態は含めない。
公開Glory/DarkCloudと使用後の確率記録は従来特徴を使う。

[celestial_c_api.h](../include/godfield/celestial_c_api.h) と
[CelestialBatchEnv](../python/celestial_env.py) は、バッチの原子的拒否、固定座席報酬、
終了行固定・自動resetなしを継承する。maskは完全な合法手一覧ではないので、構成後に `legal()` で確認する。
Pythonの返却バッファーは再使用されるため、履歴へ保存する際はコピーする。
同じハンドルを複数スレッドから同時に操作しない。

```sh
python3 python/celestial_env.py
./build/godfield_legacy_bench 100000 1 42 celestial
./build/godfield_legacy_bench 100000 4 42 celestial
```

霧・幻覚、光闇の奇跡、反撃、守護神、蘇生、終末、複数人戦と現行公式との動的照合は未対応。

## 検証結果（2026-10-31）

通常CTest 25件、ASan/UBSan 17件が通過。新旧8種類のPython suiteは計64テスト。
7属性の合成343組と逆順、実在する6属性の防具を使った7攻撃属性×64防御選択、
Gloryの通常付与・合成・反射・薬、闇の部分/完全防御・自己対象・無属性化、確率境界、
観測秘匿・不正復元の原子的拒否、1000試合の再現を確認した。
今回存在しない闇防具を作ったテストは含めず、旧表の光対闇防御は到達不能として残す。

全機能有効・10万試合・seed42・意思決定上限512で、1/4スレッドの統計とchecksumが一致した。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 3,496,884 |
| 座席0 / 座席1勝利 | 49,868 / 49,513 |
| 勝者なし / 打ち切り | 619 / 0 |
| 光攻撃 / 闇攻撃 | 59,309 / 59,407 |
| 闇の即死解決 | 37,889 |
| Glory中の防御意思決定 / 通常付与源 | 22,146 / 1 |
| 確率攻撃 / DarkCloud強制命中 | 125,289 / 2,203 |
| checksum v8 | 14233196389657251118 |

参考速度は1スレッド約444万、4スレッド約1,581万意思決定/秒。
単発測定で、行動生成・合法性検証・遷移・統計・resetを含み、観測転送・モデル推論は含まない。
旧7profileのchecksum式は維持し、同条件の各10万試合で意思決定数・勝敗・checksumが以前と一致した。

共通観測の新しい追加部分は[HP吸収v28](legacy-hp-absorption-profile.md)を参照。旧ABIの幅・行動形式は維持する。
