# Renderer

## 概要

`Renderer` は、glTFやOBJ等のファイルから読み込んだモデルデータをはじめ、メッシュなど各種ポリゴンを画面やテクスチャへ描画するための**ゲームエンジンの基幹描画システム**である。

画面（フレームバッファ）への直接描画だけでなく、オフスクリーンレンダリング（テクスチャへの描画）にも対応する。

## 主要コンポーネント

本システムは、「データ構造（Scene）」「撮影条件（View）」「描画実行（Renderer）」の3つの要素に責務を分離して構成する。

| コンポーネント | 役割・責務 |
| --- | --- |
| **Scene** | 描画対象となるオブジェクト群を保持する**シーングラフ構造**。何が存在するか（メッシュ・トランスフォームなど）を管理するが、**どのように・どこへ描画するか**の情報は持たない。 |
| **View** | Sceneをどのように撮影し、どこへ出力するかを設定する **描画条件**。 カメラパラメータ（位置・視角・行列）、ビューポート、および **描画先（画面 / RenderTexture）** を保持する。 |
| **Renderer** | View の設定に基づいて Scene から必要なデータを取得し、実際にGPUへ描画コマンドを発行する**描画実行エンジン**。 |

---

## 描画パイプライン

1フレームの描画処理は、以下の手順で実行される。

1. **Scene の構築・更新**
* オブジェクトの親子関係に基づくワールド行列の更新を行う。

2. **View の設定**
* カメラのパラメータ（投影行列・ビュー行列）やビューポート設定、出力先ターゲット（画面フレームバッファまたは RenderTarget）を指定する。

3. **Renderer への情報渡すと描画処理**
* **可視判定（Culling）**: Viewの視体積（Frustum）に基づき、Sceneから描画領域内にある可視ノードのみを抽出する。
* **描画最適化（Sorting）**: パイプライン切り替えの低減や不透明・半透明の適切な描画順序を確保するため、ノードを整列する。
* **描画コマンド発行**: Viewで指定された描画ターゲットに対して、メッシュおよびマテリアル情報を元に描画処理を実行する。

---

## レンダーターゲット

`RenderTarget` は、View の描画先をスワップチェーンからテクスチャに切り替えるためのクラスである。シャドウマップや、シーンをテクスチャへ描いて別の View で使う用途を想定している。設計は Google Filament の `RenderTarget` に合わせている。

* アタッチメントにするテクスチャは、`Texture::Builder::usage()` で `ColorAttachment` または `DepthAttachment` を指定して作成する。シェーダで読む場合は `Sampleable` も指定する。
* `RenderTarget` はテクスチャを参照するだけで所有しない。破棄は `RenderTarget::terminate()` とは別に行う。
* カラーと深度のどちらかは省略できる（例: シャドウマップ用の深度のみのターゲット）。
* `View::setRenderTarget()` に `nullptr` を設定すると、スワップチェーンに描画する（既定）。

```cpp
Texture* color = Texture::Builder()
                     .width(512).height(512)
                     .format(Texture::InternalFormat::RGBA8)
                     .usage(Texture::Usage::ColorAttachment | Texture::Usage::Sampleable)
                     .build(engine);
Texture* depth = Texture::Builder()
                     .width(512).height(512)
                     .format(Texture::InternalFormat::DEPTH32F)
                     .usage(Texture::Usage::DepthAttachment)
                     .build(engine);
RenderTarget* target = RenderTarget::Builder()
                           .texture(RenderTarget::AttachmentPoint::Color, color)
                           .texture(RenderTarget::AttachmentPoint::Depth, depth)
                           .build(engine);

offscreenView->setRenderTarget(target);
offscreenView->setOrder(-1); // テクスチャを使う View より先に描く
```

### 利用上の注意

* テクスチャを描く View は、そのテクスチャをサンプリングする View より先に描画されるよう、`View::setOrder()` で順序を指定する。
* カメラのアスペクト比は、利用側でターゲットのサイズに合わせる。
* ターゲットの内容はフレームをまたいで保持される。初めて描画するときは、`ClearFlags` に関係なくクリアされる。
* 使用例は `tests/sandbox/RenderTargetTest.cpp` を参照。
