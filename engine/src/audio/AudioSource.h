// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/audio/AudioEnums.h"

#include <algorithm>
#include <atomic>
#include <cstdint>

namespace ocf {
namespace audio {

class AudioBuffer;

/**
 * The state, volume and looping flag are atomic: the main thread sets them while the audio
 * thread (AudioMixer::render()) reads them.
 */
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
    std::atomic<AudioState> m_state = AudioState::Initial;
    std::atomic<float> m_volume = 1.0f;
    std::atomic<bool> m_looping = false;
    AudioBuffer* m_buffer = nullptr;
};

} // namespace audio
} // namespace ocf
