# ocf_dmcconv

WAV・FLAC・MP3 を、APU の DMC チャンネルで鳴らすサンプル（`.dmc`）に変換するコマンドラインツールです。
`.dmc` は FamiTracker の `.dmc` と同じ、生のビット列だけのファイルです。

変換の仕組みと設計は [docs/dmc-converter-design.md](../../docs/dmc-converter-design.md) にまとめています。

トラッカーの `DPCM Samples` ウィンドウでも、同じ変換で WAV などを直接読み込めます（[tools/tracker/README.md](../tracker/README.md#dpcm)）。このツールは、まとめて変換したいときや、FamiTracker など他のツール向けに `.dmc` を作るときに使います。

## ビルド

```
cmake -S . -B build
cmake --build build --config Debug --target dmcconv
build/tools/dmcconv/Debug/ocf_dmcconv.exe --help
```

ImGui を使わないので、`OCF_ENABLE_IMGUI` に関係なくビルドされます。

## 使い方

```
ocf_dmcconv <入力> [<入力> ...] [オプション]
```

```
# kick.dmc を作る（レート15）
ocf_dmcconv kick.wav

# 入るレートを自動で選び、前後の無音を削って、プレビューも書き出す
ocf_dmcconv kick.wav snare.wav -r auto --trim -o samples/ -p preview/
```

結果は1ファイル1行で表示されます。

```
kick.wav -> kick.dmc  rate 15 (33143.9Hz)  1665 bytes  0.40s  SNR 24.3dB  overload 0.0%
```

| 項目 | 内容 |
| --- | --- |
| `rate` | 変換したレート（0〜15）と周波数。トラッカーでサンプルを鳴らすときに同じレートを指定します |
| `bytes` | `.dmc` の大きさ。最大4081バイト。1曲のサンプルは合わせて16KBまでです |
| `SNR` | 元の波形との差。大きいほど元の音に近い |
| `overload` | 波形の変化に追いつけなかった部分の割合。大きいときは音量を下げるか、レートを上げます |

| オプション | 既定 | 内容 |
| --- | --- | --- |
| `-o, --output <パス>` | 入力と同じフォルダ | 出力先のファイル。入力が複数のときはフォルダ |
| `-r, --rate <0-15 \| auto>` | `15` | レート。`auto` は4081バイトに入る一番高いレート |
| `--normalize` | オフ | ピークを最大音量にそろえる（`--gain` の前に） |
| `-g, --gain <倍率 \| NdB>` | `1.0` | 音量。`0.5` や `-6dB`。1.0 がレベルの振幅63 |
| `--trim [dBFS]` | オフ（−50） | 前後の、しきい値より静かな部分を削る |
| `--center <0-127>` | `64` | 波形の中心のレベル |
| `--rest <0-127>` | `--center` と同じ | 鳴り始めと鳴り終わりのレベル |
| `--no-return` | オフ | 鳴り終わりに `--rest` のレベルへ戻さない |
| `--method <optimal \| greedy>` | `optimal` | ビット列の決め方。`optimal` は誤差が最小、`greedy` は FamiTracker と同じ考え方 |
| `--truncate` | オフ | 4081バイトに入らないとき、エラーにせず切り詰める |
| `-p, --preview <パス>` | なし | 変換結果を44.1kHzの WAV に戻して書き出す（`<名前>.preview.wav`） |
| `-q, --quiet` | オフ | エラーだけを表示する |

- 変換に失敗したファイルがあっても、ほかのファイルは変換します。終了コードは、すべて成功で0、失敗があれば1、引数の誤りで2です。
- 4081バイトに入らないときは、入るレートを表示します（例: `too long for rate 15 (max 4081 bytes). Rate 11 fits, or truncate it`）。

## 音量とレートの決め方

DMC は1bitごとにレベルを±2しか動かせないので、大きくて高い音は波形に追いつけずに歪みます。
レート15で最大音量だと、正弦波で約170Hzまでしか追いつけません。

- **キック・ベース**: 低い音なので、`--normalize` で大きくしても追いつけます。
- **スネア・ハイハット・声**: 高い成分が多いので、`overload` を見ながら `--gain` で下げます（`-6dB`〜`-12dB` くらいから）。レートは高いままにします。
- **長い音**: レートを下げると長く入りますが、高い音が消え、傾きの限界も下がります。まず `-r auto` で試し、`overload` が大きければ音量を下げるか、素材を短くします。

迷ったら `-p` でプレビューを書き出して聞き比べてください。
