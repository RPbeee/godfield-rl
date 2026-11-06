# Godfield RL core

God Field を強化学習で扱うための、C++20 製ローカル対戦シミュレーターです。
描画・通信・外部ライブラリに依存せず、自己対戦を高速に回すための基盤です。
C ABI と Python バインディングを備えます。学習器と学習済みモデルは含みません。

> **公式サーバーとの同等性は未検証です。** 公式クライアント JS の計算は独立実装して照合し、
> 一部の対局遷移は実ブラウザの観測で確認していますが、配布・死亡・手番の schedule などは
> 「実験規約」で埋めています。ここで学習した方策が本家で強いとは主張できません。

## 現在の状態(2026-11-06)

| 項目 | 内容 |
| --- | --- |
| 対局エンジン | `official-execution-subset-v71` / Snapshot71 / view-v69、Match ABI2 |
| 対象モデル | 公式台帳 296 モデル。既定 236 モデル・重み 499、`include_fate=True` で 237 モデル・重み 500 |
| 人数 | 2〜9 人・チーム戦(公式での実観測は 2 人・終末なしの CPU 修行のみ) |
| 公式との照合 | 公式 JS を実行した golden vector 50,270 件、状態表示 22,140 件が一致。実対局の遷移 72 件(native 比較 66 件)を固定 |
| テスト | Release CTest 158 件、ASan/UBSan(core・C API・wire)通過 |
| 旧版 | legacy 37 profile の対戦 Env・C ABI・Python は保存(`docs/readme-history.md`) |

### できること
主動作・応答・効果・死亡・補充・次手番・チーム終局を、物理反射/消去/遮断/跳ね返し、FREE、属性付替、倍化、
確率攻撃、反撃、HP 吸収、閃光/暗雲、MP 武器、二回攻撃、全敵化、自動復活、杵臼、両替、売買、病気、Fog、
Dream、178 の redraw などを含めて進められます。守護神・Earth/Moon・現象・悪魔・終末は
外部指定の入力または実験規約で限定的に扱います。
一覧は [限定実行器の契約](docs/official-execution-subset.md) を参照してください。

### まだできないこと
- 公式サーバーの抽選・schedule(対象選択、補充、死亡後の進行)の完全再現
- Dream の公式全候補分布と実体不適合入力、Earth/Moon の一般神器・奇跡選出、207 の公式現象分布
- 全補充経路の悪魔出現・再授与連鎖、完全な終末処理
- 手札満杯(最大 18)の配送、94+Wide232 のうち臼所持者あり・3 人以上の場合
- 3 人以上・チーム戦・終末の実対局による照合

最新の残件表は [仕様差分表](docs/official-spec-gap.md) にあります。

## ビルドと確認

macOS/Linux、CMake 3.20 以上、C++20 コンパイラーが必要です。パッケージのダウンロードは不要です。

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
ctest --test-dir build --output-on-failure
```

メモリ・未定義動作の検証:

```sh
cmake -S . -B build-sanitize -DCMAKE_BUILD_TYPE=RelWithDebInfo -DGF_SANITIZE=ON
cmake --build build-sanitize -j 4
ctest --test-dir build-sanitize --output-on-failure
```

ベンチマーク(試合数・人数・seed・スレッド数)と Python の動作確認:

```sh
./build/godfield_official_bench 1000 2 42 1
./build/godfield_official_bench 1000 9 42 1
PYTHONPATH=python python3 python/official_match.py
```

同じ試合数・seed ならスレッド数を変えても結果・checksum は一致します。
ベンチには行動サンプリング・合法性検証・状態更新・終局後の reset を含み、
観測の出力・Python 通信・モデル推論は含みません。

## API の入口

| 用途 | C++ | C ABI | Python |
| --- | --- | --- | --- |
| 台帳・合成計算(read-only) | `godfield::official` | `gfo_*`(ABI1) | `OfficialData` |
| 状態の読み取り・view・選択照会 | `official_state.hpp` | `gfo_state_*` | `OfficialState` |
| 対局の実行(step/reset/rollout) | `godfield::official::Match` | `gfo_match_*`(ABI2) | `Match` |

```python
from official_match import Match

with Match(seed=42) as match:
    status, rng = match.random_rollout(12345)
    print(status.winners, status.reward_by_player, status.decisions)
```

`Match` は `sample_action` / `random_rollout` で合法手のランダム実行ができます。
Fate(207)・外部補充・混乱など専用の入口が必要な場面は、契約文書を参照してください。
Python は `PYTHONPATH=python` で実行するか、`python` ディレクトリを import パスへ追加します。

## 公式仕様への対応方針

1. **公開クライアントの計算・表示**は、実 JS をオフライン実行した golden vector と照合する
   ([データと出典](data/official-v2/README.md))。
2. **対局の遷移**は、ヘッドレス Chromium で公式の CPU 修行を操作して受信した生の game update を固定し、
   同じ入力から native が同じ状態に到達するかを固定 8 seed で比較する
   ([観測手順](docs/official-headless-probe.md)、[観測記録](docs/official-live-observations.md))。
3. 観測できない部分は**実験規約**として明示し、公式同等とは呼ばない(各契約文書に規約と根拠を併記)。
4. 確率分布は、観測だけで確定できないため、推定を実装に混ぜない。

## ドキュメント

| 資料 | 内容 |
| --- | --- |
| [仕様差分表](docs/official-spec-gap.md) | 主要残件の優先度表と、各版の差分 |
| [限定実行器の契約](docs/official-execution-subset.md) | 対応モデル・規約・検証結果(版ごと) |
| [状態と表示の契約](docs/official-state-view.md) | `OfficialState` の入力・view・未投影項目 |
| [公式一次調査](docs/official-current-rules.md) | 公開クライアント/教典から確認した事実 |
| [適合計画](docs/official-conformance-plan.md) | 未完了条件と今後の計画 |
| [実ブラウザの観測記録](docs/official-live-observations.md) | 実対局の遷移と、各版で追加した観測 |
| [ヘッドレス観測の手順](docs/official-headless-probe.md) | 独立 Chromium による CPU 修行の操作 |
| [296 件の出典と版比較](data/official-v2/README.md) | 台帳・取得 hash・旧版比較・fixture |
| [旧 README(全履歴)](docs/readme-history.md) | v1〜v91 の実装経緯、legacy profile 一覧、各版の使用例と検証記録 |

## ディレクトリ

- `include/godfield/`、`src/` — C++ コア、C ABI、ベンチ
- `python/` — Python バインディング(`official_*.py`、legacy の `*_env.py`)
- `tests/` — C++/Python テスト(wire 照合は `official_wire_trace_test.cpp`)
- `scripts/` — oracle 生成、ヘッドレス probe、受信 parser
- `data/official-v2/` — 296 モデルの台帳、golden vector、実対局の wire/witness
- `docs/` — 契約文書・調査・観測記録

## GitHub(private)

認証済みアカウントで、`.git` への書き込みが可能な端末から:

```sh
sh scripts/publish-private.sh
```

`godfield-rl` を private で作成するか、既存 private リポジトリへ push します。
既存 origin は fetch/push の宛先がこのリポジトリと一致する場合だけ使い、上書きしません。
認証情報とビルド成果物は commit しません。
