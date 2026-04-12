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


