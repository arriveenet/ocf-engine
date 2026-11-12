# Scene

## 概要

シーンシステムは、ゲームの画面（タイトル画面、ステージ、HUD、ポーズメニューなど）を単位として、**寿命・読み込み・更新・入力・描画**をまとめて管理する仕組みである。

`SceneManager` は複数のシーンを**レイヤー**順に重ねて同時に動かす。たとえば「ステージ（レイヤー 0）」の上に「ポーズメニュー（レイヤー 10）」と「デバッグ HUD（レイヤー 1000）」を重ねる、といった構成をとれる。

各シーンは描画対象のツリー（`SceneGraph`）と、それをどう描くか（`View`）を所有する。ツリーは `Node` と、`Node` に付ける `Component` で構成する。

## 主要コンポーネント

```
SceneManager
 └─ Scene（レイヤー順に複数）
     ├─ SceneGraph（複数可）
     │   └─ Node（ルート）
     │       ├─ Component（Camera / Sprite / Text / TileMapRenderer / MeshInstance ...）
     │       │   └─ Renderable（描画単位、Renderer が読む）
     │       └─ Node（子）...
     └─ View（複数可）── SceneGraph と Camera を参照し、描画先と描画順を決める
```

| コンポーネント          | 役割・責務                                                                                                                                                    |
| ---------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **SceneManager** | シーンの追加・切り替え・削除・非同期読み込みを管理し、毎フレーム更新・入力・描画対象の収集をレイヤー順に行う。`Engine::getSceneManager()` で取得する。                                                                |
| **Scene**        | 画面の単位。ライフサイクルのコールバック（`onLoad()` / `onEnter()` / `onUpdate()` / `onEvent()` / `onImGui()` / `onExit()`）を持ち、`SceneGraph` と `View` を所有する。ユーザーはこれを継承して画面を作る。 |
| **SceneGraph**   | `Node` のツリー。ルートノードを所有し、毎フレームの更新とワールド行列の計算を行う。                                                                                                            |
| **Node**         | ツリーの節。トランスフォーム（位置・回転・拡大）、子ノード、コンポーネントを所有する。                                                                                                              |
| **Component**    | `Node` に付ける機能の単位。毎フレーム `update()` が呼ばれ、描画するものがあれば `Renderable` を持つ。                                                                                      |
| **View**         | 1 回の描画パス。どの `SceneGraph` をどの `Camera` で、どこ（スワップチェーン / `RenderTarget`）へ描くかを指定する。参照するだけで所有はしない。                                                            |
| **Camera**       | 投影行列とビュー行列を持つコンポーネント。                                                                                                                                    |
| **Renderable**   | 頂点・インデックスバッファ、マテリアル、パイプライン、アルファモードの組。`Renderer` が描画コマンドに変換する。                                                                                            |

| ファイル | 内容 |
| --- | --- |
| `engine/ocf/scene/SceneManager.h` | `SceneManager`、`SceneLoadOperation` |
| `engine/ocf/scene/Scene.h` | `Scene` |
| `engine/ocf/scene/SceneGraph.h` | `SceneGraph` |
| `engine/ocf/scene/Node.h` | `Node` |
| `engine/ocf/scene/Component.h` | `Component` |
| `engine/ocf/scene/View.h` | `View` |
| `engine/ocf/scene/Camera.h` | `Camera` |
| `engine/ocf/scene/Renderable.h` | `Renderable` |
| `engine/ocf/scene/2d/` | 2D 描画コンポーネント（`Drawable2D`、`Sprite`、`Text`、`TileMapRenderer`） |
| `engine/ocf/scene/3d/` | 3D 描画コンポーネント（`MeshInstance`） |
| `engine/ocf/scene/animation/` | Tween（`Tweener`）。フレームアニメーションの `SpriteAnimator` は `scene/2d/`（[animation.md](animation.md) を参照） |

---

## 1 フレームの流れ

`Engine::mainLoop()` は毎フレーム次の順に処理する。

