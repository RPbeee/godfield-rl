# 攻撃属性の付け替え v11

`legacy-attribute-change-subset-v11` は火・水の属性付け替え武器2種を追加する限定profile。
CLIの `attribute-change` と `ga_*` はv11までの拡張を有効にし、174種・旧重み合計362で動く。
旧非公式エミュレーターを参考にした実装で、現行公式の完全再現ではない。

## 対象と資料間の差

[旧版処理と順序の検算](legacy-attribute-change-rules.md)、
[台帳・出典・指紋](../data/legacy-attribute-change-v1/README.md)を参照。
既存10台帳を変更せず、旧重みを限定プール内で正規化する。公式の配布率とは扱わない。

| 旧ID | 武器として単独使用 | 追加部品として使用 | 旧価格 / 重み |
| --- | --- | --- | --- |
| 65 | 火ATK2 | その位置で火へ上書き。追加ATKは0 | 15 / 1 |
| 85 | 水ATK5 | その位置で水へ上書き。追加ATKは0 | 15 / 1 |

両方とも武器なので捨てられず、所持中は祈りの武器不所持条件を満たさない。
基本4属性の武器とは違い、追加使用で通常の属性混合をせず、その時点の属性を強制上書きする。
それより後の普通boostとは再び通常合成する。

**旧queueAttackはADD_ATTRIBUTE追加時に2/5を攻撃力へ加算しない。**
2026-09-20の第三者による現行画面抽出には、対応候補として「+ATK2/5＋火/水へ付け替え」という表示がある。
この差を[衝突C04](official-spec-gap.md)として残し、旧実装の非加算を現行公式の規則とは扱わない。
旧エミュレーターの不具合か、版差か、対応候補の違いかは未確定。
今回のlegacy profileへ現行表示の加算だけを混ぜない。
防御時の属性消去195、追加魔法197/217、倍率変更などは別効果として未対応のままにする。

## 使用順を3つの結果へまとめる

今回の追加部品は、属性を上書きする2種と、既存の純INCREASE_ATKだけ。
最後の上書きより前の属性は消え、後ろにある普通boostだけで最終属性が決まる。
この範囲では、選択した同じ部品から作れる結果は火・水・無属性に限られる。
攻撃力・消費・配布予定・元武器の付随効果は使用順によって変わらない。

| 行動kind | 合法になる条件 | 実在する使用順の例 |
| --- | --- | --- |
| attack_set_fire=7 | 選択extraに65がある | 他のextra全て→65 |
| attack_set_water=8 | 選択extraに85がある | 他のextra全て→85 |
| attack_set_neutral=9 | 選択extraに上書き部品Sと、Sとは異色かつ光ではない普通boost Bがある | 他のextra全て→S→B |

baseには元の武器、slotsには追加部品全体の位置mask、targetには座席0/1を指定する。
無属性の証人Bに、先頭武器の属性や別の上書き部品を数えない。光boostも異色の証人にはならない。
65と85だけを追加した場合は火・水を作れるが、無属性は作れない。
各部品のATKは普通boostだけを加算し、上書き部品も消費と配布予定には数える。
同じIDの別スロットは別個体として使え、先頭との重複は不可。
新kindは通常の確定命中武器を先頭にした場合だけ使える。奇跡・確率武器には付けられない。

従来attack=1の追加maskは普通boostだけで、上書き部品を含めない。
65/85を先頭として使うだけならattack=1でよく、後ろへ普通boostも付けられる。
この意味的な圧縮により、1行動のまま6byte形式を維持し、使用順の候補を欠落させない。
元XMLの選択順そのものを保存する方式ではなく、状態遷移が等しい順序をまとめた表現である。
今後、順序によって他の効果が変わる部品を追加する際は、この圧縮の前提を再検証する。

## 来歴・防御・付随効果

`Pending::attribute_card` は選択した追加部品のうち、代表となる最後の上書き神器IDを持つ。
火/水では対応する色の最小スロット、無属性では上記の証人条件を満たすSの最小スロットを選ぶ。
複数の順序が同じ結果を作っても、同じActionから一意になるよう正規化した来歴である。
実際に選んだXML順序の最終神器を記録しているわけではない。

病気・Glory・DarkCloud等のpayloadは元の先頭武器から維持する。
防御の可否・闇の即死・通常武器用の反射/遮断は最終属性で判定する。
例えば病気付き武器を火へ上書きしても病気を消さず、無属性へした場合は既存の反応連鎖へ入れる。
反射後もattribute_cardとpayloadを維持し、命中判定や元の消費を繰り返さない。
奇跡を武器へ変換する規則は追加しない。

## C++・観測・API

```cpp
godfield::legacy::Env env({512, 10, true, true, true, true, true, true, true, true, true, true});
env.reset(42);
```

