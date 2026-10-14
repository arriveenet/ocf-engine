# トラッカー楽曲フォーマット（JSON, v1）

## 概要

トラッカー（`tools/tracker`）で作る楽曲（`ocf::audio::tracker::Song`）を保存するためのJSONフォーマットです。
拡張子は `.json` とし、文字コードはUTF-8（BOMなし）です。読み込みでは、BOMが付いていても受け付けます。

読み書きは `ocf/audio/tracker/SongSerializer.h` に実装しています（[API](#api)）。JSONの解析と文字列のエスケープには [yyjson](https://github.com/ibireme/yyjson) を使います。
ノートやエフェクトなどの文字列表記は `ocf/audio/tracker/SongNotation.h` にあり、エディタと共通です。

### 方針

- 人が読めて、差分が見やすいこと。ノートは `C-4`、エフェクトは `F06` のように、トラッカーの画面と同じ表記にします。
- ファイルが小さいこと。空のパターンと空の行は書きません（疎表現）。
- どのJSONライブラリでも扱えること。使う型は object・array・string・整数（32ビットに収まる範囲）だけです。
  浮動小数点数、64ビット整数、コメント、末尾のカンマは使いません。
- 前方互換。読み込み側は、知らないキーを無視します。

## 全体の構造

```json
{
  "format": "ocf-tracker-song",
  "version": 1,
  "title": "Demo",
  "author": "",
  "speed": 6,
  "tempo": 150,
  "rowsPerPattern": 64,
  "channels": [ ... ],
  "instruments": [ ... ],
  "frames": [ ... ],
  "patterns": [ ... ],
  "dpcmSamples": [ ... ]
}
```

| キー | 型 | 必須 | 内容 | `Song` のメンバ |
| --- | --- | --- | --- | --- |
| `format` | string | ○ | 常に `"ocf-tracker-song"` | — |
| `version` | 整数 | ○ | フォーマットのバージョン。この文書は `1` | — |
| `title` | string | | 曲名。省略時は `""` | `title` |
| `author` | string | | 作者。省略時は `""` | `author` |
| `speed` | 整数 | | 1行あたりのtick数（1〜31）。省略時は `6` | `speed` |
| `tempo` | 整数 | | テンポ（32〜255）。省略時は `150` | `tempo` |
| `rowsPerPattern` | 整数 | ○ | 1パターンの行数（1〜256） | `rowsPerPattern` |
| `channels` | array | ○ | チャンネルの設定。[channels](#channels) | `effectColumns` |
| `instruments` | array | | インストゥルメント。[instruments](#instruments)。省略時は空 | `instruments` |
| `frames` | array | ○ | 再生順。[frames](#frames) | `frames` |
| `patterns` | array | | パターンの中身。[patterns](#patterns)。省略時はすべて空 | `patterns` |
| `dpcmSamples` | array | | DPCMのサンプル。[dpcmSamples](#dpcmsamples)。省略時は空 | `dpcmSamples` |

テンポの計算はFamiTrackerと同じです。1秒あたり約60tick（NTSC）で進み、`tempo * 24 / speed` を1tickごとに `3600` から引いていき、0以下になったら次の行に進みます。
`speed = 6`、`tempo = 150` のとき、6tickで1行進みます。

### `channels`

チャンネルごとの設定です。v1では、次の5要素をこの順番で必ず書きます。

```json
"channels": [
  { "type": "pulse1", "effectColumns": 1 },
  { "type": "pulse2", "effectColumns": 1 },
  { "type": "triangle", "effectColumns": 1 },
  { "type": "noise", "effectColumns": 1 },
  { "type": "dpcm", "effectColumns": 1 }
]
```

| キー | 型 | 必須 | 内容 |
| --- | --- | --- | --- |
| `type` | string | ○ | `pulse1` `pulse2` `triangle` `noise` `dpcm` |
| `effectColumns` | 整数 | | エディタに表示するエフェクト列の数（1〜4）。省略時は `1`。再生には影響しません |

`type` を書くのは、将来の拡張音源（VRC6など）でチャンネルが増えたときに、何のチャンネルかを判別できるようにするためです。
以降の `frames` と `patterns` の `channel` は、この配列のインデックスを指します。

> `dpcm` チャンネルは、インストゥルメントの [DPCMのキー割り当て](#dpcmのキー割り当て) でノートごとにサンプルを鳴らします。

### `instruments`

```json
"instruments": [
  {
    "index": 0,
    "name": "Lead",
    "sequences": {
      "volume": { "values": [15, 13, 11, 10, 9], "loop": 4, "release": -1 },
      "duty": { "values": [2], "loop": -1, "release": -1 }
    }
  }
]
```

| キー | 型 | 必須 | 内容 |
| --- | --- | --- | --- |
| `index` | 整数 | ○ | インストゥルメント番号（0〜63）。パターンの `instrument` から参照されます |
| `name` | string | | 名前。省略時は `""` |
| `sequences` | object | | シーケンス。キーはシーケンスの種類。省略したシーケンスは空 |
| `dpcm` | array | | DPCMのキー割り当て。[後述](#dpcmのキー割り当て)。省略時は割り当てなし |

- `index` に欠番があってもかまいません。読み込み時は、欠番の位置に空のインストゥルメントを作ります。
- 同じ `index` が2回出てきたらエラーです。

シーケンスの種類（`sequences` のキー）:

| キー | 値の範囲 | 内容 |
| --- | --- | --- |
| `volume` | 0〜15 | 音量。空のときは常に15 |
| `duty` | 0〜3 | Pulse: デューティ比（0=12.5%, 1=25%, 2=50%, 3=75%）。Noise: bit0がモード（1=短周期）。空のときは常に0 |

知らない種類のキー（将来の `arpeggio` `pitch` など）は無視します。

シーケンス:

| キー | 型 | 必須 | 内容 |
| --- | --- | --- | --- |
| `values` | 整数の配列 | ○ | 1tickごとの値。範囲はシーケンスの種類ごと（上の表）。要素数は256以下 |
| `loop` | 整数 | | ループ点のインデックス。`-1` または省略でなし |
| `release` | 整数 | | リリース点のインデックス。`-1` または省略でなし |

シーケンスの進み方（`ocf::audio::tracker::Sequence` のコメントと同じです）:

1. 発音時に先頭から1tickに1つずつ進みます。
2. ノートが鳴っている間は、リリース点で待ちます。ループ点がリリース点より前にあれば、ループ点とリリース点の間を繰り返します。
3. ノートがリリースされる（`===`）と、リリース点の次から続けます。
4. 末尾に来たら、ループ点があれば（かつリリース点より後ろにあれば）ループ点に戻り、なければ最後の値を保ちます。

エディタでは、FamiTracker形式のMML（`15 13 | 11 10 / 9 0`。`|` がループ点、`/` がリリース点の直前に入る）で入力します（`sequenceToMml()` / `sequenceFromMml()`）。

#### DPCMのキー割り当て

`dpcm` チャンネルでこのインストゥルメントのノートを鳴らしたときに、どのサンプルをどのレートで鳴らすかです（FamiTracker と同じ考え方）。割り当てたノートだけを、ノートの順に書きます。

```json
"dpcm": [
  { "note": "C-3", "sample": 0, "rate": 15 },
  { "note": "D-3", "sample": 1, "rate": 13, "loop": true }
]
```

| キー | 型 | 必須 | 内容 | `DpcmKey` のメンバ |
| --- | --- | --- | --- | --- |
| `note` | string | ○ | ノート（`C-0`〜`B-7`） | `Instrument::dpcmKeys` の位置 |
| `sample` | 整数 | ○ | `dpcmSamples` の `index` | `sample` |
| `rate` | 整数 | | レート（0〜15。15が最も速い）。省略時は `15` | `rate` |
| `loop` | bool | | 末尾まで来たら先頭に戻る。省略時は `false`（書くときも `true` のときだけ書きます） | `loop` |

- 同じ `note` が2回出てきたら、または無いサンプルを指したらエラーです。
- 鳴らし方: ノートで `$4011` にサンプルの `restLevel`、`$4010`〜`$4013` にレート・ループ・アドレス・長さを書いて、サンプルを先頭から鳴らします。
  - `---` で止めます。止めても、読み込み済みの最大16bitは鳴り、レベルはそこで止まった値のままです。
  - `===` でループをやめ、サンプルの末尾まで鳴らします。
  - 割り当てのないノートでは、鳴っているサンプルを止めます。
- 音量列とシーケンスは、DPCMでは使いません。

### `frames`

再生順です。1要素が1フレームで、各チャンネルが再生するパターン番号を `channels` の順に並べます。

```json
"frames": [
  [0, 0, 0, 0, 0],
  [1, 0, 1, 0, 0]
]
```

- フレーム数は1〜256。各要素は `channels` と同じ数（v1では5）の整数（0〜255）です。
- 最後のフレームの次は、先頭のフレームに戻ります（`Bxx` で戻り先を変えられます）。
- `patterns` に無いパターン番号を指してもかまいません。空のパターンとして扱います。

### `patterns`

パターンの中身です。**空でないパターンだけ**を書きます。

```json
"patterns": [
  {
    "channel": 0,
    "index": 0,
    "rows": [
      { "row": 0, "note": "C-4", "instrument": 0, "volume": 15, "effects": ["F06"] },
      { "row": 4, "note": "E-4" },
      { "row": 8, "note": "---" },
      { "row": 12, "effects": ["...", "B01"] }
    ]
  }
]
```

| キー | 型 | 必須 | 内容 |
| --- | --- | --- | --- |
| `channel` | 整数 | ○ | `channels` のインデックス |
| `index` | 整数 | ○ | パターン番号（0〜255）。チャンネルごとに独立した番号です |
| `rows` | array | ○ | 空でない行だけを、`row` の昇順に並べます |

行:

| キー | 型 | 必須 | 内容 | 省略時 |
| --- | --- | --- | --- | --- |
| `row` | 整数 | ○ | 行番号（0〜`rowsPerPattern - 1`） | — |
| `note` | string | | ノート（下表） | 空 |
| `instrument` | 整数 | | インストゥルメント番号（0〜63） | 空 |
| `volume` | 整数 | | 音量（0〜15） | 空 |
| `effects` | stringの配列 | | エフェクト列。配列のインデックスが列番号（0〜3）。途中の空の列は `"..."`、末尾の空の列は書きません | 空 |

- 同じ `(channel, index)` のパターンが2回出てきたら、または同じ `row` が2回出てきたらエラーです。
- エディタの表示とは違い、インストゥルメント番号は10進数で書きます。

ノートの表記（`noteToString()` / `noteFromString()`）:

| 表記 | 意味 | `Cell::note` |
| --- | --- | --- |
| `C-0` 〜 `B-7` | 音名・`-` または `#`・オクターブ。音名は大文字、`#` が使えるのは C D F G A のみ | 1〜96 |
| `0-#` 〜 `F-#` | ノイズチャンネルのみ。ノイズの高さ（16進1桁。FamiTrackerと同じく `F-#` が最も高く、レジスタ `$400E` の周期インデックスは `15 - 値`） | 1〜16 |
| `---` | ノートオフ（消音） | `NOTE_OFF` |
| `===` | ノートリリース（シーケンスをリリース点の次へ進める） | `NOTE_RELEASE` |

`"..."` は空として読めますが、書くときはキーごと省略します。

エフェクトの表記（`effectToString()` / `effectFromString()`）: 大文字1文字＋16進2桁。書くときは大文字、読むときは16進の部分のみ小文字も受け付けます。

| 表記 | 内容 |
| --- | --- |
| `Fxx` | xxが `00`〜`1F`: speedを変える、`20`〜`FF`: tempoを変える（`F00` は何もしない） |
| `Bxx` | この行の後、フレームxxに移る |
| `Dxx` | この行の後、次のフレームの行xxに移る |
| `Cxx` | この行の後、曲を止める |

### `dpcmSamples`

DMCチャンネルのサンプルです。曲のファイルに埋め込むので、ゲームは曲のファイル1つを読めば鳴らせます。
データは `.dmc` ファイルと同じバイト列（下位ビットから使う1bitのデルタ）です。作り方は [dmc-converter-design.md](dmc-converter-design.md) を参照してください。

```json
"dpcmSamples": [
  { "index": 0, "name": "Kick", "rate": 15, "restLevel": 64, "data": "VVVVqqqq..." }
]
```

| キー | 型 | 必須 | 内容 | `DpcmSample` のメンバ |
| --- | --- | --- | --- | --- |
| `index` | 整数 | ○ | サンプル番号（0〜63）。キー割り当ての `sample` から参照されます | — |
| `name` | string | | 名前。省略時は `""` | `name` |
| `rate` | 整数 | | 作ったときのレート（0〜15）。キーに割り当てるときの初期値。省略時は `15` | `rate` |
| `restLevel` | 整数 | | 鳴らし始めるレベル（0〜127）。鳴らすときに `$4011` に書きます。省略時は `64` | `restLevel` |
| `data` | string | ○ | バイト列のbase64（パディング付き）。`16n + 1` バイトで4081バイト以下。`""` は空のサンプル | `data` |

- `index` に欠番があってもかまいません。読み込み時は、欠番の位置に空のサンプルを作ります。
- サンプルは、`index` の順に64バイト境界で16KBのメモリ（`$C000`〜`$FFFF`）に並べます（`Song::getDpcmSampleOffset()`）。合計（各サンプルを64バイトに切り上げた和）が16384バイトを超えたらエラーです。
- サンプルがあるときだけ書きます。データが長いので、最後に書きます。

## 読み込み時の検証

次の場合は読み込みをエラーにします。エラーメッセージは、問題の場所（JSONポインタ形式）で始まります。

```
/patterns/3/rows/7/note: invalid note "H-4"
/speed: must be between 1 and 31
/instruments/1/index: duplicate instrument 0
/dpcmSamples/0/data: must be 16n + 1 bytes, at most 4081 (not 2)
invalid JSON at line 12, column 5: unexpected character, expected a string key
```

- JSONとして不正
- `format` が `"ocf-tracker-song"` でない
- `version` が `1` より大きい（新しいバージョンで保存されたファイル）
- 必須のキーが無い、型が違う
- 値が範囲外、表記が不正（`noteFromString()` などが `std::nullopt` を返す）
- 番号の重複（インストゥルメントの `index`、パターンの `(channel, index)`、行の `row`、サンプルの `index`、キー割り当ての `note`）
- シーケンスの `loop` / `release` が `values` の範囲外
- DPCMのサンプルが長さの条件を満たさない、合計が16KBを超える、キー割り当てが無いサンプルを指す

範囲外の値を丸めて読み込むことはしません。手で編集したファイルの誤りに気付けるようにするためです。

## 書き出しのレイアウト

差分を見やすくするため、書き出す内容とレイアウトは常に同じになるようにしています。

- キーは、この文書の表の順に並べます。`title` `author` `speed` `tempo` は、既定値でも省略せずに書きます。シーケンスの `loop` `release` も、`-1` のときも書きます。
- インデントは2スペースです。
- 次の値は1行に書きます。それ以外は、1要素を1行に書きます。
  - スカラー値だけを持つ配列・オブジェクト（チャンネル、フレーム、`effects`）
  - スカラー値と「スカラー値だけの配列」を持つオブジェクト（行、シーケンス、キー割り当て、サンプル）
- 1行に書くオブジェクトは `{ "key": value, ... }`、配列は `[1, 2, 3]` の形です。
- 末尾は改行で終わります。

書き出したものを読み込んで、もう一度書き出すと、同じ文字列になります。

## API

```cpp
#include <ocf/audio/tracker/SongSerializer.h>

namespace ocf::audio::tracker {

std::string saveSongToString(const Song& song);
Result<Song, std::string> loadSongFromString(std::string_view json);

Result<void, std::string> saveSong(const Song& song, const std::filesystem::path& path);
Result<Song, std::string> loadSong(const std::filesystem::path& path);

} // namespace ocf::audio::tracker
```

- エラーは `Result` の `Err` で、[読み込み時の検証](#読み込み時の検証)のメッセージを返します。例外は使いません。
- `saveSong()` は、親ディレクトリが無ければ作ります。いったん `<ファイル名>.tmp` に書き出してから置き換えるので、書き込みに失敗しても元のファイルは壊れません。
- 実装は `engine/src/audio/tracker/SongSerializer.cpp` です。yyjsonはエンジンに `PRIVATE` でリンクし、公開ヘッダにはyyjsonの型を出しません。

ゲームから読み込む例:

```cpp
auto result = audio::tracker::loadSong(FileSystem::getInstance()->getAssetFullPath("songs/stage1.json"));
if (result.isErr()) {
    OCF_LOG_ERROR("{}", result.unwrapErr());
    return;
}
audioSystem.tracker_setSong(bgm, std::make_shared<const audio::tracker::Song>(std::move(result).unwrap()));
```

## 完全な例

Pulse1でメロディ、Triangleでベースを鳴らす2フレームの曲です。
`saveSongToString()` はこの曲をこの通りの文字列で書き出します（`engine/test/test_song_serializer.cpp` で確認しています）。

```json
{
  "format": "ocf-tracker-song",
  "version": 1,
  "title": "Example",
  "author": "ocf",
  "speed": 6,
  "tempo": 150,
  "rowsPerPattern": 16,
  "channels": [
    { "type": "pulse1", "effectColumns": 1 },
    { "type": "pulse2", "effectColumns": 1 },
    { "type": "triangle", "effectColumns": 1 },
    { "type": "noise", "effectColumns": 1 },
    { "type": "dpcm", "effectColumns": 1 }
  ],
  "instruments": [
    {
      "index": 0,
      "name": "Lead",
      "sequences": {
        "volume": { "values": [15, 13, 11, 10, 9, 8], "loop": -1, "release": -1 },
        "duty": { "values": [2], "loop": -1, "release": -1 }
      }
    },
    {
      "index": 1,
      "name": "Bass",
      "sequences": {
        "volume": { "values": [15], "loop": -1, "release": -1 }
      }
    }
  ],
  "frames": [
    [0, 0, 0, 0, 0],
    [1, 0, 0, 0, 0]
  ],
  "patterns": [
    {
      "channel": 0,
      "index": 0,
      "rows": [
        { "row": 0, "note": "C-4", "instrument": 0 },
        { "row": 4, "note": "E-4" },
        { "row": 8, "note": "G-4" },
        { "row": 12, "note": "---" }
      ]
    },
    {
      "channel": 0,
      "index": 1,
      "rows": [
        { "row": 0, "note": "B-3", "instrument": 0, "volume": 12 },
        { "row": 8, "note": "D-4" },
        { "row": 15, "effects": ["B00"] }
      ]
    },
    {
      "channel": 2,
      "index": 0,
      "rows": [
        { "row": 0, "note": "C-3", "instrument": 1 },
        { "row": 8, "note": "G-2" }
      ]
    }
  ]
}
```
