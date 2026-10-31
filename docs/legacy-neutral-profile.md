# 旧版中立ルールセット v1

`legacy-neutral-subset-v1` は、Igoorx/godfield-flashの挙動を参考にしてC++で実装した、
中立属性の60神器に制限した自己対戦環境。現行公式への完全対応ではない。
初期の `synthetic-neutral-duel-v1` と既存 `gf_*` ABIはそのまま残している。

## 仕様と範囲

実装の根拠関数と照合候補は[旧版処理の調査](legacy-neutral-rules.md)に記載。
資料間の衝突C01〜C03は[仕様差分表](official-spec-gap.md)に残す。

| 項目 | このprofileの動作 |
| --- | --- |
| 初期値 | 2人、HP40・MP10・所持金20、初期配布10、容量16。初期配布はC++ Configで0〜16へ変更可能 |
| 台帳 | 旧CSV ID付き60種。武器35、防具18、HP回復4、MP回復3。技術ラベルであり現行公式名の対応表ではない |
| 抽選 | 台帳の旧重み合計225で復元抽出。除外カードの確率を含む公式の分布ではない |
| 攻撃 | 先頭武器1枚＋追加ATKを持つ神器の部分集合。追加ATK武器は先頭にも使える。通常武器2枚の合算は不可 |
| 防御 | 攻防兼用武器と防具。0防御も選択可。過剰防御でも選択した全てを消費 |
| 回復 | 自分または相手へHP/MP回復。自分なら即解決、相手なら空の防御応答で受ける |
| 自分への攻撃 | 即時に無防御で解決。ベンチマーク方策は自分を攻撃しない |
| 捨てる | 武器は禁止。対象の非武器を複数選択可。捨てによる補充はなし |
| 祈り | 武器を持たない場合のみ可能。配布予定+1。手札が満杯でも抽選結果を消費 |
| 消費 | 選択した個体を削除して残りを詰める。新しい神器は末尾へ追加 |
| 補充 | 通常使用・防御1枚につき配布予定+1。応答完了後、座席0→1の順で生存者へ予定数を配る |
| 通常手番 | 2人の順序を周期ごとに抽選。周期境界では同じ人物の手番が連続し得る。応答者と通常手番の所有者は別 |
| 終局 | 生存者の予定配布後に勝者確定。終局報酬は固定座席で+1/-1。蘇生・死亡時攻撃は対象プールに含めない |
| 打ち切り | デフォルト512意思決定。応答待ちでも適用。自然終局を優先し、打ち切りでは報酬0 |

未対応: 属性、奇跡、取引、状態異常、反射・跳ね返し、守護神、蘇生、終末、複数人戦など。
光の防御判定・病気の悪化確率は実装しておらず、資料間の衝突を推測で解消していない。
現在の公式クライアントとの動的な照合もまだ行っていない。

## C++ API

[legacy_env.hpp](../include/godfield/legacy_env.hpp) の `godfield::legacy::Env` を使う。
`reset(seed)`、`state()`、`observe()`、`options()`、`legal(action)`、`step(action)` を提供する。
各手で候補全体を列挙せず、16bitの構成要素マスクから意味的な行動を作る。
`reset/step/observe/options/legal` は動的メモリ確保を行わない。

| ActionKind | base | slots | target |
| --- | --- | --- | --- |
| pray=0 | 255 | 0 | 255 |
| attack=1 | 先頭武器の手札位置 | 追加ATKの位置マスク。先頭との重複不可 | 0または1 |
| defend=2 | 255 | 防御する位置マスク。空ならそのまま受ける | 255 |
| use=3 | HP/MP回復の手札位置 | 0 | 0または1 |
| discard=4 | 255 | 捨てる位置の非空マスク | 255 |

位置は0〜15、同じIDでも別の位置なら別個体。consume後は位置が変わる。
通常武器、ブースター、防具、回復、捨て可能な神器の各マスクだけでは完全な合法性を表さない。
生成した行動は `legal()` でも確認する。不正手は状態・乱数・配布スクリプトを更新しない。

`sample_action(policy_rng)` はカテゴリ→先頭→追加分という階層的なランダム方策。
全合法手に対する一様分布ではない。ゲーム用の乱数とは独立し、ゲーム状態を変えない。

## 観測 v1

