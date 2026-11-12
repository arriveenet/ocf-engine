# Animation

## 概要

アニメーションは次の 2 つのコンポーネントで行う。どちらも `Node` に付けて使い、毎フレームの `update()` で時間を進める。

| コンポーネント | 内容 |
| --- | --- |
| **SpriteAnimator** | `Sprite` に表示するフレームを時間で切り替える（パラパラ漫画）。名前を付けた複数のアニメーションを持ち、`play("walk")` で切り替える。 |
| **Tweener** | ノードの位置・回転・拡大と、2D コンポーネントの色を、イージングを付けて滑らかに変化させる（Tween）。順番に実行・同時に実行・繰り返しを組み合わせられる。 |

フレームアニメーションをしながら移動する、のように両方を同じノードに付けて併用できる。

| ファイル | 内容 |
| --- | --- |
| `engine/ocf/resource/SpriteAnimation.h` | `SpriteAnimation`（フレームと表示時間の列）、`SpriteAnimationFrame`、`AnimationLoop` |
| `engine/ocf/scene/2d/SpriteAnimator.h` | `SpriteAnimator` |
| `engine/ocf/scene/animation/Tween.h` | `Tween`（変化の単位と、それを作るファクトリ関数） |
| `engine/ocf/scene/animation/Tweener.h` | `Tweener`（Tween を実行するコンポーネント） |
| `engine/ocf/scene/animation/Easing.h` | `Ease`、`ease()` |
| `tests/sandbox/AnimationTest.cpp` | 動作確認シーン |

---

## SpriteAnimator

### 使い方

```cpp
#include <ocf/resource/SpriteAnimation.h>
#include <ocf/scene/2d/SpriteAnimator.h>

Sprite* sprite = node->addComponent<Sprite>(engine);
sprite->setTextureFilter(TextureFilter::Nearest);

SpriteAnimator* animator = node->addComponent<SpriteAnimator>(sprite);
// "walk_" で始まるフレームを名前順に、1 秒あたり 8 フレームで
animator->addAnimation(SpriteAnimation::fromSheet("walk", *sheet.ptr(), "walk_", 8.0f));
animator->addAnimation(SpriteAnimation::fromSheet("attack", *sheet.ptr(), "attack_", 12.0f,
                                                  AnimationLoop::Once));
animator->play("walk");

// 攻撃が終わったら歩きに戻す
animator->setOnFinished([animator](const SpriteAnimation&) { animator->play("walk"); });
```

### SpriteAnimation

`SpriteAnimation` は、フレーム（`SpriteFrame` のコピー）と各フレームの表示時間（秒）の列、テクスチャ、ループの仕方を持つ**データ**である。フレームをコピーして持つので、`SpriteSheet` を解放しても使える（テクスチャの寿命は `TextureManager` に依存する）。

| 作り方 | 内容 |
| --- | --- |
| `SpriteAnimation::fromSheet(name, sheet, prefix, fps, loop)` | 名前が `prefix` で始まるフレームを**名前順**に並べる。`walk_01.png`、`walk_02.png`… のように連番にしておく。 |
| `SpriteAnimation::fromFrames(name, frames, texture, fps, loop)` | 任意の `SpriteFrame` の列から作る。 |
| メンバを直接設定 | フレームごとに表示時間を変えたいときは `frames[i].duration` を設定する。 |

表示時間は正の値でなければならない。フレームがない、または表示時間が 0 以下（NaN を含む）のアニメーションは `addAnimation()` が警告を出して追加しない。

| `AnimationLoop` | 最後のフレームの後 |
| --- | --- |
| `Once` | 最後のフレームで止まり、`isFinished()` が true になり、`setOnFinished()` のコールバックが呼ばれる。 |
| `Loop`（既定） | 最初のフレームに戻る。 |
| `PingPong` | 逆順に最初のフレームまで戻り、また順に進む。両端のフレームは 1 往復につき 1 回だけ表示する（0, 1, 2, 1, 0, 1, …）。 |

### 再生の制御

