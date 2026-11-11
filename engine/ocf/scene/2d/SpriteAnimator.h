// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/resource/SpriteAnimation.h"
#include "ocf/scene/Component.h"

#include <cstddef>
#include <functional>
#include <string_view>
#include <utility>
#include <vector>

namespace ocf {

class Sprite;

/**
 * @brief Component that plays sprite animations on a Sprite.
 *
 * Animations are added by name and played with play(). The frame shown changes in the same
 * frame as the time passes, even when the animator is updated after the sprite.
 *
 * The sprite is not owned and must outlive the animator; adding both to the same node does
 * that. Without a sprite, the animator only keeps time and calls its callbacks.
 *
 * The callbacks may call play(), stop() and the other playback functions, but not
 * addAnimation(), which may move the animation they were given.
 */
class OCF_API SpriteAnimator : public Component {
public:
    using FrameChangedCallback =
        std::function<void(const SpriteAnimation& animation, size_t frameIndex)>;
    using FinishedCallback = std::function<void(const SpriteAnimation& animation)>;

    explicit SpriteAnimator(Sprite* sprite);
    ~SpriteAnimator() override;

    void update(float deltaTime) override;

    /**
     * @brief Add an animation. One with the same name is replaced.
     *
     * Replacing the animation that is playing starts it again from its first frame.
     *
     * @return false if the animation is not valid (see SpriteAnimation::isValid()); it is not
     *         added then.
     */
    bool addAnimation(SpriteAnimation animation);

    /** @brief The animation with the name, or nullptr. Valid until the next addAnimation(). */
    const SpriteAnimation* getAnimation(std::string_view name) const;

    /**
     * @brief Play an animation from its first frame.
     *
     * If the animation is already playing and has not finished, it goes on without starting
     * again unless restart is true, so this can be called every frame with the current state.
     * A paused animator stays paused.
     *
     * @return false if there is no animation with the name; nothing changes then.
     */
    bool play(std::string_view name, bool restart = false);

    /** @brief Stop on the current frame. play() starts again. */
    void stop() noexcept;

    /** @brief Pause or resume without losing the position in the animation. */
    void setPaused(bool paused) noexcept { m_paused = paused; }
    bool isPaused() const noexcept { return m_paused; }

    /** @brief Whether an animation was played and has neither finished nor been stopped. */
    bool isPlaying() const noexcept;

    /** @brief Whether an animation with AnimationLoop::Once reached its last frame. */
    bool isFinished() const noexcept { return m_finished; }

    /** @brief The animation played last, or nullptr. Valid until the next addAnimation(). */
    const SpriteAnimation* getCurrentAnimation() const noexcept;

    size_t getFrameIndex() const noexcept { return m_frameIndex; }

    /** @brief Show a frame of the current animation from its beginning. Ignored if out of range. */
    void setFrameIndex(size_t index);

    /** @brief Multiply the time passed to the animation. Negative values are treated as 0. */
    void setSpeed(float speed) noexcept { m_speed = speed > 0.0f ? speed : 0.0f; }
    float getSpeed() const noexcept { return m_speed; }

    /**
     * @brief Set the anchor of the sprite to the pivot of each frame.
     *
     * Off by default, as with Sprite::setSpriteFrame(), so the anchor set on the sprite is kept.
     */
    void setUseFramePivot(bool use);
    bool isUsingFramePivot() const noexcept { return m_useFramePivot; }

    /**
     * @brief Called when another frame is shown, including the first one on play().
     *
     * If several frames pass in one update, it is called once with the last of them.
     */
    void setOnFrameChanged(FrameChangedCallback callback) { m_onFrameChanged = std::move(callback); }

    /** @brief Called when an animation with AnimationLoop::Once reaches its last frame. */
    void setOnFinished(FinishedCallback callback) { m_onFinished = std::move(callback); }

private:
    static constexpr size_t NONE = static_cast<size_t>(-1);

    /** Move to the next frame. Returns false if the animation finished instead. */
    bool advance();
    void applyFrame();

    Sprite* m_sprite = nullptr;
    std::vector<SpriteAnimation> m_animations;

    size_t m_current = NONE; // Index into m_animations
    size_t m_frameIndex = 0;
    float m_frameTime = 0.0f; // Seconds the current frame has been shown
    int m_direction = 1; // -1 while a PingPong animation plays backward
    float m_speed = 1.0f;
    bool m_playing = false;
    bool m_paused = false;
    bool m_finished = false;
    bool m_useFramePivot = false;

    FrameChangedCallback m_onFrameChanged;
    FinishedCallback m_onFinished;
};

} // namespace ocf