1. **`SceneManager::update(deltaTime)`**
   1. キューに溜まった変更（追加・切り替え・削除）を適用する。
   2. 非同期読み込みが終わったシーンを有効化する。
   3. 有効化で新たに要求された変更を適用する。
   4. 有効なシーンを**下のレイヤーから順に**更新する。各シーンで `onUpdate()` を呼び、続けてそのシーンの各 `SceneGraph::update()` を呼ぶ。
2. **`SceneManager::updateImGui()`**（`OCF_ENABLE_IMGUI` 時のみ）: 有効なシーンの `onImGui()` を下のレイヤーから呼ぶ。
3. **`SceneManager::collectViews()`**: 有効なシーンの `View` を **レイヤー → `View::getOrder()`** の順に並べる。
4. **`Renderer::render(view)`**: 並べた順に各 `View` を描画し、最後に ImGui を重ねる。

入力イベント（キーボード・マウス）は `EventDispatcher` から `SceneManager::dispatchEvent()` に渡され、**上のレイヤーから順に** `onEvent()` が呼ばれる。

---

## Scene

### ライフサイクル

```
Created ──▶ Loading ──▶ Loaded ──▶ Active ──▶ Exited
          (onLoad)            (onEnter)    (onExit)
```

| 状態 | 意味 |
| --- | --- |
| `Created` | `SceneManager` に渡された直後。`onLoad()` はまだ呼ばれていない。 |
| `Loading` | `onLoad()` の実行中。 |
| `Loaded` | `onLoad()` が終わり、有効化を待っている。 |
| `Active` | `onEnter()` が終わり、更新・描画・入力の対象になっている。 |
| `Exited` | `onExit()` が終わり、破棄される直前。 |

| コールバック | 呼ばれるタイミング | スレッド |
| --- | --- | --- |
| `onLoad()` | 有効化の前に 1 回 | `loadAsync()` のときはワーカースレッド、それ以外はメインスレッド |
| `onEnter()` | 有効になるとき 1 回 | メインスレッド |
| `onUpdate(dt)` | 毎フレーム、`SceneGraph` の更新の前 | メインスレッド |
| `onEvent(event)` | 入力イベントごと | メインスレッド |
| `onImGui()` | 毎フレーム、更新の後 | メインスレッド |
| `onExit()` | 破棄の前に 1 回（`Active` になったシーンのみ） | メインスレッド |

### どこで何をするか

- **コンストラクタ**: `getEngine()` と `getSceneManager()` はまだ使えない（`SceneManager` に渡された時点で設定される）。`createSceneGraph()` / `createView()` や `Node` の構築など、エンジンに依存しない準備だけを行う。
- **`onLoad()`**: ファイルの読み込みや解析など **CPU の処理だけ**を行う。`loadAsync()` で読み込むとワーカースレッドで実行されるため、GPU リソースの作成や `SceneManager` の操作をしてはならない。
- **`onEnter()`**: GPU リソースを作る処理はここで行う。`Sprite`、`Text`、`MeshInstance` などの描画コンポーネントはコンストラクタでシェーダやパイプラインを作るため、`onEnter()` 以降で追加する。ウィンドウサイズもここで読む（後述の `WindowResizeEvent` は有効化前のシーンには届かない）。
- **`onExit()`**: シーン外のものへの後始末（購読の解除など）を行う。シーンが所有する `SceneGraph` と `View` はデストラクタで自動的に破棄される。

### 所有と破棄の順序

`Scene` は `SceneGraph` と `View` を `std::unique_ptr` で所有する。`View` は `SceneGraph` を参照するため、**`View` が先に破棄される**。

### シーンの制御フラグ

