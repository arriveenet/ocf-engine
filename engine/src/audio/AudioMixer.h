// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <vector>

namespace ocf {
namespace audio {

class AudioSource;

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