| メソッド | 動作 |
| --- | --- |
| `play(name)` | 最初のフレームから再生する。**同じアニメーションを再生中なら何もしない**ので、状態に応じて毎フレーム呼んでよい。終わった `Once` のアニメーションは最初からやり直す。 |
| `play(name, true)` | 再生中でも最初からやり直す。 |
| `stop()` | 今のフレームで止める。`play()` で再開する。 |
| `setPaused(bool)` | 位置を保ったまま一時停止・再開する。 |
| `setSpeed(float)` | 再生速度の倍率。 |
| `setFrameIndex(i)` | 今のアニメーションの i 番目のフレームへ飛ぶ。 |
| `setUseFramePivot(true)` | フレームごとに `Sprite` のアンカーをフレームの pivot にする。既定は false（`Sprite` に設定したアンカーを使う）。 |
| `setOnFrameChanged(f)` | 表示するフレームが変わったときに呼ばれる（`play()` での最初のフレームも含む）。足音の再生などに使う。1 回の更新で複数のフレームが進んだ場合は、最後のフレームで 1 回だけ呼ばれる。 |
| `setOnFinished(f)` | `Once` のアニメーションが最後のフレームに達したときに呼ばれる。 |

同じ名前で `addAnimation()` すると置き換える。再生中のアニメーションを置き換えた場合は、最初のフレームからやり直す。

### 注意点

- `Sprite` は所有しない。`Sprite` は `SpriteAnimator` より長く生きていなければならない（同じノードに付ければよい）。`nullptr` を渡すと、時間を進めてコールバックを呼ぶだけになる。
- コールバックの中から `play()` や `stop()` などは呼んでよいが、`addAnimation()` は呼んではならない（コールバックに渡したアニメーションが移動することがある）。
- 1 フレームの経過時間が長いと、その分だけフレームを飛ばす。余った時間は次のフレームに持ち越すので、フレームレートが変わっても再生速度は変わらない。

### 更新順と表示の遅れ

`Node` はコンポーネントを追加した順に更新し、`Sprite` は自分の `update()` でジオメトリを作り直す。そのため、`Sprite` の後に追加したコンポーネントがフレームを変えると、表示は次のフレームまで遅れてしまう。

`SpriteAnimator` はフレームを変えた直後に `Drawable2D::updateGeometry()` を呼んで、同じフレームのうちに反映する。自作のコンポーネントから `Sprite` などを変更する場合も同じ方法が使える。

---

## Tween と Tweener

### 使い方

```cpp
#include <ocf/scene/animation/Tweener.h>

Tweener* tweener = node->addComponent<Tweener>();

// 右へ移動してからフェードアウトする
tweener->run(Tween::sequence(
    Tween::moveBy(math::vec3(3.0f, 0.0f, 0.0f), 1.0f, Ease::OutQuad),
    Tween::fadeTo(sprite, 0.0f, 0.5f)));

// 上下にふわふわ動き続ける
Tweener::TweenId id = tweener->run(Tween::repeat(Tween::sequence(
    Tween::moveBy(math::vec3(0.0f, 0.2f, 0.0f), 1.0f, Ease::InOutSine),
    Tween::moveBy(math::vec3(0.0f, -0.2f, 0.0f), 1.0f, Ease::InOutSine))));
tweener->stop(id);
```

### Tween の種類

| 関数 | 内容 |
| --- | --- |
| `moveTo` / `moveBy` | ノードの位置（親の空間）。`By` は開始時の位置からの相対値。 |
| `rotateTo` / `rotateBy` | ノードの回転（`Node::setRotation()` と同じ度数法のオイラー角）。`rotateBy` は 360° を超えて回せる。 |
| `scaleTo` / `scaleBy` | ノードの拡大。`scaleBy` は軸ごとの**倍率**を掛ける。 |
| `colorTo(target, color)` | `Drawable2D`（`Sprite`、`Text` など）の色。 |
| `fadeTo(target, alpha)` | `Drawable2D` の色のアルファだけ。 |
| `custom(f, duration)` | イージング後の進み具合（0 → 1）で `f` を毎フレーム呼ぶ。ファクトリのない値の変化に使う。終了時は必ず 1 で呼ばれる。 |
| `delay(duration)` | 待つだけ。 |
| `call(f)` | `f` を 1 回呼んですぐ終わる。 |
| `sequence(a, b, ...)` | 順番に実行する。 |
| `parallel(a, b, ...)` | 同時に実行し、最も長いものが終わったときに終わる。 |
| `repeat(t, count)` | `count` 回繰り返す。0 以下なら終わらない。 |

