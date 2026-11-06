# 公式CPU戦のヘッドレス実測

[official_headless_probe.mjs](../scripts/official_headless_probe.mjs) は独立した一時Chromiumを起動する。通常のFirefoxや既存のログイン済みプロファイルを使わず、ウインドウの前面復帰も必要ない。実行器や学習器の依存ではなく、公式挙動を確認する任意の調査ツールである。

Node.js、Playwrightの `playwright-core` と、その版に対応するChromiumを別途用意する。依存を自動インストールしたり、普段のブラウザ設定を変更したりしない。

```sh
node scripts/official_headless_probe.mjs \
  --playwright /absolute/path/to/node_modules/playwright-core \
  --out-dir /tmp/godfield-private-probe
```

特定の初期手札を探す場合は、同じprobeを操作する [有限探索wrapper](../scripts/find_official_training_hand.mjs) を使える。

```sh
node scripts/find_official_training_hand.mjs \
  --playwright /absolute/path/to/node_modules/playwright-core \
  --out-dir /tmp/godfield-private-search \
  --max-trials 12 --targets 34,83,216
```

`--max-trials` は1〜12、既定12。`--targets` は3/34/83/216から指定し、既定は34/83/216。両替の検証には `--targets 3` を使う。各試行で2人・終末なしの表示設定を確認し、GF1で二回同じ配置になった可視9枚だけを検索する。不一致なら通常退出とメニュー復帰を確認してから次へ進む。試行は退出を含め90秒、各commandは25秒以内で、不明な画面や退出失敗をskipして続行しない。

候補では手札に触れず、prefixと画像を保存して同じブラウザの対話操作へ戻る。試行上限ではメニューへ戻る。探索中の `{"kind":"state"}` は直近の簡潔な探索状況、`{"kind":"abort"}` は直列処理の境界で探索を中止する要求である。探索終了後は通常のprobeのJSONLを渡せる。初期手札を条件に選んだ試行なので、公式の配布率や自然対局の発生率の推定には使わない。

標準入力へJSONを1行ずつ渡す。`state` は画面の本文・表示入力欄・可視画像のpathと矩形を返す。操作後は新しい `state` や `waitText` で結果を確認する。クリックが成功したという応答だけでゲーム操作の受理を認定しない。

```json
{"kind":"state"}
{"kind":"fillName","text":"RulesCheck"}
{"kind":"clickText","text":"生誕する"}
{"kind":"waitText","text":"コンピュータと対戦"}
{"kind":"clickText","text":"修行"}
{"kind":"waitText","text":"戦いを始める"}
{"kind":"screenshot"}
{"kind":"capture"}
```

表示された修行設定の人数・終末条件を確認してから開始する。`clickText` は完全一致する可視要素が一つの場合だけ使える。`clickImage` は直前の `state` に存在した画像pathと矩形を現DOMと再照合し、指定した手札領域内で一意な場合にクリックする。同名の手札・選択済み表示・tooltipを混同しない。

```json
{"kind":"clickImage","path":"/images/items/weapons/bouncing-sword.webp","handRect":{"x":180,"y":490,"w":750,"h":110}}
{"kind":"clickPoint","x":1020,"y":285}
{"kind":"screenshot"}
```

座標は1440×1000の実測例であり、現在の画面を確認して使う。公式clientでは下部の「許す」や攻撃値は表示用summaryで、送信は上側のmain paneのクリック領域に結び付いている。v85では応答側の空白 `(690,250)` で空応答を、攻撃側の空白 `(350,330)` で選択済み二枚の自己使用を送信できた。選択やtarget変更を終えた画面を確認してから確定し、次のGF・資源・受信更新で受理を確認する。これらの座標を全画面・全場面の汎用操作としない。

受信保存にはChrome DevTools Protocolの [Network.streamResourceContent](https://chromedevtools.github.io/devtools-protocol/tot/Network/#method-streamResourceContent) と `dataReceived` を使う。実ブラウザが受けたFirestoreのGET `/channel` 本文だけを順に保存する。通常の `response.body()` が応答完了まで待つ継続接続でも、その時点までのbytesを保存できる。任意のアプリ内部状態の評価、独自のゲームAPI呼出し、コマンド再送は行わない。

`capture` は各streamの不変なprefixファイルとbytes・SHA256・`completed`・`capture_error` を返す。`completed:false, capture_error:false` は正常に受信中のprefixでも生じる。完全なJSON frameの末尾まで届いていても、HTTP応答全体の完了を意味しない。`completed:true` はその保存境界以前に正常な `loadingFinished` を観測した場合だけで、対局の終局を意味しない。

[parse_official_webchannel.py](../scripts/parse_official_webchannel.py) でprefix中の完全なframeだけを解析できる。長さ行はUTF-8 byte数ではなくJavaScriptのUTF-16 code unit数として扱う。不完全な末尾を補完したり、途中からJSONを探索して再同期したりしない。出典にはprefixのbytes/hash、完全frame数・解析済み終端・残りbytes・採用frame番号を残す。未知のgame fieldや文字列はエラーにするため、新しい受信形式は元資料と照合してdecoderを更新する。

```sh
python3 scripts/parse_official_webchannel.py \
  --input /tmp/godfield-private-probe/PREFIX.bin \
  --out-file /tmp/godfield-private-probe/decoded.json \
  --viewer-id 2
```

出力先ディレクトリは0700、本文・画像・解析物は0600で保存する。raw本文には対局識別情報や非公開手札が含まれるため、元ファイルをGitへ追加しない。公開fixtureには匿名化したgame literalと出典hashを採用し、手札の真値は検証用の特権入力として扱う。HTTP header、request body、cookie、HARは保存しない。

終了時は画面の戻る操作から「戦いをあきらめる」を選び、メニューへの復帰を確認して `{"kind":"close"}` を送る。`close` はその時点のprefixを保存して一時ブラウザを終了する。途中退出を勝敗確定や完全なstart→endの証拠に数えない。

両替3の検証では、MPと所持金の各列に `+10 / +1 / -1 / -10` が表示される。v87の1440×1000画面では所持金側 `+10` の中心は `(760,123)`。40/10/20から4回でpreviewが0/10/60になるが、確定までは実statusが40/10/20のままであることを確認した。主動作ペイン `(350,250)` の確定1回で受信更新へ進む。座標だけで再実行せず、現在の画面と各preview値を確認する。終局後の『戦いを終わる』試行ではメニュー復帰を確認できず、その後一時ブラウザを閉じた。終局受信の確認と、終了ボタン後の遷移確認は別に記録する。


v90は追加の公式接続なしで、別セッション取得済み216の自己使用を取り込んだ。抽出済みraw各1frameも同parserで検証し、元prefixの完全frame1/3（0-based）の匿名化gameと一致することを確認した。raw断片のbytes/hashと元prefixのbytes/hash・frame数・採用位置は分けて記録する。詳細は[観測記録v90](official-live-observations.md#作業世代v90取得済み216単品自己使用とnativeの一致)と[公開wire](../data/official-v2/wire-game-updates-v90.json)を参照。
