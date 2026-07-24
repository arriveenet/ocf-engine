// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/audio/AudioEnums.h"

#include <cstdint>
#include <vector>

namespace ocf {
namespace audio {

class AudioSource;

constexpr uint32_t InternalSampleRate = 48000;
constexpr uint32_t InternalChannels = 2;
constexpr AudioFormat InternalFormat = AudioFormat::F32;

class AudioMixer {
public:
    void render(float* output, uint32_t frameCount, uint32_t channels);

    void addSource(AudioSource* source);

    void removeSource(AudioSource* source);

private:
    std::vector<AudioSource*> m_sources;
};

} // namespace audio
} // namespace ocf
