#pragma once

#include <algorithm>
#include <cstdint>

namespace ocf {
namespace audio {

class AudioSource {
public:
    enum class State {
        Initial,
        Playing,
        Paused,
        Stopped
    };

    virtual ~AudioSource() = default;

    virtual void render(float* output, uint32_t frameCount, uint32_t channels) = 0;

    virtual void update() {};

    void play() { m_state = State::Playing; }

    void pause() { m_state = State::Paused; }

    void stop() { m_state = State::Stopped; }

    State getState() const noexcept { return m_state; }

    float getVolume() const noexcept { return m_volume; }

    void setVolume(float volume) noexcept { m_volume = std::clamp(volume, 0.0f, 1.0f); }

    bool isLooping() const noexcept { return m_looping; }   

    void setLooping(bool looping) noexcept { m_looping = looping; }

protected:
    State m_state = State::Initial;
    float m_volume = 1.0f;
    bool m_looping = false;
};

} // namespace audio
} // namespace ocf
