# 病気・悪化・継続効果・解除 v6

`legacy-disease-subset-v6` はCold/Fever/Hell/Heavenの病気と解除を追加した限定profile。
CLIと `gd_*` はv6までの機能を有効にし、harmsは無効のまま138種で動く。現行公式の完全再現ではない。
霧・幻覚・Glory・DarkCloud等のharms、守護神、蘇生はこの段階では扱わない。

## 対象と出典

[旧版の病気処理の調査](legacy-disease-rules.md)と
[台帳・出典](../data/legacy-disease-v1/README.md)を参照。

| 旧ID | 効果 |
| --- | --- |
| 30 | 無属性ATK8、ダメージ時Hell |
| 34 | 無属性ATK9、ダメージ時Cold |
| 49 | 無属性ATK13、ダメージ時Cold |
| 61 | 土ATK1、ダメージ時Cold |
| 175 | 火DEF10、使用者へFever |
| 234 | MP+20、その対象へHeaven |
| 235 | 軽い病気の解除 |
| 236 | 全病気の解除 |

追加8種の旧重み合計は24。全機能構成の重み合計326で復元抽出する。
対象プール内で旧重みを正規化しており、公式の出現分布とは扱わない。
旧ID・技術ラベルで管理し、現行公式IDとの対応は未確定。
薬の元の効果は病気以外のharmsも対象に含むが、今回はそれらが存在しないprofileへ限定する。
harmsを有効にして薬の対象も広げた環境は、別の[v7 profile](legacy-harms-profile.md)として提供する。
このv6のプールや挙動へharmsを混ぜず、病気だけを治す独立した公式効果だとも扱わない。

## 付与・継続効果・解除

| 状態 | 手番末の効果 |
| --- | --- |
| Cold | HP-1 |
| Fever | HP-2 |
| Hell | HP-5 |
| Heaven | HP+5、上限99 |

病気なしなら受けた病気になる。既に病気なら、incomingが上位ならincomingへ、
同位・下位なら現在の病気を1段進める。Heavenへさらに病気が付くとHP0になり、悪化率を0へ戻す。
Heavenという状態名自体を消してから処理するわけではない。

ダメージ時の病気は、防御後に残る攻撃力が正なら付け、その後に通常ダメージを与える。
完全防御なら付かない。反射で対象が変われば、最終対象へ付ける。
防具175の病気は防具を使用した時点で使用者へ付くため、完全防御でも発生する。
MP+20とHeavenはMPを増やしてからHeavenを付ける。どちらの効果も省略しない。

軽い解除はCold/Feverだけを消し、Hell/Heavenとその悪化率は残す。
全解除はHeavenも直接消し、再付与による転落を起こさない。
HP回復だけでは病気は治らない。使用・解除は自分または相手を指定できる。
相手へ使う場合は応答を待ち、空の防御で受ける。病気付与や薬の奇跡はまだ対象外。

## 自然悪化は旧版方式

初期・新規付与・悪化の直後は内部の悪化率カウンター0。
通常手番の所有者が生きていて病気なら、手番末に0〜99を1回抽選する。
現在のカウンターより小さければ1段悪化してカウンター0へ、そうでなければ1増やす。
その後、悪化後の病気の継続効果を1回適用する。

0%でも抽選を消費し、失敗して1へ進む。100なら必ず悪化するため自然に100を超えない。
Cold→Feverとなった回はHP-2、Hell→HeavenならHP+5。
Heavenの自然悪化でHP0になったときは、その後にHP+5で復活させない。

**公式画面の抽出記録にある固定5%とは異なる。**
このprofileは旧版の増加方式を使い、衝突C02を解消済みとは扱わない。
固定5%への切替・現行公式との動的照合はまだ行っていない。

## 手番末・死亡・反応連鎖の順序

1. 攻撃と全ての応答・反射を解決する。
2. 元の通常手番所有者がHP正で病気なら、自然悪化と継続効果を1回処理する。
3. 生存者へ予定分の神器を座席順で配る。
4. 終局を判定し、継続するなら次の通常手番を選ぶ。

最後に防御した人全員へtickするのではない。相手を倒した手番でも、所有者自身の病気は処理する。
既にHP0ならtickをせず、Heavenで復活させない。

両者HP0となる経路を扱い、`terminated=true, winner=-1, rewards=[0,0]` とする。
これは旧サーバーが0人生存でも終了することを、**学習環境で勝者なしのdrawとして表す規約**。
公式の引き分けや順位・レートの規則を確定したものではない。学習用打ち切りとも区別する。

旧サーバーではHP0と死亡確定フラグが別である。
Heavenの人が防具175を使ってHP0になり、その後に反射を選ぶと、HP0のまま応答連鎖が続く経路がある。
今回のコアでも、病気と反応の未解決連鎖に限ってこの暫定HP0を扱う。
跳ね返し対象にも死亡未確定の人物を含め、解決境界で死亡を確定する。
旧クライアントUIがこの局面で何を選択できたかは未検証で、サーバーの到達可能経路として扱う。
通常手番開始時に任意のHP0状態を行動させることとは区別する。

## C++・検証用抽選

```cpp
godfield::legacy::Env env({512, 10, true, true, true, true, true, false});
env.reset(42);
```

`Config::disease` はデフォルトfalse。属性付き病気神器を含むのでelementsも有効にする必要がある。
v6名は共通で、再現にはConfigの全flagも保存する。CLIと `gd_create` はv6までの機能を有効にし、harms=falseを維持する。

