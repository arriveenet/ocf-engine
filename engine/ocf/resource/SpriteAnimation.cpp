// SPDX-License-Identifier: MIT
#include "ocf/resource/SpriteAnimation.h"

#include <algorithm>
#include <utility>

namespace ocf {

float SpriteAnimation::getDuration() const noexcept
{
    float duration = 0.0f;
    for (const SpriteAnimationFrame& frame : frames) {
        duration += frame.duration;
    }
    return duration;
}

bool SpriteAnimation::isValid() const noexcept
{
    // Written as !(> 0) so that NaN durations are rejected as well
    return !frames.empty() &&
           std::none_of(frames.begin(), frames.end(), [](const SpriteAnimationFrame& frame) {
               return !(frame.duration > 0.0f);
           });
}

SpriteAnimation SpriteAnimation::fromFrames(std::string name,
                                            const std::vector<const SpriteFrame*>& frames,
                                            Texture* texture, float framesPerSecond,
                                            AnimationLoop loop)
{
    SpriteAnimation animation;
    animation.name = std::move(name);
    animation.texture = texture;
    animation.loop = loop;

    const float duration = framesPerSecond > 0.0f ? 1.0f / framesPerSecond : 0.0f;
    animation.frames.reserve(frames.size());
    for (const SpriteFrame* frame : frames) {
        if (frame != nullptr) {
            animation.frames.push_back(SpriteAnimationFrame{*frame, duration});
        }
    }
    return animation;
}

SpriteAnimation SpriteAnimation::fromSheet(std::string name, const SpriteSheet& sheet,
                                           std::string_view prefix, float framesPerSecond,
                                           AnimationLoop loop)
{
    return fromFrames(std::move(name), sheet.getFramesWithPrefix(prefix), sheet.getTexture(),
                      framesPerSecond, loop);
}

} // namespace ocf
