// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/compiler.h"
#include "ocf/resource/SpriteSheet.h"

#include <string>
#include <string_view>
#include <vector>

namespace ocf {

class Texture;

/** @brief What a sprite animation does after its last frame. */
enum class AnimationLoop {
    Once, // Stop on the last frame
    Loop, // Go back to the first frame
    PingPong, // Play backward to the first frame, then forward again, and so on
};

/** @brief A frame of a sprite animation and how long it is shown. */
struct OCF_API SpriteAnimationFrame {
    SpriteFrame frame;
    /** Seconds the frame is shown. Must be positive. */
    float duration = 0.0f;
};

/**
 * @brief Frames of a texture shown one after another, played by a SpriteAnimator.
 *
 * The frames are copied, so the animation stays valid after the sprite sheet is released. The
 * texture is not owned.
 */
struct OCF_API SpriteAnimation {
    std::string name;
    Texture* texture = nullptr;
    std::vector<SpriteAnimationFrame> frames;
    AnimationLoop loop = AnimationLoop::Loop;

    /** @brief Sum of the frame durations, for one pass through the frames. */
    float getDuration() const noexcept;

    /** @brief Whether there is a frame and every frame has a positive duration. */
    bool isValid() const noexcept;

    /**
     * @brief An animation showing each frame for 1 / framesPerSecond seconds.
     *
     * Null frames are skipped. The result is not valid if framesPerSecond is not positive.
     */
    static SpriteAnimation fromFrames(std::string name,
                                      const std::vector<const SpriteFrame*>& frames,
                                      Texture* texture, float framesPerSecond,
                                      AnimationLoop loop = AnimationLoop::Loop);

    /**
     * @brief An animation of the frames of a sheet whose names start with a prefix.
     *
     * The frames are sorted by name, as in SpriteSheet::getFramesWithPrefix(), so "walk_01.png",
     * "walk_02.png", ... play in order. The result has no frames if none matches.
     */
    static SpriteAnimation fromSheet(std::string name, const SpriteSheet& sheet,
                                     std::string_view prefix, float framesPerSecond,
                                     AnimationLoop loop = AnimationLoop::Loop);
};

} // namespace ocf