Playerは病気と内部悪化率、Pendingは元の病気神器ID・付与病気・解除種別を持つ。
`Effect::cure` は応答を持てる0量の解除効果として扱い、HP/MPの0回復へ置き換えない。
`State::last_tick` は最後の公開tick記録で、座席、前後の病気、悪化の有無、実際のHP増減を保持する。
過去の記録なので、その後の行動で変わった現在の病気と常に一致するとは仮定しない。
ただし、段飛ばし・不可能なHP変化など矛盾した過去記録は復元で拒否する。

`StepResult::disease_tick/disease_worsened/disease_damage/disease_healing` は当該stepの自然tickだけを返す。
ダメージ・回復は上限処理後の実HP変化であり、武器のダメージと重複計上しない。
`script_disease_rolls()` は最大64件、0〜99のfixture用prefix。
指定分はゲーム乱数を進めず、使い切ったらseed付き抽選へ戻る。
病気なし・tick対象がHP0・解除済みなら抽選しない。無効profileは空prefixのクリアだけを許す。

Snapshotは反撃拡張と共通のversion37。病気・悪化率・保留効果・暫定HP0連鎖・抽選prefix・過去記録を検証する。
旧versionを黙って移行せず、不正入力は状態・乱数を変えず拒否する。
永続checkpointや、旧版と同じseedの再現を保証する形式ではない。

## 観測とABI

C++の共通観測は1368次元、`gd_*` ABI 6は先頭258次元を返し、構成要素maskは7種類。
このv6 profileでは末尾1110個は0。追加部分は[harms profile](legacy-harms-profile.md)と[光・闇profile](legacy-celestial-profile.md)と[攻撃奇跡profile](legacy-offensive-miracles-profile.md)と[防御奇跡profile](legacy-defensive-miracles-profile.md)と[属性付け替えprofile](legacy-attribute-change-profile.md)と[属性消去profile](legacy-attribute-removal-profile.md)と[反撃profile](legacy-counter-profile.md)と[反撃拡張profile](legacy-counter-extensions-profile.md)と[所持金反撃profile](legacy-money-counter-profile.md)と[魔法反射profile](legacy-magic-reflection-profile.md)と[万能反射profile](legacy-any-reflection-profile.md)と[MP武器profile](legacy-mana-weapon-profile.md)と[魔法遮断profile](legacy-magic-block-profile.md)を参照。
`gl_*` / `gm_*` / `ge_*` / `gc_*` / `gr_*` は128 / 164 / 186 / 210 / 236次元を維持する。

| 位置 | 内容 |
| --- | --- |
| 0〜235 | [反応profile](legacy-reflection-profile.md)の定義を維持 |
| 236〜240 | 自分の病気のone-hot。none/cold/fever/hell/heaven |
| 241〜245 | 相手の公開病気のone-hot。同じ順 |
| 246 | 保留中に付与する病気enum / 4 |
| 247 | 保留中の解除種別enum / 2 |
| 248 | 病気profile有効なら1 |
| 249〜250 | 最後の公開tick記録あり / そのtickで悪化したならそれぞれ1 |
| 251 | 最後のtick対象が現在の行動プレイヤーなら1 |
| 252〜253 | 最後のtickの前後の病気enum / 4 |
| 254〜255 | そのtickの実HP減少 / 増加。それぞれ99で割る |
| 256 | 保留中の病気・解除神器ID / 363 |
| 257 | 保留効果がcureなら1 |

病気無効profileでは追加22個は全て0。悪化率カウンターと未来の抽選prefixは出さない。
過去の公開イベントから推定できる情報と、内部の真値を直接渡すことは区別する。
[DiseaseBatchEnv](../python/disease_env.py) は依存追加なしのctypesラッパー。
0〜99の検証用抽選を指定でき、バッチの原子的拒否・終了行凍結・固定座席報酬を維持する。

## 実行・対応限界

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
ctest --test-dir build --output-on-failure
python3 python/disease_env.py
./build/godfield_legacy_bench 100000 1 42 disease
./build/godfield_legacy_bench 100000 4 42 disease
```

diseaseのchecksumはversion6。既存5profileの計算式は維持する。
統計は両者HP0のdrawを独立に数え、勝利・draw・打ち切りの合計を検証する。
奇跡の回復量は同stepの自然tickによるHP変化を差し戻し、病気の回復・ダメージと区別する。
速度には行動生成・状態更新・統計処理を含み、モデル推論・観測転送は含まない。

霧・幻覚・Glory・DarkCloud、病気付与・解除の奇跡、万能反射、守護神、蘇生、終末、複数人戦等は未対応。
現在の公式説明との相違・UIの確認・実サーバーとの動的照合は残る。

### 今回の検証結果（2026-10-31）

通常ビルドのCTestは19件、ASan/UBSanビルドは13件が通過。
追加した過去tick記録の整合検証も、正例・不正例とsanitizerで確認した。
付与・重複・0/1/99/100の悪化率境界、解除、反射後のtick、双方の暫定HP0連鎖、
生存者なしの終局、秘匿、1000seed再現を検証している。

全機能有効の10万試合・seed42・意思決定上限512で、1スレッドと4スレッドの次の値が一致した。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 4,403,428 |
| 座席0 / 座席1の勝利数 | 49,472 / 49,742 |
| 勝者なしの終局 / 打ち切り | 786 / 0 |
| 病気tick / 自然悪化 | 279,332 / 9,759 |
| tickによる実HP減少 / 増加 | 596,822 / 296,181 |
| checksum v6 | 2098379072543714231 |

参考速度は1スレッド約529万意思決定/秒、4スレッド約1,956万意思決定/秒。
単発の測定であり、モデル推論・観測転送は含まない。
既存5profileの同条件10万試合も、以前の意思決定数・勝敗・checksumを維持した。

共通観測の新しい追加部分は[HP吸収v28](legacy-hp-absorption-profile.md)を参照。旧ABIの幅・行動形式は維持する。
