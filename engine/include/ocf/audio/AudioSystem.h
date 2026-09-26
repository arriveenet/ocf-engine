// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/audio/AudioEnums.h"

#include <cstdint>
#include <memory>
#include <string_view>

namespace ocf {
namespace audio {

class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    bool initialize();

    void shutdown();

    void update();

    AudioBufferHandle createStreamBuffer(std::string_view filename);

    AudioSourceHandle createSource(AudioBufferHandle bufferHandle);

    AudioSourceHandle createNesApuSource();

    void destroyBuffer(AudioBufferHandle handle);

    void destroySource(AudioSourceHandle handle);

    void play(AudioSourceHandle handle, bool loop = false, float volume = 1.0f);

    void stop(AudioSourceHandle handle);

    void pause(AudioSourceHandle handle);

    AudioState getState(AudioSourceHandle handle) const;

    float getVolume(AudioSourceHandle handle) const;

    void setVolume(AudioSourceHandle handle, float volume);

    bool isLoop(AudioSourceHandle handle) const;

    void setLoop(AudioSourceHandle handle, bool loop);

    bool isInitialized() const noexcept { return m_initialized; }

    // APU specific functions
    void apu_writeRegister(AudioSourceHandle handle, uint16_t address, uint8_t data);

    uint8_t apu_readStatusRegister(AudioSourceHandle handle);

    void apu_writeStatusRegister(AudioSourceHandle handle, uint8_t data);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    bool m_initialized = false;
    uint32_t m_HandleCounter = 0;
};

} // namespace audio
} // namespace ocf
