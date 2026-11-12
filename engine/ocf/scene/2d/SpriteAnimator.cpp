// SPDX-License-Identifier: MIT
#include "ocf/scene/2d/SpriteAnimator.h"

#include "ocf/core/Logger.h"
#include "ocf/scene/2d/Sprite.h"

#include <algorithm>
#include <utility>

namespace ocf {

SpriteAnimator::SpriteAnimator(Sprite* sprite)
    : m_sprite(sprite)
{
}

SpriteAnimator::~SpriteAnimator() = default;

void SpriteAnimator::update(float deltaTime)
{
    if (!isPlaying() || m_paused) {
        return;
    }

    const size_t current = m_current;
    const size_t previousFrame = m_frameIndex;
    const std::vector<SpriteAnimationFrame>& frames = m_animations[current].frames;

    // Durations are positive (checked in addAnimation()), so the loop ends
    m_frameTime += deltaTime * m_speed;
    while (m_frameTime >= frames[m_frameIndex].duration) {
        m_frameTime -= frames[m_frameIndex].duration;
        if (!advance()) {
            m_frameTime = 0.0f;
            m_finished = true;
            break;
        }
    }

    const bool finished = m_finished;
    if (m_frameIndex != previousFrame) {
        applyFrame();
        if (m_onFrameChanged) {
            m_onFrameChanged(m_animations[current], m_frameIndex);
        }
    }
    // The frame callback may have played another animation
    if (finished && m_current == current && m_finished && m_onFinished) {
        m_onFinished(m_animations[current]);
    }
}

bool SpriteAnimator::addAnimation(SpriteAnimation animation)
{
    if (!animation.isValid()) {
        OCF_LOG_WARN("SpriteAnimator: animation \"{}\" needs frames with positive durations",
                     animation.name);
        return false;
    }

    auto it = std::find_if(m_animations.begin(), m_animations.end(),
                           [&animation](const SpriteAnimation& existing) {
                               return existing.name == animation.name;
                           });
    if (it == m_animations.end()) {
        m_animations.push_back(std::move(animation));
        return true;
    }

    *it = std::move(animation);
    const size_t index = static_cast<size_t>(it - m_animations.begin());
    if (index == m_current) {
        // The frame index may be out of range for the new frames
        const bool playing = m_playing;
        play(m_animations[index].name, true);
        m_playing = playing;
    }
    return true;
}

const SpriteAnimation* SpriteAnimator::getAnimation(std::string_view name) const
{
    for (const SpriteAnimation& animation : m_animations) {
        if (animation.name == name) {
            return &animation;
        }
    }
    return nullptr;
}

bool SpriteAnimator::play(std::string_view name, bool restart)
{
    auto it = std::find_if(m_animations.begin(), m_animations.end(),
                           [name](const SpriteAnimation& animation) {
                               return animation.name == name;
                           });
    if (it == m_animations.end()) {
        OCF_LOG_WARN("SpriteAnimator: no animation \"{}\"", name);
        return false;
    }

    const size_t index = static_cast<size_t>(it - m_animations.begin());
    if (index == m_current && isPlaying() && !restart) {
        return true;
    }

    m_current = index;
    m_frameIndex = 0;
    m_frameTime = 0.0f;
    m_direction = 1;
    m_playing = true;
    m_finished = false;

    applyFrame();
    if (m_onFrameChanged) {
        m_onFrameChanged(m_animations[index], m_frameIndex);
    }
    return true;
}

void SpriteAnimator::stop() noexcept
{
    m_playing = false;
}

bool SpriteAnimator::isPlaying() const noexcept
{
    return m_current != NONE && m_playing && !m_finished;
}

const SpriteAnimation* SpriteAnimator::getCurrentAnimation() const noexcept
{
    return m_current != NONE ? &m_animations[m_current] : nullptr;
}

void SpriteAnimator::setFrameIndex(size_t index)
{
    if (m_current == NONE || index >= m_animations[m_current].frames.size()) {
        return;
    }
    m_frameTime = 0.0f;
    if (index != m_frameIndex) {
        m_frameIndex = index;
        applyFrame();
        if (m_onFrameChanged) {
            m_onFrameChanged(m_animations[m_current], m_frameIndex);
        }
    }
}

void SpriteAnimator::setUseFramePivot(bool use)
{
    if (m_useFramePivot != use) {
        m_useFramePivot = use;
        if (m_current != NONE) {
            applyFrame();
        }
    }
}

bool SpriteAnimator::advance()
{
    const SpriteAnimation& animation = m_animations[m_current];
    const size_t last = animation.frames.size() - 1;

    switch (animation.loop) {
    case AnimationLoop::Once:
        if (m_frameIndex >= last) {
            return false;
        }
        ++m_frameIndex;
        return true;

    case AnimationLoop::Loop:
        m_frameIndex = m_frameIndex >= last ? 0 : m_frameIndex + 1;
        return true;

    case AnimationLoop::PingPong:
        if (last == 0) {
            return true;
        }
        // The end frames are shown once per turn, not twice
        if ((m_direction > 0 && m_frameIndex >= last) || (m_direction < 0 && m_frameIndex == 0)) {
            m_direction = -m_direction;
        }
        m_frameIndex = m_direction > 0 ? m_frameIndex + 1 : m_frameIndex - 1;
        return true;
    }
    return true;
}

void SpriteAnimator::applyFrame()
{
    if (m_sprite == nullptr || m_current == NONE) {
        return;
    }

    const SpriteAnimation& animation = m_animations[m_current];
    const SpriteFrame& frame = animation.frames[m_frameIndex].frame;
    m_sprite->setSpriteFrame(frame, animation.texture);
    if (m_useFramePivot) {
        m_sprite->setAnchor(frame.pivot);
    }
    // The sprite may have been updated before this component in this frame
    m_sprite->updateGeometry();
}

} // namespace ocf
