// SPDX-License-Identifier: MIT
#include "AudioMixer.h"

#include "audio/AudioSource.h"

#include <algorithm>
#include <vector>

namespace ocf {
namespace audio {

void AudioMixer::render(float* output, uint32_t frameCount, uint32_t channels)
{
    std::fill(output, output + (frameCount * channels), 0.0f);

    std::vector<float> sourceBuffer(frameCount * channels, 0.0f);

    for (auto& source : m_sources) {
        if (source->isStopped()) {
            removeSource(source);
            continue;
        }

        source->render(sourceBuffer.data(), frameCount, channels);

        // Mix the source buffer into the output buffer
        for (uint32_t i = 0; i < frameCount * channels; i++) {
            output[i] += sourceBuffer[i] * source->getVolume();
        }
    }

    // Soft clip the output to the range [-1.0, 1.0]
    for (uint32_t i = 0; i < frameCount * channels; i++) {
        output[i] = std::clamp(output[i], -1.0f, 1.0f);
    }
}

void AudioMixer::addSource(AudioSource* source)
{
    auto iter = std::find(m_sources.begin(), m_sources.end(), source);
    if (iter == m_sources.end()) {
        m_sources.push_back(source);
    }
}

void AudioMixer::removeSource(AudioSource* source)
{
    auto iter = std::find(m_sources.begin(), m_sources.end(), source);
    if (iter != m_sources.end()) {
        m_sources.erase(iter);
    }
}

} // namespace audio
} // namespace ocf