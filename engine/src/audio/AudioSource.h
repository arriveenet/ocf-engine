// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/audio/AudioEnums.h"

#include <algorithm>
#include <cstdint>

namespace ocf {
namespace audio {

class AudioBuffer;

class AudioSource {
public:
    AudioSource() = default;
    AudioSource(AudioBuffer* buffer);

    virtual ~AudioSource() = default;

    virtual void update();

    virtual void render(float* output, uint32_t frameCount, uint32_t channels);

    virtual void play();

    virtual void pause();

    virtual void stop();

    AudioBuffer* getBuffer() const noexcept { return m_buffer; }

    void setBuffer(AudioBuffer* buffer) { m_buffer = buffer; }

    AudioState getState() const noexcept { return m_state; }

    bool isPlaying() const noexcept { return m_state == AudioState::Playing; }

    bool isPaused() const noexcept { return m_state == AudioState::Paused; }

    bool isStopped() const noexcept { return m_state == AudioState::Stopped; }

    float getVolume() const noexcept { return m_volume; }

    void setVolume(float volume) noexcept { m_volume = std::clamp(volume, 0.0f, 1.0f); }

    bool isLooping() const noexcept { return m_looping; }   

    void setLooping(bool looping) noexcept { m_looping = looping; }

protected:
    AudioState m_state = AudioState::Initial;
    float m_volume = 1.0f;
    bool m_looping = false;
    AudioBuffer* m_buffer = nullptr;
};

} // namespace audio
} // namespace ocf
