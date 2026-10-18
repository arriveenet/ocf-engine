# トラッカー（APU音楽エディタ）

## 概要

エンジンのNES APUエミュレーション（`engine/src/audio/apu/`）で鳴らす曲を作る、FamiTracker風のトラッカーです。
エディタ本体は `tools/tracker`（実行ファイル `ocf_tracker`）で、GUIはDear ImGuiで作っています。画面の表示は英語です。
作った曲はJSONファイルに保存し（フォーマットは [tracker-format.md](tracker-format.md)）、エンジンの `AudioSystem` からゲーム中でも再生できます。

ツールの使い方は [tools/tracker/README.md](../tools/tracker/README.md) にまとめています。この文書は、主にエンジン側の構成の説明です。

現在は最小構成です。対応している機能と、まだの機能は[機能一覧](#機能一覧)を参照してください。

```
cmake -S . -B build
cmake --build build --config Debug --target tracker
build/tools/tracker/Debug/ocf_tracker.exe
```

ImGuiを使うので、`OCF_ENABLE_IMGUI=OFF` のときはビルドされません。
起動するとデモ曲が読み込まれた状態になります（`File` → `Open Demo Song` でも開けます）。F5で再生できます。
サンプル曲として、パッヘルベルの「カノン」（`assets/songs/canon_in_d.json`）と、バッハの「G線上のアリア」（`assets/songs/air_on_the_g_string.json`）もあります。`File` → `Open...` で開けます。

## 画面

| ウィンドウ | 内容 |
| --- | --- |
| `Song` | タイトル、作者、スピード、テンポ、1パターンの行数 |
| `Frames` | 再生順（フレーム）と、各チャンネルが再生するパターン番号。クリックしたフレームが `Pattern` ウィンドウに表示されます |
| `Instruments` | インストゥルメントの一覧と、音量・デューティのシーケンス |
| `Pattern` | カーソルのあるフレームのパターン。ノートなどを打ち込みます |

1チャンネル分の表示は `C-4 00 F F06` の形で、左から ノート・インストゥルメント（16進2桁）・音量（16進1桁）・エフェクト です。
エフェクト列の数（1〜4）は、チャンネル名を右クリックして変えます。

- ノート: `C-4` `C#4` など。ノイズチャンネルは `0-#`〜`F-#`（`F-#` が最も高い）。`---` はノートオフ、`===` はノートリリース
- 4行ごと・16行ごとに行の色が変わります
- 再生中の行は紫、カーソルの行は青（編集モード中は赤）で表示されます

### ファイル

`File` メニューで、曲の作成・読み込み・保存をします。

| メニュー | キー | 内容 |
| --- | --- | --- |
| `New` | `Ctrl+N` | 新しい曲 |
| `Open...` | `Ctrl+O` | JSONファイルを開く |
| `Open Demo Song` | | デモ曲を開く |
| `Save` | `Ctrl+S` | 上書き保存。まだ保存していない曲は `Save As...` と同じ |
| `Save As...` | `Ctrl+Shift+S` | 名前を付けて保存。拡張子を省略すると `.json` を付けます |

- ファイルの選択は、ツール内のダイアログで行います。フォルダの移動（一覧のフォルダをクリック、`Up`、パスの直接入力）と、`.json` ファイルの選択ができます。
- 最初は `assets/songs` を開きます（無ければ保存時に作ります）。ゲームからは `FileSystem::getAssetFullPath("songs/...")` で読み込めます。
- 既にあるファイルに保存するときは、置き換えてよいか確認します。
- 変更があるときに `New` や `Open...` を選ぶと、変更を破棄してよいか確認します。ウィンドウを閉じるときは確認しないので、先に保存してください。
- メニューバーの右端に、ファイル名（未保存なら `(untitled)`）と、変更があるときは `*` を表示します。
- 読み込みに失敗したときは、問題の場所を示すメッセージを表示します（例: `/patterns/3/rows/7/note: invalid note "H-4"`）。

### インストゥルメント

シーケンスはFamiTracker形式のMMLで入力します。値は1tickごと（約1/60秒）に進みます。

```
15 14 12 10 | 8 6 / 4 2 0
```

- `|` の後ろの値がループ点です。末尾まで進むとループ点に戻ります。
- `/` の後ろの値がリリース点です。ノートが鳴っている間はリリース点で止まり（ループ点がリリース点より前にあれば、その間を繰り返し）、`===` が来るとリリース点の次から進みます。
- 音量は0〜15、デューティは0〜3（0=12.5%, 1=25%, 2=50%, 3=75%）。範囲外の値は丸めます。
- 空のシーケンスは、音量なら常に15、デューティなら常に0として扱います。

`Preview (C-4)` ボタンを押している間、選んでいるインストゥルメントでPulse 1が鳴ります。

## キー操作

キー操作は、テキスト入力欄を編集していないときに有効です。どのウィンドウにフォーカスがあっても、パターンへの入力になります。
ツールの中では `Help` → `Keyboard Shortcuts` で一覧を見られます。

| キー | 操作 |
| --- | --- |
| `Z S X D C V G B H N J M` `, L . ; /` | ノート入力（現在のオクターブから） |
| `Q 2 W 3 E R 5 T 6 Y 7 U` `I 9 O 0 P` | ノート入力（1オクターブ上から） |
| `1` | ノートオフ（`---`） |
| `-` | ノートリリース（`===`） |
| `0`〜`9` `A`〜`F` | インストゥルメント・音量・エフェクト値の入力（16進） |
| `F` `B` `D` `C` | エフェクトの種類（エフェクト列の1文字目） |
| `Delete` | カーソル位置を消去（編集モード中）。ノート列ではノート・インストゥルメント・音量を消去 |
| 矢印キー | カーソル移動（上下はフレームをまたぐ） |
| `PageUp` / `PageDown` | 16行移動 |
| `Home` / `End` | 先頭 / 末尾の行 |
| `Tab` / `Shift+Tab` | 次 / 前のチャンネル |
| `Space` | 編集モードの切り替え |
| テンキー `/` `*` | オクターブを下げる / 上げる |
| `Enter` | 再生 / 停止（カーソルのフレームの先頭から） |
| `F5` | 最初から再生 |
| `F6` | カーソルのフレームの先頭から再生 |
| `F7` | カーソル位置から再生 |
| `F8` | 停止 |
| `Ctrl+N` / `Ctrl+O` | 新しい曲 / 開く |
| `Ctrl+S` / `Ctrl+Shift+S` | 保存 / 名前を付けて保存 |

- 編集モードがオフのときは、ノートのキーを押すと試聴だけします（キーを離すと止まる）。オンのときは、試聴してからセルに書き込み、`Step` の行数だけカーソルを進めます。
- ノート以外の列（インストゥルメント・音量・エフェクト）への入力と `Delete` は、編集モード中だけ有効です。
- ノートを入力すると、選択中のインストゥルメントも一緒に書き込みます。
- キーは物理的な位置で判定するので、日本語キーボードでも上の表と同じ位置のキーです。
- チャンネル名をクリックすると、そのチャンネルをミュートします。

ImGuiの標準では `Tab` でウィジェット間のフォーカスが移動しますが、トラッカーではチャンネル移動に使うため、この機能を無効にしています（`tools/tracker/main.cpp`）。

## エフェクト

| エフェクト | 内容 |
| --- | --- |
| `Fxx` | `xx` が `01`〜`1F` ならスピード、`20`〜`FF` ならテンポを変える |
| `Bxx` | この行の後、フレーム `xx` に移る |
| `Dxx` | この行の後、次のフレームの行 `xx` に移る |
| `Cxx` | この行の後、曲を止める |

テンポの計算はFamiTrackerと同じです。スピード6・テンポ150のとき、6tick（約0.1秒）で1行進みます。

## 機能一覧

| 機能 | 状態 |
| --- | --- |
| Pulse 1 / Pulse 2 / Triangle | 再生できる |
| Noise | 再生できる |
| DPCM | 再生できる。インストゥルメントのキー割り当てで、ノートごとにサンプル・レート・ループを選ぶ |
| インストゥルメント | 音量・デューティのシーケンス |
| エフェクト | `Fxx` `Bxx` `Dxx` `Cxx` |
| ファイルの保存・読み込み | JSON（[tracker-format.md](tracker-format.md)） |
| アルペジオ・ピッチのシーケンス、ピッチ系エフェクト（`0xy` `1xx` `2xx` `3xx` `4xy` など） | 未実装 |
| Undo / Redo、範囲選択、コピー・貼り付け | 未実装 |

## エンジン側の構成

### クラス

| ファイル | 内容 |
| --- | --- |
| `engine/include/ocf/audio/tracker/TrackerTypes.h` | チャンネル・エフェクトなどの種類、定数 |
| `engine/include/ocf/audio/tracker/Song.h` | 曲のデータ（`Song` `Pattern` `Cell` `Instrument` `Sequence`） |
| `engine/include/ocf/audio/tracker/SongNotation.h` | ノート・エフェクト・シーケンスの文字列表記。ファイルフォーマットとエディタで共通 |
| `engine/include/ocf/audio/tracker/SongSerializer.h` | 曲のJSONの読み書き（実装は `engine/src/audio/tracker/SongSerializer.cpp`、yyjsonを使用） |
| `engine/src/audio/tracker/SongPlayer.h` / `.cpp` | 曲を再生する。1tickごとに行を進め、APUのレジスタを書く |
| `engine/src/audio/tracker/SongSynthesizer.h` / `.cpp` | APUと `SongPlayer` を持ち、1サンプルずつ音を作る（APUのクロック、tick、DCカット）。リアルタイム再生と書き出しで共用 |
| `engine/include/ocf/audio/tracker/SongExporter.h` | 曲をWAVに書き出す。ループ回数とフェードアウトを指定でき、進捗の通知と中止ができる |
| `engine/src/audio/tracker/NoteTable.h` / `.cpp` | ノートからAPUのタイマー周期への変換（NTSC） |
| `engine/src/audio/AudioTrackerSource.h` / `.cpp` | 専用の `SongSynthesizer` を持つオーディオソース。メインスレッドからの操作を、オーディオスレッドで反映する |
| `engine/include/ocf/audio/AudioSystem.h` | `createTrackerSource()` と `tracker_*()` 関数 |

### 使い方（ゲームから再生する）

```cpp
#include <ocf/audio/AudioSystem.h>
#include <ocf/audio/tracker/SongSerializer.h>
#include <ocf/platform/FileSystem.h>

auto result = audio::tracker::SongSerializer::load(FileSystem::getInstance()->getAssetFullPath("songs/stage1.json"));
if (result.isErr()) {
    OCF_LOG_ERROR("{}", result.unwrapErr());
    return;
}

auto& audioSystem = engine.getAudioSystem();
audio::AudioSourceHandle bgm = audioSystem.createTrackerSource();
audioSystem.play(bgm); // ソースをミキサーに載せる（曲はまだ鳴らない）

audioSystem.tracker_setSong(bgm, std::make_shared<const audio::tracker::Song>(std::move(result).unwrap()));
audioSystem.tracker_play(bgm); // 曲の先頭から再生

// 終了時
audioSystem.stop(bgm);
audioSystem.destroySource(bgm);
```

- `tracker_stop()` は曲を止めて無音にするだけで、ソースはミキサーに載ったままです。停止中も `tracker_playNote()` で音を鳴らせます。
- 渡した `Song` はオーディオスレッドと共有されるので、渡した後に変更してはいけません。変更したいときは、コピーを作って `tracker_setSong()` で差し替えます（エディタも編集のたびにそうしています）。

### スレッド

```
メインスレッド                              オーディオスレッド
AudioSystem::tracker_*()                   AudioTrackerSource::render()
  └ コマンドをキューに積む ──(mutex)──▶     1. キューのコマンドを SongPlayer に反映
                                           2. サンプルごとにAPUをクロックし、
                                              29780.5サイクル（NTSCの1フレーム）ごとに SongPlayer::tick()
                                           3. 再生位置をアトミック変数に書く
AudioSystem::tracker_getStatus()  ◀────── 
AudioSystem::update()
  └ AudioTrackerSource::update(): 差し替えられた古い Song を解放
```

- APUのレジスタは、すべてオーディオスレッドで、APUをクロックする合間に書きます。`apu_writeRegister()` のようにメインスレッドから直接書くことはしません。
- 差し替えられた古い `Song` は、オーディオスレッドでメモリを解放しないように、退避しておいてメインスレッドの `update()` で解放します。
- `tracker_getStatus()` が返すのは、直前の `render()` の時点の再生位置です（最大でオーディオのバッファ1つ分遅れます）。
- APUの出力は常に正の値なので、1次のハイパスフィルタ（48kHzで約38Hz）で直流成分を取り除いてから出力します。再生の開始・停止時のプチノイズを防ぐためです。

### APUへの書き込み

| チャンネル | レジスタ |
| --- | --- |
| Pulse | `$4000` = デューティ・長さカウンタ停止・定音量・音量（毎tick）、`$4001` = スイープ無効（最初の1回）、`$4002` = 周期の下位（毎tick）、`$4003` = 周期の上位（発音時と、上位ビットが変わったときだけ） |
| Triangle | `$4008` = 鳴らすとき `$FF`、止めるとき `$00`（毎tick）、`$400A` = 周期の下位、`$400B` = 周期の上位（発音時、無音から鳴り始めるとき、上位ビットが変わったとき） |
| Noise | `$400C` = 定音量・音量、`$400E` = モード・周期、`$400F` = 発音時 |
| 共通 | `$4015` = `$0F`（最初の1回） |

音量は、インストゥルメントの音量シーケンスとパターンの音量列を掛けて求めます（`シーケンス × 音量列 / 15`。どちらも0でないのに0になる場合は1）。
ソフトウェアで音量を制御するので、APUのエンベロープと長さカウンタは使いません。

## テスト

| ファイル | 内容 |
| --- | --- |
| `engine/test/test_tracker_song.cpp` | `Song` のヘルパ |
| `engine/test/test_tracker_notation.cpp` | 文字列表記の変換と往復 |
| `engine/test/test_song_player.cpp` | `SongPlayer` が書くレジスタ（テンポ、エフェクト、シーケンス、ミュート、プレビュー） |
| `engine/test/test_audio_tracker_source.cpp` | APUを通した出力（音が出るか、音程、停止後の無音、古い曲の解放） |
| `engine/test/test_apu.cpp` | APUの矩形波・三角波・スイープ（周期、`$4002` だけでの周期変更、ミュート条件） |
| `engine/test/test_song_serializer.cpp` | JSONの読み書き（設計書の例と同じ文字列になるか、往復、検証エラーとメッセージ、ファイル） |
| `engine/test/test_sample_songs.cpp` | `assets/songs` のサンプル曲（読み込めるか、書き出し形式のままか、ループの流れ） |
| `engine/test/test_song_exporter.cpp` | WAVの書き出し（長さの見積もり、ループ回数と `Bxx`、ヘッダー、フェードアウト、中止、リアルタイム再生と同じサンプルになるか） |

`SongPlayer` などの内部クラスをテストするため、テストのターゲットには `engine/src` もインクルードパスに入れています。