末尾の `Config::attribute_change` はデフォルトfalse。有効化にはelementsが必要で、他の拡張は個別に選べる。
CLIとC APIは全拡張有効。再現にはprofile名だけでなく全Configを保存する。
`Options::attribute_slots` は追加使用できる上書き部品の位置maskで、boost_slotsとは分ける。
先頭武器としてはweapon_basesにも載る。各構成要素のmaskだけでは3結果の合法性は表せないので `legal()` でも検証する。
`StepResult::attribute_attack/attribute_cards_used/attribute_element` は上書き部品を追加した攻撃を記録する。
Snapshotの共通versionは35。同じビルド・設定のfixture用で、旧形式を自動移行する永続checkpointではない。

共通C++観測は1317次元、`ga_*` ABI 11は先頭375次元を返す。
v11では末尾942個が0になる。追加部分は[属性消去profile](legacy-attribute-removal-profile.md)と[反撃profile](legacy-counter-profile.md)と[反撃拡張profile](legacy-counter-extensions-profile.md)と[所持金反撃profile](legacy-money-counter-profile.md)と[魔法反射profile](legacy-magic-reflection-profile.md)と[万能反射profile](legacy-any-reflection-profile.md)と[MP武器profile](legacy-mana-weapon-profile.md)と[魔法遮断profile](legacy-magic-block-profile.md)を参照。先頭356個は[防御奇跡profile](legacy-defensive-miracles-profile.md)を継承する。

| 位置 | 内容 |
| --- | --- |
| 356 | attribute_change有効なら1 |
| 357〜372 | 自分の各手札位置の上書き色enum / 6。65は1/6、85は2/6、他0 |
| 373 | 未解決攻撃のattribute_card / 363 |
| 374 | 未解決攻撃に上書き来歴があれば1 |

通常の手札ATK欄は先頭使用の2/5を表示し、追加ATK欄は0。
相手の手札・完全な保持数、未来の抽選値・乱数状態は出さない。旧profileの追加19特徴は0。

[attribute_change_c_api.h](../include/godfield/attribute_change_c_api.h) の `ga_*` と
[AttributeChangeBatchEnv](../python/attribute_change_env.py) を使う。
6byteの `gl_action` を維持し、新ABIだけkind7〜9を追加。mask幅9の最後がattribute_slots。
旧 `gl/gm/ge/gc/gr/gd/gh/gs/go/gb` の128/164/186/210/236/258/283/301/334/356次元とmask幅・kind範囲は維持する。
バッチの原子的拒否、固定座席報酬、終了行固定・自動resetなしを継承する。
Pythonバッファーは再使用するので、履歴へ保存する際はコピーする。同じハンドルを複数スレッドから同時操作しない。

```sh
python3 python/attribute_change_env.py
./build/godfield_legacy_bench 100000 1 42 attribute-change
./build/godfield_legacy_bench 100000 4 42 attribute-change
```

属性消去195、追加魔法、反撃、吸収、霧・幻覚、守護神、蘇生、終末、複数人戦、現行公式との動的照合と学習器は未対応。

## 検証結果（2026-10-31）

通常CTest 34件、ASan/UBSan 23件が通過。新旧11種類のPython suiteは計90テスト。
独立した全順列相当DPで125,125ケース、実カード2,702構成で3結果の到達可否と量・消費・来歴を照合した。
別の集合・全順列モデルでも25,410局面/983,262順列で遷移集合の一致を確認した。
最大15追加、同ID別個体、base/光をNEの証人にしない拒否、病気・Glory・DarkCloudの維持、
反射・遮断、観測秘匿・不正復元拒否、1000試合の再現も検証した。

全機能有効・10万試合・seed42・意思決定上限512で、1/4スレッドの統計とchecksumが一致した。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 3,380,925 |
| 座席0 / 座席1勝利 | 49,672 / 49,755 |
| 勝者なし / 打ち切り | 573 / 0 |
| 上書き部品を追加した攻撃 / 上書き部品の消費枚数 | 11,346 / 11,404 |
| 最終火 / 水 / 無属性 | 5,259 / 5,335 / 752 |
| 65/85を先頭武器として使用 | 8,999 |
| checksum v11 | 3472155029955084997 |

参考速度は1スレッド約338万、4スレッド約1,194万意思決定/秒。
単発測定で、行動生成・合法性検証・遷移・統計・resetを含み、観測転送・モデル推論は含まない。
旧10profileのchecksum式を維持し、同条件の各10万試合で意思決定数・勝敗・checksumが以前と一致した。

共通観測の新しい追加部分は[HP吸収v28](legacy-hp-absorption-profile.md)を参照。旧ABIの幅・行動形式は維持する。