`gl_*` が返す、行動プレイヤー視点の128個のfloat。`actor` は別途固定座席0/1で返す。
反撃対応を追加した後のC++ `Env::observe()` は1345次元の共通形式となり、
このprofileでは末尾1217個が0になる。`gl_*` は先頭128個だけを返して既存ABIを維持する。
末尾の定義は[奇跡profile](legacy-miracles-profile.md)、[属性profile](legacy-elements-profile.md)、
[確率profile](legacy-chance-profile.md)、[反応profile](legacy-reflection-profile.md)、[病気profile](legacy-disease-profile.md)、
[harms profile](legacy-harms-profile.md)と[光・闇profile](legacy-celestial-profile.md)と[攻撃奇跡profile](legacy-offensive-miracles-profile.md)と[防御奇跡profile](legacy-defensive-miracles-profile.md)と[属性付け替えprofile](legacy-attribute-change-profile.md)と[属性消去profile](legacy-attribute-removal-profile.md)と[反撃profile](legacy-counter-profile.md)と[反撃拡張profile](legacy-counter-extensions-profile.md)と[所持金反撃profile](legacy-money-counter-profile.md)と[魔法反射profile](legacy-magic-reflection-profile.md)と[万能反射profile](legacy-any-reflection-profile.md)と[MP武器profile](legacy-mana-weapon-profile.md)と[魔法遮断profile](legacy-magic-block-profile.md)を参照。

| 位置 | 値 |
| --- | --- |
| 0〜2 | 自分HP/MP/所持金。それぞれ99で割る |
| 3〜5 | 相手HP/MP/所持金。それぞれ99で割る |
| 6〜7 | 通常・応答フェーズのone-hot |
| 8 | 未解決効果の大きさ / 480 |
| 9 | 意思決定数 / 上限 |
| 10 | 自分が通常手番の所有者なら1 |
| 11 | 相手手札枚数 / 16 |
| 12〜15 | 未解決効果 none/damage/heal/mana のone-hot |
| 16〜127 | 自分の16スロット × 7値。ID/363、攻撃/30、追加ATK/20、防御/30、HP回復/20、MP回復/20、種別/3 |

空きスロットは全て0。相手手札・未来の抽選結果・乱数状態・内部手番リストは含めない。
状態異常で見える情報が変わる処理は、このprofileでは未対応。

## C ABIとPython

[legacy_c_api.h](../include/godfield/legacy_c_api.h) の `gl_*` は独立したABI v1。
`gl_create` でバッチを確保し、`gl_reset_at`、`gl_observe`、`gl_legal`、`gl_step` を使う。
`gl_sample_actions` は方策乱数を更新してランダム行動を得る補助関数。
観測128、構成要素マスク5、actor/phase/prayの配列を返す。

`gl_action` は明示的な予約byte付き6bytes。予約byteは0にする。
バッチ内に不正な行動があると、全行を更新せず-3を返す。
終了行は行動を無視して状態固定・報酬0。自動resetは行わない。
同じバッチハンドルを複数スレッドから同時操作しない。

Pythonの [LegacyBatchEnv](../python/legacy_env.py) は外部依存なしのctypesラッパー。
返却バッファーは再使用されるため、学習履歴へ保存する際はコピーする。

```sh
python3 python/legacy_env.py
./build/godfield_legacy_bench 100000 4 42
```

## 再現と検証

`snapshot()/restore()` は同じルールセット・同じConfigで使うメモリー内のfixture用。
現在の共通形式はversion36。状態、手番順、ゲーム乱数状態、配布スクリプトと位置を含み、形式tag/versionを検証する。
無効な拡張の状態やprefixを持ち込むことは拒否し、古い形式を黙って移行しない。
ディスク保存用の永続的・異なるビルド間で互換なシリアライズ形式ではない。
未対応神器ID、非正規の空きスロット、不正な手番や配布予定、異なる設定は原子的に拒否する。

`script_draws()` で最大64枚の配布結果を先頭部分として指定できる。
スクリプト部分はゲーム乱数を進めず、使い切ったらseed付き抽選へ戻る。
公式と同じseedや乱数消費順を再現するという意味ではない。

テストは出典の挙動を基にした局面検証、1000seedの再現、保存状態の復元、秘匿、
ABI・Python境界、不正バッチの原子性、台帳生成の一致を扱う。
旧版Pythonを実行して自動比較したテストや、現行公式との照合実績とは区別する。

```sh
ctest --test-dir build --output-on-failure
python3 data/legacy-neutral-v1/generate.py --check
```

ASan/UBSanビルドではネイティブ検証を実行する。
通常のPythonへASan付き共有ライブラリーを読み込む際のランタイム順序に依存しないよう、
Python接続のCTestは通常ビルドのみで登録している。

共通観測の新しい追加部分は[HP吸収v28](legacy-hp-absorption-profile.md)を参照。旧ABIの幅・行動形式は維持する。
