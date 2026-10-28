# Rendering Hardware Interface (RHI)

## 概要

RHI（Rendering Hardware Interface）は、グラフィックスAPIとグラフィックスハードウェアの間の抽象化レイヤーです。
RHIは、異なるプラットフォームやグラフィックスAPI(Vulkan、OpenGL、DirectX12など)に対して一貫したインターフェースを提供し、
上位レイヤーがハードウェアやAPIに依存せずにグラフィックス機能を利用できるようにします。

## 基本方針

- GLFWなど外部ライブラリに依存せず、ウィンドウ管理や入力処理はアプリケーション側で行う。
- RTTIを使用せず、静的な型システムを活用してパフォーマンスを最適化する。
- 例外を使用せず、エラー処理は`ocf::Result<T, E>`などの型を使用して行う。


## アーキテクチャ

``` mermaid

flowchart TB

    subgraph Application
        Renderer
    end

    subgraph RHI
        Device
    end

    subgraph GraphicsAPI
        VulkanDevice
        OpenGLDevice
        DirectX12Device
    end

    Renderer --> Device
    Device --> VulkanDevice
    Device --> OpenGLDevice
    Device --> DirectX12Device

```

### RHIの主要なコンポーネント

- **Device**: グラフィックスデバイスを表し、リソースの作成やコマンドの発行などの基本的な操作を提供します。



## レンダーターゲット

描画先は `RenderTargetHandle` で指定する。`CommandBuffer::beginRendering(RenderTargetHandle, RenderingInfo)` が、そのターゲットへの描画パスを開始する。

- **`Device::createRenderTarget(width, height, color, depth)`**: テクスチャを描画先とするターゲットを作成する。テクスチャは `TextureUsage::ColorAttachment` / `DepthAttachment` を指定して作成しておく。ターゲットはテクスチャを所有しない。
- **`Device::createDefaultRenderTarget()`**: スワップチェーンの現在の画像と Device の深度バッファを指すターゲットを作成する。
- **`Device::destroyRenderTarget()`**: ほかのリソースと同様、GPU が使い終わるまで破棄を遅延する。

### レイアウト遷移（Vulkan）

オフスクリーンのターゲットについては、イメージレイアウトの遷移を RHI が自動で行う。

- `beginRendering()`: 各アタッチメントを、追跡中のレイアウトからアタッチメント用のレイアウトへ遷移する。前フレームでのサンプリングとの競合も、このバリアで解消する。まだ一度も描画されていない（`UNDEFINED`）アタッチメントは、強制的にクリアする。
- `endRendering()`: `Sampleable` なアタッチメントを `SHADER_READ_ONLY_OPTIMAL` へ遷移し、後続のパスからサンプリングできるようにする。

スワップチェーンのレイアウト遷移は、従来どおり Renderer が `transitionLayout()` で行う。

### パイプラインキャッシュ（Vulkan）

VkPipeline はアタッチメントのフォーマットに依存するため、`createPipeline()` はパイプラインレイアウトと設計図（シェーダ、頂点入力、ラスタステート、トポロジ）だけを作る。VkPipeline は `bindPipeline()` 時に `VulkanPipelineCache` から取得する（Filament の VulkanPipelineCache と同じ方式）。そのため、同じ Renderable をフォーマットの異なる任意のターゲットに描画できる。

- **キー**: VkPipeline の生成に使う全状態をまとめた `PipelineKey`。設計図に、現在のターゲットのカラーと深度のフォーマットを加えたもの。xxHash（XXH3）でハッシュし、等価判定はバイト列の比較で行う。
  - キーはバイト列として扱うため、パディングのない固定長の構造体にしている（`static_assert` で保証）。頂点属性は `VERTEX_ATTRIBUTE_COUNT_MAX` 個の固定長配列で、未使用分は 0 にする。
  - シェーダモジュールとパイプラインレイアウトは、Vulkan のハンドル値ではなく、再利用されない ID でキーに入れる。破棄されたオブジェクトと同じハンドル値の新しいオブジェクトが、古いエントリに一致することはない。
- **生成**: キャッシュにないキーは、その場で VkPipeline を生成して登録する。生成に失敗したキーも登録し、毎回の再試行は避ける。
- **破棄**: 各エントリには最後に使ったフレーム番号を記録する。`MAX_PIPELINE_AGE`（10）フレーム使われず、かつ GPU が使い終わったエントリは、フレームの開始時に破棄する。`destroyPipeline()` は VkPipeline を直接破棄せず、キャッシュ側で寿命切れを待つ。
- 現状、パイプラインレイアウトは `createPipeline()` ごとに作られる。そのため、同じシェーダと状態でも、別の `PipelineHandle` 同士で VkPipeline は共有されない。

- ビューポートとシザーは動的ステートとし、`beginRendering()` でターゲットのサイズに合わせて設定する。
- シェーダモジュールはパイプラインと共有所有される。`destroyShaderModule()` の後も、そのシェーダを使うパイプラインが残っている間は破棄されない。
