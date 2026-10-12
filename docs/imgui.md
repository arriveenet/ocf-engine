# Dear ImGui（デバッグUI）

## 概要

エンジンには、デバッグ用のUIとして [Dear ImGui](https://github.com/ocornut/imgui) を組み込んでいます。
CMakeオプション `OCF_ENABLE_IMGUI`（既定はON）で有効・無効を切り替えます。

```
cmake -S . -B build -DOCF_ENABLE_IMGUI=OFF
```

ゲーム側のコードは、ON・OFFのどちらでも同じままビルドできます。`#if OCF_ENABLE_IMGUI` で囲む必要はありません。
その代わりに、ImGuiを使ってよい場所が決まっています（[ルール](#ルール-imguiを使ってよい場所)を参照）。

## 使い方

### デバッグUIを作る: `Scene::onImGui()`

シーンで `onImGui()` をオーバーライドし、その中でImGuiのAPIを直接呼びます。

```cpp
#include <imgui.h>

class MainScene : public Scene {
public:
    void onImGui() override
    {
        ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("FPS: %.1f", getEngine().getFrameCounter().getFrameRate());
        ImGui::SliderFloat("回転速度 (deg/s)", &m_rotationSpeed, 0.0f, 180.0f);
        ImGui::End();
    }

private:
    float m_rotationSpeed = 16.0f;
};
```

- 毎フレーム、全シーンの更新が終わったあとに呼ばれます。
- 有効なシーンすべてについて、下のレイヤーから順に呼ばれます。
- 上のシーンが更新を止めている間（ポーズ中など）も呼ばれます。止まっているシーンの値をデバッグUIで確認・変更できます。

### フォントやスタイルを設定する: `Engine::configureImGui()`

渡した関数は、その場で1回だけ呼ばれます。エンジンの作成後、たとえば `Application::run()` のセットアップ用コールバックの中で呼びます。

ImGuiの標準フォントには日本語の文字が無いので、日本語を表示するには日本語フォントを読み込みます。

```cpp
engine.configureImGui([](ImGuiIO& io) {
    const std::string fontPath =
        FileSystem::getInstance()->getAssetFullPath("fonts/NotoSansJP-Regular.ttf");
    if (!fontPath.empty()) {
        io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 18.0f);
    }
});
```

ここで行うのは、フォントやスタイルの設定だけです。`ImGui::Begin()` などのウィンドウやウィジェットを作るAPIは、フレームの外なので呼べません。

## ルール: ImGuiを使ってよい場所

ImGuiのAPIを使ってよいのは、次の2か所だけです。

- `Scene::onImGui()` の中
- `Engine::configureImGui()` に渡した関数の中

`onUpdate()`、`onEnter()`、`onEvent()` など、それ以外の場所から呼んではいけません。

### 理由

エンジンはImGuiのフレームを、全シーンの更新が終わったあと、`onImGui()` を呼ぶ直前に開始します（`ImGui::NewFrame()`）。
フレームは描画のときに終了します（`ImGui::Render()`）。このため、それ以外の場所はImGuiのフレームの外になります。

OFFのときはImGui自体が初期化されず、上の2か所に書いたコードは一度も実行されません。
ルールを守っていれば、OFFのときにImGuiのコードが動くことはありません。

### ルール外で使った場合

| 構成 | ルール内での使用 | ルール外での使用（例: `onUpdate()`） |
| --- | --- | --- |
| ON・Debug | 動作する | アサートで停止する（`Forgot to call ImGui::NewFrame()` など） |
| ON・Release | 動作する | アサートが無効なので検出されず、動作は保証されない |
| OFF | 呼ばれない | ImGuiが初期化されていないので停止する（Debugではアサート `No current context`、Releaseではクラッシュ） |

ルール外の使用は、ONのDebugビルドでそのコードが実行されれば、アサートで検出されます。
開発中はONのDebugビルドで動作確認することで、OFFのビルドでクラッシュするコードが残らないようにしてください。

## 1フレームの流れ

1. `Window::pollEvents()`: GLFWの入力をImGuiも受け取ります。
2. `Engine::update()`
    1. シーンの更新（`Scene::onUpdate()` など）
    2. ImGuiのフレーム開始
    3. 有効な各シーンの `Scene::onImGui()`
3. `Engine::draw()`
    1. 各Viewの描画
    2. ImGuiの描画。Viewの上に重ねて描き、深度バッファは使いません。
    3. 画面への表示

OFFのときは、2-2、2-3、3-2を行いません。

## 入力

ImGuiは、GLFWから直接入力を受け取ります。
ImGuiがマウスやキーボードを使っている間は、その入力イベントはシーンに届きません。`Scene::onEvent()` も呼ばれません。

| 入力 | ImGuiが使っている状態 | 判定に使う値 |
| --- | --- | --- |
| マウス | ImGuiのウィンドウの上にマウスがある、ImGuiの上で押したボタンを押し続けている、ポップアップが開いている | `ImGuiIO::WantCaptureMouse` |
| キーボード | ウィジェットを操作している（テキスト入力欄の編集中、スライダーのドラッグ中など）、モーダルウィンドウが開いている | `ImGuiIO::WantCaptureKeyboard` |

ImGuiのウィンドウが表示されている、またはフォーカスされているだけでは、キーボード入力はシーンに届きます。
ImGuiのウィンドウの外でマウスのボタンを押し始めた場合は、そのままImGuiのウィンドウの上に移動しても、マウス入力はシーンに届きます。

## `OCF_ENABLE_IMGUI=OFF` のときの動作

- ImGuiのコンテキスト、GLFW・Vulkanとの接続、GPUリソースは作りません。入力も横取りしません。
- `Scene::onImGui()` と `Engine::configureImGui()` に渡した関数は呼ばれません。
- ImGui本体（`imgui` ターゲット）はビルド・リンクされます。ゲーム側のコードを変えずにビルドできるようにするためです。
  そのため、使われないImGuiのコードも実行ファイルに含まれます。sandboxのReleaseビルドでは、ImGuiを完全に取り除いた場合より約0.8MB大きくなります（デモウィンドウを含む）。
- GLFW・Vulkanとの接続部分（`imgui_backend` ターゲット）はリンクされません。

コンソールやモバイル向けの出荷などで、ImGuiのコードを実行ファイルから完全に取り除く必要が出た場合は、この方式を見直してください。

## 補足

- ウィンドウの位置や大きさは、実行時のカレントディレクトリの `imgui.ini` に保存されます。`.gitignore` の対象です。
- 現在、エンジンはウィンドウのサイズ変更（スワップチェーンの作り直し）に対応していないため、ImGui側もサイズ変更には対応していません。

## 実装

| ファイル | 内容 |
| --- | --- |
| `engine/src/renderer/ImGuiLayer.h` / `.cpp` | ImGuiのコンテキスト、GLFW・Vulkanとの接続、フレームの開始と描画、入力の横取り |
| `engine/src/renderer/Renderer.cpp` | `Renderer::renderImGui()`: Viewの上にImGuiを描く描画パス |
| `engine/src/core/Engine.cpp` | ImGuiLayerの作成・破棄、1フレームの流れ、`Engine::configureImGui()` |
| `engine/src/scene/SceneManager.cpp` | `SceneManager::updateImGui()`: 有効な各シーンの `onImGui()` を呼ぶ |
| `third-party/imgui/ocf/CMakeLists.txt` | `imgui`（本体）と `imgui_backend`（GLFW・Vulkanとの接続）のビルド設定 |