| メソッド | 効果 | 用途の例 |
| --- | --- | --- |
| `setBlocksUpdate(true)` | このシーンが有効な間、**下のレイヤー**の `onUpdate()` と `SceneGraph` の更新を止める。描画と `onImGui()` は止めない。 | ポーズメニュー |
| `setBlocksInput(true)` | 入力イベントを下のレイヤーに渡さない。 | モーダルなメニュー |
| `setPersistent(true)` | `SceneManager::change()` で切り替えても削除されない。 | フェード演出、常駐 HUD |
| `setLoadProgress(p)` | `onLoad()` の進捗（0〜1）を報告する。読み込み中のスレッドから呼べる。 | ローディング画面 |

### 入力イベント

- `onEvent()` は上のレイヤーから順に呼ばれる。同じレイヤーでは後から追加したシーンが先である。
- `event.setHandled(true)` を呼ぶと、そのイベントは下のレイヤーに届かない。
- `WindowResizeEvent` だけは例外で、handled フラグや `setBlocksInput()` に関係なく**すべての有効なシーン**に届く。

### 子シーン

`Scene::addChild<T>(layer, args...)` は、寿命が親に含まれるシーンを追加する。

- 子は親より後に作られ、親より先に破棄される。そのため返されたポインタは親が生きている間有効である（ただし親のデストラクタ内では無効）。
- 親を削除すると子もまとめて削除される。キューに入ったまま未追加の子は追加されない。
- `setPersistent()` が効くのはトップレベルのシーンだけである。子シーンは親に従う。

サンドボックスでは、各テストシーンが `initializeScene()` でデバッグ HUD を子シーンとして追加している（`tests/sandbox/TestBaseScene.cpp`）。

```cpp
void TestBaseScene::initializeScene()
{
    addChild<DebugHudScene>(DEBUG_HUD_LAYER); // DEBUG_HUD_LAYER = 1000
}
```

### ImGui

`onImGui()` は Dear ImGui の API を使ってよい唯一の場所である。`OCF_ENABLE_IMGUI` を無効にしてビルドすると呼ばれないため、`#if` で囲む必要はない。`setBlocksUpdate()` で更新が止まっているシーンでも呼ばれるので、ポーズ中もデバッグ UI は表示され続ける。

---

## SceneManager

### シーンの追加・切り替え・削除

| メソッド | 動作 |
| --- | --- |
| `change<T>(args...)` | 永続でないトップレベルのシーンをすべて削除し、`T` をレイヤー 0 に追加する。 |
| `add<T>(layer, args...)` | 現在のシーンの上に `T` を追加する。 |
| `remove(scene)` / `Scene::close()` | シーンとその子を削除する。 |
| `loadAsync<T>(mode, layer, args...)` | `T` の `onLoad()` をワーカースレッドで実行し、終わったらメインスレッドで有効化する。 |

```cpp
// アプリケーション起動時
engine.getSceneManager().change<MainScene>();

// シーンの中から
getSceneManager().change<SpriteTest>();          // 画面を切り替える
auto* pause = getSceneManager().add<PauseMenu>(10); // ポーズメニューを重ねる
pause->close();                                   // 閉じる
```

**変更はすべてキューに入り、次の `update()` の最初に適用される。** そのため、どのコールバックの中からでも安全に呼べる。返されるポインタはすぐに使えるが、その時点ではまだ有効化されていない（`onLoad()` / `onEnter()` も呼ばれていない）。適用中のコールバックから要求された変更も、同じ `update()` の中で続けて適用される。

`change()` / `add()` で追加したシーンの `onLoad()` は、適用時にメインスレッドで同期的に実行される。重い読み込みには `loadAsync()` を使う。

### レイヤーの順序

- シーンはレイヤーの小さい順に並ぶ。同じレイヤーでは後から追加したものが上になる。
- **更新と描画は下から上**、**入力は上から下**へ進む。
- `View` の描画順は、まずシーンのレイヤー、同じレイヤー内では `View::setOrder()` の値で決まる。

### 非同期読み込み

```cpp
SceneLoadOperation op =
    getSceneManager().loadAsync<StageScene>(SceneManager::LoadMode::Single, 0);
op.setAllowActivation(false); // ローディング画面を最低限表示したい場合

// ローディング画面の onUpdate() で
m_progressText->setString(std::format("{:.0f}%", op.progress() * 100.0f));
if (op.isLoaded() && m_elapsed > 1.0f) {
    op.activate(); // 次の update() の最初に有効化される
}
```