`colorTo` / `fadeTo` の対象は所有しない。対象は Tween より長く生きていなければならない。

### 開始値を読むタイミング

各 Tween は**作ったときではなく、始まったとき**に開始値（今の位置など）を読む。

- `Tweener::run()` した Tween は、**次の `Tweener::update()`** で始まる。`run()` の後に同じフレームでノードの位置を設定しても、そこから動き出す。
- `sequence()` の中の Tween は、前の Tween が終わったときに始まる。`moveBy()` を続けると、前の移動の終点から動く。
- `repeat()` は毎回始め直して開始値を読み直す。`repeat(moveBy(d))` は動き続け、`repeat(sequence(moveBy(d), moveBy(-d)))` は往復する。

### 時間の扱い

- `sequence()` は、終わった Tween の余った時間を次の Tween に渡す。1 フレームが長くても全体の長さは変わらない。
- `duration` が 0 の Tween は、最初の更新ですぐに終点の値になる。
- `repeat()` は 1 回の更新の中で複数回まわることがあるが、時間を使わない Tween（`call()` だけなど）の無限の繰り返しは 1 回の更新につき 1 回にとどめる。

### Tweener

| メソッド | 動作 |
| --- | --- |
| `run(tween)` | Tween を実行し、ID を返す（0 は使わない）。複数同時に実行でき、`run()` した順に更新する。 |
| `stop(id)` / `stopAll()` | その場で止める（終点には移動しない）。 |
| `isRunning(id)` / `isRunning()` | 実行中かどうか。 |
| `setPaused(bool)` | すべての Tween を一時停止する。 |
| `setTimeScale(float)` | 経過時間の倍率。 |

`call()` や `custom()` のコールバックの中から、同じ `Tweener` の `run()` / `stop()` を呼んでよい。コールバックの中で `run()` した Tween は次の更新から始まる。ほかのコンポーネントと同じく、更新中にこのコンポーネントやノードを破棄してはならない。

同じノードの同じ値を複数の Tween が同時に変えると、後に `run()` したものの値が残る。

### イージング

`Ease` は [easings.net](https://easings.net) の曲線である。`In` はゆっくり始まり、`Out` はゆっくり終わり、`InOut` は両方。

| 系統 | 値 |
| --- | --- |
| 等速 | `Linear` |
| 多項式 | `InQuad` `OutQuad` `InOutQuad` `InCubic` `OutCubic` `InOutCubic` |
| 正弦 | `InSine` `OutSine` `InOutSine` |
| 指数 | `InExpo` `OutExpo` `InOutExpo` |
| 行き過ぎて戻る | `InBack` `OutBack` `InOutBack` |
| ばね | `InElastic` `OutElastic` `InOutElastic` |
| 跳ねる | `InBounce` `OutBounce` `InOutBounce` |

`ease(curve, t)` は t = 0 で厳密に 0、t = 1 で厳密に 1 を返すので、Tween は必ず目標値で終わる。`Back` と `Elastic` は途中で 0〜1 の範囲を超える。

---

## 制限事項・今後の拡張

- フレームを切り替えるたびに `Sprite` の頂点バッファを作り直す（`SpriteSheet` と同じ制限）。
- アニメーションをファイル（Aseprite の JSON など）から読み込む機能はない。
- Tween の逆再生（yoyo）はない。往復は `sequence()` で逆向きの Tween を並べる。
- カメラは `lookAt()` で位置が決まるため、カメラのノードを Tween しても視点は動かない（[scene.md](scene.md) を参照）。`custom()` の中で `lookAt()` を呼ぶ。
