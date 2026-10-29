# TileMap

## 概要

タイルマップは、同じ大きさのタイルを格子状に並べて 2D のマップを描画する仕組みである。

マップは次の 2 通りの方法で用意できる。

- [Tiled Map Editor](https://www.mapeditor.org) で作成したファイルを読み込む（JSON 形式 `.tmj` / `.tsj` と XML 形式 `.tmx` / `.tsx` の両方に対応）
- コードからマップを直接組み立てる

対応するのは**直交（orthogonal）マップ**のタイルレイヤーの描画である。オブジェクトレイヤーとカスタムプロパティは描画せず、データとして保持してゲーム側から参照できるようにする。

## 主要コンポーネント

本システムは、「データ（TileMap）」「読み込み（TiledMapLoader）」「描画（TileMapRenderer）」の 3 つに責務を分離して構成する。

```
TiledMapLoader (.tmj / .tmx を解析) ─┐
                                    ├─▶ TileMap (データのみ) ─▶ TileMapRenderer (描画)
コードから直接構築 ─────────────────┘
```

| コンポーネント | 役割・責務 |
| --- | --- |
| **TileMap** | マップの大きさ、タイルセット、レイヤーを保持する**データ**。GPU には依存しないため、単体テストで扱える。`Resource` を継承し、`Ref<TileMap>` で共有する。 |
| **TiledMapLoader** | Tiled Map Editor のファイルを解析して `TileMap` を作る。タイルセットの画像は `TextureManager` で読み込む。 |
| **TileMapRenderer** | `TileMap` のタイルレイヤーを描画する 2D コンポーネント。`Drawable2D` を継承する。 |

| ファイル | 内容 |
| --- | --- |
| `engine/ocf/resource/TileMapTypes.h` | `TileGid`、反転フラグ、`TileProperties` |
| `engine/ocf/resource/TileSet.h` | `TileSet` |
| `engine/ocf/resource/TileMapLayer.h` | `TileMapLayer`、`TileLayer`、`ObjectLayer`、`MapObject` |
| `engine/ocf/resource/TileMap.h` | `TileMap` |
| `engine/ocf/resource/TiledMapLoader.h` | `TiledMapLoader`（公開 API） |
| `engine/ocf/resource/TiledCommon.h` | 読み込みの内部実装（JSON と XML で共通の処理） |
| `engine/ocf/resource/TiledJsonReader.cpp` | JSON 形式の解析（yyjson） |
| `engine/ocf/resource/TiledXmlReader.cpp` | XML 形式の解析（pugixml） |
| `engine/ocf/scene/2d/TileMapRenderer.h` | `TileMapRenderer` |

---

## 座標系

`TileMap` の座標は Tiled Map Editor と同じく、**ピクセル単位・左上が原点・y が下向き**である。オブジェクトの位置やレイヤーのオフセットもこの座標で表す。

`TileMapRenderer` はこの座標をそのまま `Drawable2D` のレイアウト座標として使う。ノードのローカル空間への配置（y 上向きへの変換、`setAnchor()`、`setPixelsPerUnit()`）は `Sprite` や `Text` と同じ仕組みで行われる。マップ全体の大きさ（`幅 × タイル幅`, `高さ × タイル高さ`）がコンテンツサイズになる。

---

## データモデル

### TileGid

タイルレイヤーの 1 マスは `TileGid`（`uint32_t`）で表す。

- `0` は空のマスを表す。
- それ以外の値は、マップ全体で通し番号になったタイルの ID である。どのタイルセットのどのタイルかは、各タイルセットの `firstGid` から決まる。
- 上位 4 ビットは Tiled Map Editor の反転フラグである。

| フラグ | 値 | 意味 |
| --- | --- | --- |
| `TILE_FLIPPED_HORIZONTALLY` | `0x80000000` | 左右反転 |
| `TILE_FLIPPED_VERTICALLY` | `0x40000000` | 上下反転 |
| `TILE_FLIPPED_DIAGONALLY` | `0x20000000` | 対角反転（左上と右下を結ぶ線で反転） |
| `TILE_ROTATED_HEXAGONAL_120` | `0x10000000` | 六角形マップ用（描画では使わない） |

`getTileGid()` でフラグを除いた ID を、`isTileFlippedX()` などで各フラグを取り出せる。`makeTileGid(gid, flipX, flipY, flipDiagonally)` は ID とフラグを組み合わせる。

反転は **対角 → 左右 → 上下** の順に適用する（Tiled Map Editor と同じ）。たとえば時計回りに 90 度回転したタイルは `makeTileGid(gid, true, false, true)` になる。

### TileSet

1 枚の画像を同じ大きさのタイルに切り分けたものである。タイルは左上から行ごとに `0` から番号が振られる。

- `margin` は画像の端から最初のタイルまでの距離、`spacing` はタイル同士の間隔である（いずれもテクセル単位）。
- `setImage()` または `setTexture()` で画像の大きさを与えると、列数（`columns`）とタイル数（`tileCount`）を自動で計算する。Tiled のファイルに値が書かれていれば、その値で上書きする。
- `getTextureRect(localId)` は、タイルのテクスチャ上の矩形を返す。
- テクスチャは所有しない。テクスチャの管理は `TextureManager` が行う。

### レイヤー

レイヤーは `TileMap` が生成・所有する。`TileMap::getLayers()` の順に、後ろのものほど手前に描画される。

| クラス | 内容 |
| --- | --- |
| `TileMapLayer` | 全レイヤー共通の基底。名前、ID、表示・非表示、不透明度、オフセット、プロパティを持つ。 |
| `TileLayer` | マスごとの `TileGid`。大きさはマップと同じで、左上から行ごとに格納する。`getTile()`、`setTile()`、`fill()`、`setTiles()` で読み書きする。 |
| `ObjectLayer` | `MapObject` の一覧。描画はしない。 |

`MapObject` は、出現位置や当たり判定の範囲などに使う図形・点である。形状（矩形、楕円、点、多角形、折れ線、テキスト）、位置、大きさ、回転、クラス名、プロパティを持つ。Tiled Map Editor と同じく、タイルオブジェクトの位置は左下、それ以外のオブジェクトの位置は左上を表す。

### カスタムプロパティ

`TileProperties` は名前から `Variant` への対応表で、マップ・レイヤー・タイルセット・タイル・オブジェクトがそれぞれ持つ。Tiled の型は次のように格納する。

| Tiled の型 | 格納する型 |
| --- | --- |
| `bool` | `bool` |
| `int`、`object` | `int` |
| `float` | `float` |
| `string`、`color` | `std::string`（`color` は `"#AARRGGBB"` のまま） |
| `file` | `std::string`（アセットパスに解決したもの） |
| `class` | 格納しない |

### 変更の検知

`TileMap::getRevision()` は、描画に影響する変更（タイルの書き換え、レイヤーの追加、表示・不透明度・オフセットの変更、タイルセットの追加）のたびに増える。`TileMapRenderer` はこの値を毎フレーム確認し、変わっていればジオメトリを作り直す。そのため、マップを描画中でも `setTile()` などで直接書き換えてよい。

---

## Tiled ファイルの読み込み

### API

```cpp
#include <ocf/resource/TiledMapLoader.h>

auto result = TiledMapLoader::load(engine, "tilemaps/sample.tmj");
if (result.isErr()) {
    OCF_LOG_ERROR("{}", result.unwrapErr());
    return;
}
Ref<TileMap> map = result.unwrap();
```

| 関数 | 内容 |
| --- | --- |
| `load(engine, assetPath)` | ファイルを読み込み、タイルセットのテクスチャも読み込む。拡張子が `.tmx` なら XML、それ以外は JSON として扱う。 |
| `loadFromString(text, format, baseDirectory, reader)` | 文字列から読み込む。テクスチャは読み込まない（GPU 不要）。外部タイルセットは `reader` で読む（省略するとアセットディレクトリから読む）。テストで使う。 |
| `resolveTextures(map, textureManager)` | テクスチャ未設定のタイルセットの画像を読み込む。 |

結果は `Result<Ref<TileMap>, std::string>` で返す。エラーメッセージは問題の箇所から始まる。

- JSON: JSON Pointer 形式（例: `/layers/0/data: expected 2 tiles, got 3`）
- XML: 要素のパス（例: `/map/layer[0]/data: infinite maps are not supported`）

### パスの解決

ファイル内のパス（タイルセットの画像、外部タイルセット、`file` 型のプロパティ）は、**そのパスが書かれたファイルのディレクトリ**を基準に解決し、アセットパスとして保持する。外部タイルセットに書かれた画像のパスは、マップではなくタイルセットファイルの位置が基準になる。

外部タイルセットの形式は拡張子で判断する（`.tsx` なら XML、それ以外は JSON）。そのため、JSON のマップから XML のタイルセットを参照するような、形式をまたいだ組み合わせも読める。

### 対応範囲

| 項目 | 対応状況 |
| --- | --- |
| マップの向き | 直交（orthogonal）のみ。他はエラー。 |
| 無限マップ（infinite / chunk） | 非対応。エラー。 |
| タイルデータ | CSV、JSON の配列、base64（圧縮なし / zlib / gzip）、XML の `<tile>` 要素。zstd はエラー。 |
| タイルセット | 埋め込み・外部（`.tsj` / `.tsx`）。1 枚の画像から切り出すものに限る。タイルごとに別画像のものはエラー。 |
| タイルレイヤー | 対応。 |
| オブジェクトレイヤー | データとして保持（描画しない）。オブジェクトテンプレート（`.tx`）はエラー。 |
| グループレイヤー | 子レイヤーに展開する。オフセットは加算、不透明度は乗算、表示は論理積で子に反映する。グループ自体のプロパティは保持しない。 |
| 画像レイヤー | 警告を出して読み飛ばす。 |
| 描画順（renderorder） | 無視する。常に右下方向（right-down）に描画する。 |

タイルセットの画像の読み込み（zlib の展開を含む）には、既存の stb_image を使う。XML の解析には third-party に追加した pugixml を使う。

---

## コードからの構築

```cpp
#include <ocf/resource/TileMap.h>
#include <ocf/scene/2d/TileMapRenderer.h>

Texture* texture = engine.getTextureManager().addImage("tilemaps/terrain.png");

// 8 × 3 マス、1 マス 16 × 16 ピクセル
Ref<TileMap> map(new TileMap(8, 3, 16, 16));
const TileGid first = map->addTileSet(Ref<TileSet>(new TileSet(texture, 16, 16)));

TileLayer* ground = map->addTileLayer("ground");
ground->fill(first + 0);                                    // 全マスを 0 番のタイルで埋める
ground->setTile(3, 1, makeTileGid(first + 5, true, false)); // 5 番のタイルを左右反転して置く

node->addComponent<TileMapRenderer>(engine, map);
```

- `addTileSet()` は、既存のタイルセットの直後を `firstGid` として割り当てて返す。タイル数で次の開始位置が決まるため、追加する前にタイルセットの大きさを確定させておく。
- 開始位置を指定したい場合は `addTileSet(tileSet, firstGid)` を使う。既存の範囲と重なる場合は `false` を返す。
- `addTileLayer()` で作るレイヤーは、マップと同じ大きさになる。

---

## 描画

### TileMapRenderer

```cpp
TileMapRenderer* renderer = node->addComponent<TileMapRenderer>(engine, map);
renderer->setPixelsPerUnit(1.0f);              // 1 ピクセル = 1 ワールド単位
renderer->setAnchor(math::vec2(0.0f, 0.0f));   // マップの左上をノードの原点に置く
renderer->setTextureFilter(TextureFilter::Nearest);
```

| 関数 | 内容 |
| --- | --- |
| `setTileMap()` | 描画するマップを設定する。マップは共有され、所有はしない（`Ref` で参照する）。 |
| `setTextureFilter()` | テクスチャのフィルタ。既定は `Nearest`（ピクセルアートがにじまないようにするため）。 |
| `setColor()`、`setAnchor()`、`setPixelsPerUnit()` | `Drawable2D` から継承。 |

### 描画の流れ

`rebuild()` で、表示中のタイルレイヤーを順に処理してジオメトリを作る。

1. 各マスのタイルを `TileMap::findTileSet()` でタイルセットごとに振り分ける。
2. **（レイヤー, タイルセット）の組ごとに 1 つのジオメトリ**を作り、それぞれ 1 回の描画コマンドにする。
3. レイヤーの順に描画コマンドを追加する。

レイヤーの重なり順は描画コマンドを追加した順で決まるため、ビューは **`View::SortMode::SubmitOrder`** で使う。`Distance` では半透明物体としてカメラからの距離順に並べ替えられるため、他の物体と混ざったときにレイヤーの順序が保証されない。

シェーダーは `Sprite` と同じ `shaders/sprite.frag.spv` を使う。

### タイルの配置

タイルは**セルの左下に揃えて**配置する（Tiled Map Editor と同じ）。セルより背の高いタイル（木など）は上にはみ出して描画される。

```
位置 = (x × タイル幅, (y + 1) × タイル高さ − タイルセットのタイル高さ)
       + タイルセットの tileOffset + レイヤーのオフセット
```

反転フラグは、四隅のテクスチャ座標を入れ替えて表現する。対角反転は矩形の左右・上下の入れ替えでは表せないため、`Drawable2D::appendQuad()` に四隅ごとのテクスチャ座標を受け取るオーバーロードを追加している。

### レイヤーの不透明度

レイヤーの不透明度は、描画コマンドごとのマテリアルインスタンスに設定する。`Drawable2D::createMaterialInstance(opacity)` で作ったインスタンスには `setColor()` の色のアルファに不透明度を掛けた値が設定され、`setMaterialOpacity()` で後から変更できる。

マテリアルインスタンスは個別に解放できないため、描画コマンドの番号ごとにキャッシュし、作り直しのたびに再利用する。テクスチャやフィルタが変わった場合だけディスクリプタを更新する。

### 作り直しの条件

`update()` で次のいずれかを検知すると、次のフレームでジオメトリを作り直す。

- `TileMap::getRevision()` が前回の作り直しから変わった
- タイルセットのテクスチャが変わった（`resolveTextures()` を後から呼んだ場合など）
- `setTileMap()`、`setTextureFilter()`、`setAnchor()`、`setPixelsPerUnit()` が呼ばれた

現在は 1 マスの変更でもマップ全体を作り直す。

---

## 制限事項と今後の拡張

- **チャンク分割**: 大きなマップで 1 マスだけ書き換える場合も全体を作り直す。大規模なマップではチャンク単位の更新が必要になる。
- **タイルアニメーション**: Tiled のタイルアニメーション（フレームの切り替え）は読み込まない。
- **オブジェクトの描画**: タイルオブジェクトや画像レイヤーは描画しない。
- **その他の向き**: アイソメトリック・六角形マップには対応しない。
- **テクスチャのにじみ**: `Linear` フィルタでは、隣のタイルの色が端ににじむことがある。タイルセット側で余白（`spacing`）を取るか、`Nearest` を使う。

---

## テストとサンプル

| 場所 | 内容 |
| --- | --- |
| `engine/test/test_tilemap.cpp` | データモデルのテスト（反転フラグ、タイルの矩形、`firstGid` の割り当て、変更の検知など）。 |
| `engine/test/test_tiled_map_loader.cpp` | 読み込みのテスト。同じ内容の JSON と XML が同じ結果になることや、各エンコード、エラーメッセージを確認する。`assets/tilemaps` のサンプルが JSON と XML で一致することも確認する。 |
| `assets/tilemaps/` | サンプルのマップ（`sample.tmj`、`sample.tmx`）、外部タイルセット（`props.tsj`、`props.tsx`）、画像（`terrain.png`、`props.png`）。 |
| `tests/sandbox/TileMapTest.cpp` | サンドボックスの「TileMap Test」。JSON 版と XML 版のサンプル、コードで組んだマップ（「F」のタイルを 8 通りの向きで並べたもの）を表示する。ImGui で倍率、フィルタ、レイヤーの表示・不透明度を変更でき、オブジェクトの一覧も確認できる。 |