| `LoadMode` | 動作 |
| --- | --- |
| `Single` | 有効化のときに永続でないシーンを削除してから追加する（`change()` 相当）。 |
| `Additive` | 現在のシーンの上に追加する（`add()` 相当）。 |

- 既定では読み込みが終わるとすぐに有効化される。`setAllowActivation(false)` で待たせ、`activate()` で有効化する。
- `progress()` は 1 フレームに 1 回更新される。値は `onLoad()` 内で `setLoadProgress()` を呼んで報告する。
- `JobSystem` が初期化されていない場合は、呼び出したスレッドでその場で読み込む。
- `SceneManager` の破棄時は、読み込み中のシーンの完了を待ってから破棄する。

---

## SceneGraph と Node

### SceneGraph の更新

`SceneGraph::update(dt)` は次の順に処理する。

1. `setUpdateCallback()` で設定したコールバックを呼ぶ。
2. ルートから深さ優先で `Node::update()` を呼ぶ。各ノードでは**自分のコンポーネントを追加順に更新してから**、子ノードを更新する。
3. ルートからワールド行列を計算する。変更のあったノードとその子孫だけが再計算される。

### Node

```cpp
SceneGraph* world = createSceneGraph();

Node* player = world->getRoot()->createChild();
player->setPosition(math::vec3(0.0f, 1.0f, 0.0f));
player->setRotation(math::vec3(0.0f, 90.0f, 0.0f)); // 度数法、X → Y → Z の順に適用
player->setScale(math::vec3(2.0f));

Sprite* sprite = player->addComponent<Sprite>(getEngine(), texture);
```

- 子ノードは `createChild()` で作り、親が `std::unique_ptr` で所有する。`destroyChild()` で破棄する。
- `addComponent<T>(args...)` でコンポーネントを作って所有する。`removeComponent()` で破棄する。
- `getWorldMatrix()` は**直前の `SceneGraph` 更新時点**の値である。同じフレーム内で位置を変えても、次の更新まで反映されない。
- `setVisible(false)` にしたノードとその子孫は描画されない。ただし更新は続く。

### Component

- `update(dt)` をオーバーライドしてフレームごとの処理を書く。
- `getOwner()` で付けられたノードを取得できる。コンストラクタ内ではまだ設定されていない。
- 描画するコンポーネントは `m_renderables` に `Renderable` を追加する。`Renderer` は可視ノードのコンポーネントから `Renderable` を集め、ノードのワールド行列で描画する。

既存の描画コンポーネント:

| コンポーネント | 内容 |
| --- | --- |
| `Sprite` | テクスチャの一部を矩形で描く。 |
| `Text` | フォントで文字列を描く。 |
| `TileMapRenderer` | タイルマップを描く（[tilemap.md](tilemap.md) を参照）。 |
| `MeshInstance` | 3D メッシュを描く。 |

アニメーションのコンポーネント（`SpriteAnimator`、`Tweener`）は [animation.md](animation.md) を参照。

2D コンポーネントは共通の基底 `Drawable2D` を継承し、ピクセル単位のレイアウト座標（左上原点・y 下向き）を `setAnchor()` と `setPixelsPerUnit()`（既定 100）でノードのローカル空間（y 上向き）に配置する。UI のようにピクセル座標でそのまま置きたい場合は `setPixelsPerUnit(1.0f)` にする。

### 注意点

- `Node::update()` はコンポーネントと子ノードの配列を走査しながら更新するため、**更新中に同じノードの `removeComponent()` / `destroyChild()` を呼んではならない**。破棄はシーンの `onUpdate()`（`SceneGraph` の更新前）で行う。
- `Node::setParent()` は親ポインタを書き換えるだけで、所有権は移らない。付け替えには使えない。

---

