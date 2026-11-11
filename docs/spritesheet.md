# SpriteSheet

## 概要

スプライトシート（テクスチャアトラス）は、複数の画像を 1 枚のテクスチャに詰め込み、名前でフレームを取り出して描画する仕組みである。

[TexturePacker](https://www.codeandweb.com/texturepacker) の次の 2 形式を読み込める（Phaser や PixiJS などでも使われる汎用形式）。

- **JSON (Hash)**: `frames` がフレーム名をキーとするオブジェクト
- **JSON (Array)**: `frames` が配列で、各要素の `filename` がフレーム名

TexturePacker の **回転（rotated）**、**トリミング（trimmed）**、**pivot** に対応する。ポリゴンメッシュ（`vertices`）は無視し、矩形として描画する。

## 主要コンポーネント

TileMap と同様に、「データ」「読み込み」「描画」の 3 つに責務を分離して構成する。

```
SpriteSheetLoader (.json を解析) ─▶ SpriteSheet (SpriteFrame の集合) ─▶ Sprite::setSpriteFrame() (描画)
```

| コンポーネント | 役割・責務 |
| --- | --- |
| **SpriteFrame** | 1 フレームの情報。アトラス上の領域、回転の有無、トリミング前の画像内での位置（`offset`）と元サイズ（`sourceSize`）、pivot を持つ。 |
| **SpriteSheet** | 名前付きフレームとテクスチャを保持する**データ**。GPU には依存しない。`Ref<SpriteSheet>` で共有する。テクスチャは所有しない（`TextureManager` が所有）。 |
| **SpriteSheetLoader** | フォーマットを判定し、対応する reader で `SpriteSheet` を作る。画像は `TextureManager` で読み込む。 |
| **Sprite** | `SpriteFrame` を描画する 2D コンポーネント（既存の `Sprite` を拡張）。 |

| ファイル | 内容 |
| --- | --- |
| `engine/ocf/resource/SpriteSheet.h` | `SpriteFrame`、`SpriteQuad`、`computeSpriteQuad()`、`SpriteSheet` |
| `engine/ocf/resource/SpriteSheetLoader.h` | `SpriteSheetLoader`（公開 API、フォーマット判定） |
| `engine/ocf/resource/SpriteSheetCommon.h` | 読み込みの内部実装（各フォーマットの reader の宣言、共通の検証 `validateFrame()`） |
| `engine/ocf/resource/SpriteSheetJsonReader.cpp` | TexturePacker JSON の解析（yyjson） |
| `engine/ocf/resource/AssetUtility.h` | アセットパスの解決とファイル読み込み（TileMap と共通） |
| `engine/ocf/resource/JsonUtility.h` | yyjson のパースとエラー位置の整形（内部用、TileMap と共通） |
| `engine/ocf/scene/2d/Sprite.h` | `Sprite::setSpriteFrame()` |
| `assets/spritesheets/sample.json` / `sample.png` | 回転・トリミングを含むサンプル |
| `tests/sandbox/SpriteSheetTest.cpp` | 動作確認シーン |

## 使い方

```cpp
#include <ocf/resource/SpriteSheetLoader.h>
#include <ocf/scene/2d/Sprite.h>

auto result = SpriteSheetLoader::load(engine, "spritesheets/sample.json");
if (result.isErr()) {
    OCF_LOG_ERROR("{}", result.unwrapErr());
    return;
}
m_sheet = result.unwrap(); // Ref<SpriteSheet> はシーンなどで保持する

Sprite* sprite = node->addComponent<Sprite>(engine);
sprite->setSpriteFrame(*m_sheet.ptr(), "walk_01.png"); // 見つからなければ警告を出して false
sprite->setTextureFilter(TextureFilter::Nearest);      // ドット絵の場合

// アニメーション用に、接頭辞が一致するフレームを名前順で取得できる
std::vector<const SpriteFrame*> walk = m_sheet->getFramesWithPrefix("walk_");
sprite->setSpriteFrame(*walk[index], m_sheet->getTexture());
```

フレームを時間で切り替えて再生するには `SpriteAnimator` を使う（[animation.md](animation.md) を参照）。

- `Sprite` はフレームを**コピー**して保持するため、`SpriteSheet` を先に解放してもフレーム情報は残る（テクスチャの寿命は `TextureManager` に依存）。
- `setTexture()` / `setTextureRect()` を呼ぶと、フレームはトリミング・回転なしの通常の矩形に戻る。

## 描画の仕様

### コンテンツサイズとトリミング

`Sprite` のコンテンツサイズはトリミング**前**の元画像のサイズ（`sourceSize`）になる。トリミングされた部分は `offset` の位置に描かれる。

そのため、アニメーションのフレームごとにトリミング量が違っても、アンカーに対するキャラクターの位置はずれない。

### pivot

pivot は自動では適用しない（`setAnchor()` の設定を優先する）。TexturePacker で設定した pivot を使う場合は次のようにする。`Drawable2D` のアンカーと TexturePacker の pivot はどちらも「(0, 0) が左上、(1, 1) が右下」なので、そのまま渡せる。

```cpp
sprite->setAnchor(frame.pivot);
```

### 回転

TexturePacker は画像を **90° 時計回り**に回転して詰める。JSON の `frame.w` / `frame.h` は回転前（描画される向き）のサイズで、アトラス上では `h × w` の領域を占める（`SpriteFrame::getPackedRect()`）。

描画時は四隅の UV を次のように割り当てて元の向きに戻す（`computeSpriteQuad()`）。

| 描画される隅 | アトラス上の隅 |
| --- | --- |
| 左上 | 右上 |
| 左下 | 左上 |
| 右下 | 左下 |
| 右上 | 右下 |

### 反転

`setFlipX()` / `setFlipY()` は元画像全体を反転する。UV の入れ替えに加えて、トリミングされた部分の位置も反転する（`offset.x' = sourceSize.x - offset.x - w`）。

## フォーマットの追加

各フォーマットの reader は、読み込んだ内容を `SpriteFrame` の規約（`textureRect` のサイズは描画される向き、回転は時計回り格納、`offset` はトリミング部分の左上）に変換して返す。描画側（`Sprite`、`computeSpriteQuad()`）はフォーマットを意識しない。

新しいフォーマットは次の手順で追加する。

1. `SpriteSheetLoader::Format` に値を追加する
2. `SpriteSheet<Format>Reader.cpp` に reader を実装し、`SpriteSheetCommon.h` に宣言する。各フレームは `validateFrame()` で検証する
3. `SpriteSheetLoader::detectFormat()` に判定を追加する（JSON は先頭の `{`、XML は先頭の `<` とルート要素名で判定する）
4. `SpriteSheetLoader::loadFromString()` の `switch` に分岐を追加する

## 制限事項・今後の拡張

- フレームを切り替えるたびに頂点バッファを作り直す。
- 1 ファイル = 1 テクスチャ。マルチパックは複数の `SpriteSheet` として読み込む。
- Starling/Sparrow XML、cocos2d の plist 形式、ポリゴンメッシュには未対応。
