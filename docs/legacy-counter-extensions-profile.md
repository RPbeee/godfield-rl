# 順序付き反撃拡張 v14

`legacy-counter-extensions-subset-v14` は、土190に4種の反撃と混合順序を追加する限定profile。
CLIの `counter-extensions` と `gx_*` は既存拡張も有効にし、180種・旧重み合計370で動く。
旧非公式エミュレーターを参考にした実装で、現行公式との完全一致は確認していない。

## 追加した神器と効果

[処理順の調査](legacy-counter-extensions-rules.md)、[台帳・出典・hash](../data/legacy-counter-extensions-v1/README.md)、
[土反撃と死亡・キューの基礎](legacy-counter-profile.md)を参照。
旧13台帳を変更せず、追加4種を含むプール内で重みを正規化する。公式の配布率とは扱わない。

| 旧ID | 属性 | 予約効果 | 対象 |
| --- | --- | --- | --- |
| 187 | 火 | 防御後のraw残余rの75%攻撃 | 予約時に生存している全敵。1対1では相手1人 |
| 190（既存） | 土 | raw残余r×2の確定攻撃 | 現在のincoming source |
| 191 | 光 | 直接Glory付与 | 現在のincoming source |
| 192 | 闇 | 直接DarkCloud付与 | 現在のincoming source |
| 193 | 無属性 | MP+r×2 | counter使用者自身 |

全て通常DEF0・MP消費0・価格10・重み1で、消費型の防具。保持奇跡や攻撃武器にはしない。
188霧・189幻覚・194所持金吸収はv14の対象外で、渡された場合は拒否する。
194は[所持金反撃v15](legacy-money-counter-profile.md)に分けて追加する。

全prefixの通常防御・病気・属性消去を先に処理し、残余r>0かつ特殊応答なしの場合だけ、
選択した反撃を指定順でFIFO末尾へ積む。各反撃は同じ最終残余rを使う。
r=0・非damage・特殊応答時は反撃を予約しないが、選択した防具は消費され配布予定へ数える。
反撃の属性も防御合成へ参加する。193の無属性を合成から除外しない。

191/192は直接状態付与で、普通DEFを積んでも付与を止められない。195は属性だけを消し、効果を消さない。
192の闇は即死ATKではない。重複排除より先にHeaven転落を処理する既存のharms規則を使う。
193は起動時に自分のMPへ2rを加え、上限99で止める。防御応答は要求せず、元被弾等で死亡済みならskipする。
死者sourceから生者targetへの予約済み反撃は実行するという190等の規則とは区別する。

187の命中は予約時ではなく起動時に決める。対象がDarkCloudなら必中でhit乱数を消費しない。
予約後にtargetが死亡していた場合も、旧実装の順序どおり命中処理を先に行うが、
その非公開hit/missをLastChanceや観測へ出さない。公開イベントは直前のものを保つ。
予約時に生存敵が0人なら187の項目自体を作らない。

反撃への防御は旧server受理を採用し、普通防具・195・再反撃を使える。
武器用20/39・魔法用210・武器遮断215は反撃へ使えない。
防御しない旧botとの相違C05は未解消。光攻撃へ192闇と158光DEFを合成する経路も到達可能となるが、
光を闇で防げる旧規則と現行表示の相違C01は解消していない。

## 順序付き行動

同じ選択maskでも反撃順序によって結果が変わる。
例えばNE攻撃へ192→187なら、先行DarkCloudで後続火反撃が必中になる。
187→192なら火反撃を先に抽選する。191→190ならGloryの単枚制限が後続の土防御に先にかかる。

C++ `Action` は従来のkind/base/slots/targetに `counter_count:uint8` と `counter_order:uint64` を追加する。
orderの下位4bitから順に、**消費前の手札slot番号**を格納する。
slots内のcounterを重複なく全て列挙し、countを正確に一致させる。最大16枚。
未使用上位bitは0、通常手番はcount/orderとも0。特殊で予約が抑止される場合も列挙が必要。
旧profileでは拡張2フィールドを0にし、190だけの既存挙動を維持する。

新C ABI [counter_extensions_c_api.h](../include/godfield/counter_extensions_c_api.h) は `gx_*` version14。
`gx_action` は16byteで、最初の6byteは旧 `gl_action` と同じ配置。
countはoffset6、追加reservedは7、orderは8。reservedは両方0が必要。
**既存のgl〜gk APIは6byte行動・各観測幅を維持する。** 共通C++ Actionのメモリ配置を旧C行動へ直接castしない。

[Pythonラッパー](../python/counter_extensions_env.py)では、次のようにslotのtupleで指定する。
この例の2つのslotに反撃がある局面を想定する。

```python
from counter_extensions_env import Action, ActionKind, CounterExtensionsBatchEnv

action = Action(ActionKind.DEFEND, slots=(1 << 2) | (1 << 0), counter_order=(2, 0))
with CounterExtensionsBatchEnv(16) as env:
    env.reset(42)
    observations, options, actors, phases, pray = env.observe()
    actions = env.sample_actions()
    assert all(env.legal(actions))
    rewards, terminated, truncated = env.step(actions)
```

mask幅は11を維持し、最後のcounter_slotsは5種全ての候補。
属性合成・Glory・195・特殊応答の条件を同時に満たす必要があるため、候補maskだけを完全な合法手集合とみなさず `legal()` を使う。
新profileでは組合せによって属性が適合する候補も提示する。native samplerはcounter部分の順序を方策用RNGで選ぶ。
ゲーム用RNGや未来の抽選は行動生成・観測へ渡さない。