## View と Camera

### View

```cpp
View* view = createView();
view->setScene(m_world);              // SceneGraph を 1 つに置き換える
view->addSceneGraph(m_effects);       // 複数の SceneGraph を同じパスで描く
view->setCamera(m_camera);
view->setOrder(0);                    // 同じレイヤー内で小さいほど先に描く
view->setClearFlags(View::ClearFlags::All);
view->setClearColor(math::vec4(0.1f, 0.1f, 0.15f, 1.0f));
view->setSortMode(View::SortMode::Distance);
```

| 設定 | 内容 |
| --- | --- |
| `setClearFlags()` | 描画前にクリアするアタッチメント。`Depth` だけにすると前の View の上に重ねて描ける（UI 用）。フレームで最初にスワップチェーンへ描く View は、設定に関係なく両方クリアされる。 |
| `setSortMode(Distance)` | 不透明 → マスク → 半透明（奥から手前）の順に並べる。3D 向け（既定）。 |
| `setSortMode(SubmitOrder)` | ツリーを辿った順に描く。後のノードほど手前になる。UI・2D 向け。 |
| `setRenderTarget()` | テクスチャに描く。詳細は [renderer.md](renderer.md) の「レンダーターゲット」を参照。 |

`Camera` を設定していない View は描画されない。

### Camera

`Camera` は `Component` で、ノードに付けて使う。

```cpp
// 3D
m_camera = m_world->getRoot()->createChild()->addComponent<Camera>();
m_camera->perspective(math::radians(60.0f), aspect, 0.1f, 100.0f); // 視野角はラジアン
m_camera->lookAt(math::vec3(0.0f, 2.0f, 5.0f), math::vec3(0.0f));

// 2D UI（左下原点・ピクセル単位）
Camera* uiCamera = m_ui->getRoot()->createChild()->addComponent<Camera>();
uiCamera->orthographic(0.0f, width, 0.0f, height);
```

**ビュー行列とカメラ位置は `lookAt()` で設定した値だけで決まり、付けたノードのトランスフォームは反映されない。** カメラを動かすときは `lookAt()` を呼び直す。

---

## 実装例

UI だけのシーンを既存のシーンに重ねる例（`tests/sandbox/TestBaseScene.cpp` を簡略化したもの）。

```cpp
class HudScene : public ocf::Scene {
public:
    void onEnter() override
    {
        using namespace ocf;
        const math::ivec2 size = getEngine().getWindowSize();

        m_ui = createSceneGraph();
        Camera* camera = m_ui->getRoot()->createChild()->addComponent<Camera>();
        camera->orthographic(0.0f, float(size.x), 0.0f, float(size.y));

        View* view = createView();
        view->setScene(m_ui);
        view->setCamera(camera);
        view->setClearFlags(View::ClearFlags::Depth); // 下のシーンの上に重ねる
        view->setSortMode(View::SortMode::SubmitOrder);

        FontTrueTypeConfig config;
        config.fontPath = "fonts/NotoSansJP-Regular.ttf";
        config.fontSize = 16;
        Ref<Font> font = getEngine().getFontManager().loadFontTTF(config);

        Node* node = m_ui->getRoot()->createChild();
        node->setPosition(math::vec3(16.0f, float(size.y), 0.0f));
        m_text = node->addComponent<Text>(getEngine(), font);
        m_text->setPixelsPerUnit(1.0f);
        m_text->setAnchor(math::vec2(0.0f, 0.0f));
    }

    void onUpdate(float) override
    {
        m_text->setString(std::format("FPS: {:.1f}",
                                      getEngine().getFrameCounter().getFrameRate()));
    }

private:
    ocf::SceneGraph* m_ui = nullptr;
    ocf::Text* m_text = nullptr;
};

// 使う側
getSceneManager().add<HudScene>(1000);
```

その他の使用例は `tests/sandbox/` の各テストシーンを、`SceneManager` の挙動の詳細は `engine/test/test_scene_manager.cpp` を参照。
