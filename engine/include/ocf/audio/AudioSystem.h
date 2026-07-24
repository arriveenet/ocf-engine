// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/audio/AudioEnums.h"

#include <memory>
#include <string_view>

namespace ocf {
namespace audio {

class AudioDevice;
class AudioMixer;

class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    bool initialize();

    void shutdown();

    void update();

    AudioHandle load(std::string_view filename);

    void play(AudioHandle handle, bool loop = false, float volume = 1.0f);

    void stop(AudioHandle handle);

    void pause(AudioHandle handle);

    void setVolume(AudioHandle handle, float volume);

    void setLoop(AudioHandle handle, bool loop);

    bool isInitialized() const noexcept { return m_initialized; }

private:
    struct Imple;
    std::unique_ptr<Imple> m_imple;
    bool m_initialized = false;
    uint32_t m_HandleCounter = 0;
};

} // namespace audio
} // namespace ocf
