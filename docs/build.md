# ビルド

## 概要

エンジンはCMake（3.20以上）でビルドします。C++20に対応したコンパイラが必要です。

```
cmake -S . -B build
cmake --build build --config Debug
```

主な成果物は次のとおりです（Visual Studioジェネレーターの場合）。

| ターゲット | 出力先 | 内容 |
| --- | --- | --- |
| `ocfengine` | `build/engine/<Config>/ocfengine.lib` | エンジン本体 |
| `test_ocfengine` | `build/engine/test/<Config>/test_ocfengine.exe` | GoogleTestによる単体テスト |
| `sandbox` | `build/tests/sandbox/<Config>/ocf_sandbox.exe` | 動作確認用のアプリ |

## CMakeオプション

| オプション | 値 | 既定 | 内容 |
| --- | --- | --- | --- |
| `OCF_RHI_BACKEND` | `Vulkan` / `D3D12` | `Vulkan` | RHIのグラフィックスAPI。1回のビルドに含まれるバックエンドは1つだけです。 |
| `OCF_ENABLE_IMGUI` | `ON` / `OFF` | `ON` | Dear ImGuiのデバッグUI。詳しくは [imgui.md](imgui.md) を参照してください。 |
| `BUILD_SHARED_LIBS` | `ON` / `OFF` | `OFF` | エンジンを共有ライブラリとしてビルドします。 |

### `OCF_RHI_BACKEND`

RHIのバックエンドをビルド時に選びます。実行時には切り替えられません。

| 値 | 対応プラットフォーム | 必要なもの | 状態 |
| --- | --- | --- | --- |
| `Vulkan` | Windows、Linux | Vulkan SDK（Vulkan 1.3） | 使用可能 |
| `D3D12` | Windowsのみ | Windows SDK（`d3d12`、`dxgi`、`dxguid` をリンク） | 実装中。現在はスタブで、何も描画されません。 |

```
cmake -S . -B build/d3d12 -DOCF_RHI_BACKEND=D3D12
cmake --build build/d3d12 --config Debug
```

- `Vulkan`、`D3D12` 以外の値、またはWindows以外での `D3D12` は、CMakeの構成時にエラーになります。
- バックエンドごとにビルドディレクトリを分けておくと、切り替えるたびに全体を再ビルドせずに済みます。`build/` は `.gitignore` の対象なので、その下に作るのがおすすめです。
- `D3D12` のときは、Vulkan SDKがなくてもビルドできます。
- **`D3D12` では、まだImGuiに対応していません。** `OCF_ENABLE_IMGUI` は警告を出して自動的にOFFになり、ImGuiを使う `tracker` もビルドされません。

選んだバックエンドは、コンパイル定義でコードから判定できます。`ocfengine` をリンクしたターゲットにも伝わります（PUBLIC）。
使っているバックエンドが `1`、それ以外が `0` になるので、`#ifdef` ではなく `#if` で判定してください。

```cpp
#if OCF_RHI_D3D12
    // Direct3D 12
#elif OCF_RHI_VULKAN
    // Vulkan
#endif
```

| `OCF_RHI_BACKEND` | `OCF_RHI_VULKAN` | `OCF_RHI_D3D12` |
| --- | --- | --- |
| `Vulkan` | `1` | `0` |
| `D3D12` | `0` | `1` |

## 実装

| ファイル | 内容 |
| --- | --- |
| `CMakeLists.txt` | オプションの定義と値のチェック、`D3D12` でImGuiをOFFにする処理 |
| `engine/CMakeLists.txt` | バックエンドごとのソース、リンクするライブラリ、コンパイル定義 |
| `third-party/imgui/ocf/CMakeLists.txt` | `imgui_backend` は `Vulkan` のときだけビルドされる |
| `engine/ocf/rhi/DeviceFactory.cpp` | コンパイル定義に応じて、使うバックエンドの `DeviceFactory` を選ぶ |