全active行を事前検証し、不正があればバッチ全体を原子的に拒否する。
終了行は固定・報酬0・不正手を無視し、自動resetしない。報酬は固定座席順。
Pythonの返却バッファーは再使用するため、replay保存時はコピーする。同一ハンドルの同時操作はしない。

## C++・状態・観測

`Config::counter_extensions` はデフォルトfalseで、countersとcelestialを必要とする。
celestialにはelements/chance/harmsも必要。他の既存拡張は個別に選べる。
キュー32枠・生成予算・int64威力上限・項目間死亡確定・全キュー後のtick/補充/勝敗はv13を継承する。
193がMPを増やすのは元MAGIC連鎖の終了後で、以後のcounterはMAGICではないため、
現scopeでは保持防御奇跡の配布上限20を維持できる。[根拠](legacy-counter-extensions-rules.md#mp回復とretained_deals上限20)を参照。

Snapshot共通versionは37。同じビルド・Config内のfixture用で、永続checkpointの自動移行はしない。
各counterのkind/ID/属性/率/harm/対象方針・威力倍率・生成予算を検証する。
193は自動解決なので未解決current193を拒否し、未解決187は公開済み命中イベントとの整合も検証する。

共通C++観測は1368次元、gx出力は先頭661次元。v14では末尾707特徴が0。先頭515個はv13の位置を維持し、
手札counter特徴394〜409は5種全てを認識する。追加146特徴は旧profileでは0。

| 位置 | 内容 |
| --- | --- |
| 515 | counter_extensions有効なら1 |
| 516〜531 | 自分の各手札が追加4種なら、187/191/192/193を1/2/3/4として4で割る。その他0 |
| 532 | 未解決項目が直接harmなら1 |
| 533〜660 | 公開queue32枠×4値。counter_card/363、Effect enum/6、hit_rate/100、harm enum/2。空き0 |

量の正規化に上限1という仮定を置かない。相手の未使用手札・保持の完全数・乱数状態・
未来の抽選値・非公開抽選の結果・病気の内部悪化率は観測へ含めない。

StepResultのcounter_queued/launched/skipped/resolvedに加えて、missed・auto_resolved、
公開chance内訳、死亡済みtargetへの非公開抽選のhidden_checks、実際のMP増加、直接harm適用件数を区別する。
resolvedは外部応答待ちcounterの解決を表し、193はauto_resolvedへ数える。
launchedには193も含み、死亡skipやmissは含めない。hidden_checksは監査用でhit/missの結果を返さない。

```sh
python3 python/counter_extensions_env.py
./build/godfield_legacy_bench 100000 1 42 counter-extensions
./build/godfield_legacy_bench 100000 4 42 counter-extensions
```

## 検証結果（2026-10-31）

通常CTest43件、ASan/UBSan29件、14種類のPython suite計119テストが通過。混合順序2対照、16個全nibble、自己MP回復・上限・dead skip、
直接harmへの過剰DEFとHeaven、非公開hit/missが同じ観測になること、最小依存Config、
variant/orderの不正復元・行動の原子的拒否、1000試合replayを検証した。

全機能有効・10万試合・seed42・意思決定上限512で、1/4スレッドの全統計とchecksumが一致した。

| 項目 | 値 |
| --- | --- |
| 意思決定数 | 3,376,320 |
| 座席0 / 座席1勝利 | 49,551 / 49,794 |
| 勝者なし / 打ち切り | 655 / 0 |
| 反撃予約 / 起動 / 死亡target skip / miss | 14,919 / 13,924 / 261 / 734 |
| 外部反撃解決 / 自動自己MP解決 | 11,456 / 2,468 |
| 公開火反撃の命中試行 / hit / miss / forced | 3,000 / 2,266 / 734 / 69 |
| 死亡targetへの非公開命中処理 | 3 |
| counterによる実MP増加 / 直接harm適用 | 35,239 / 6,352 |
| counter消費枚数 | 23,147 |
| 未処理キュー / 未解決反撃 | 0 / 0 |
| checksum v14 | 2918521066370405427 |

予約されなかった反撃も消費枚数へ含める。queue予約=起動+skip+miss+残数、
起動=外部解決+自動解決+未解決counterの収支を検査する。
公開火反撃はベンチのchance総計にも加算するが、非公開のhit/missは集計しない。
v14ではcounterへの195使用をcounter_removal_responsesへ分け、武器への応答内訳に含めない。
checksum v14は反撃順序の履歴も含める。旧13profileは従来のchecksum式を維持し、
各10万試合で意思決定数・勝敗・draw・打ち切り・checksumが以前と一致した。
ベンチの速度出力は行動生成・合法性検証・遷移・統計・resetを含み、観測転送・モデル推論を含まない。

v14では188/189/194は対象外。v15でも188/189、万能反射・追加魔法・忘却・守護神・蘇生・死亡時効果・終末・複数人戦、
旧版Pythonとの動的差分実行と現行公式照合、学習器・学習済みモデルは未対応。資料間衝突C01〜C05も残す。

共通観測の新しい追加部分は[HP吸収v28](legacy-hp-absorption-profile.md)を参照。旧ABIの幅・行動形式は維持する。
