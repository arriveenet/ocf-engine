// SPDX-License-Identifier: MIT
#include "AudioMixer.h"

#include "audio/AudioSource.h"

#include <algorithm>
#include <vector>

namespace ocf {
namespace audio {

void AudioMixer::render(float* output, uint32_t frameCount, uint32_t channels)
{
    const size_t sampleCount = static_cast<size_t>(frameCount) * channels;
    std::fill(output, output + sampleCount, 0.0f);

    std::lock_guard<std::mutex> lock(m_mutex);

    // Grows only when the device asks for more frames than before
    if (m_sourceBuffer.size() < sampleCount) {
        m_sourceBuffer.resize(sampleCount);
    }

    for (AudioSource* source : m_sources) {
        if (source->isStopped()) {
            continue;
        }

        // A source that does not write (e.g. not playing) must not mix the previous one again
        std::fill(m_sourceBuffer.begin(), m_sourceBuffer.begin() + sampleCount, 0.0f);
        source->render(m_sourceBuffer.data(), frameCount, channels);

        // Mix the source buffer into the output buffer
        const float volume = source->getVolume();
        for (size_t i = 0; i < sampleCount; i++) {
            output[i] += m_sourceBuffer[i] * volume;
        }
    }

    // Removed after the loop, since erasing while iterating invalidates the iterators
    std::erase_if(m_sources, [](const AudioSource* source) { return source->isStopped(); });

    // Soft clip the output to the range [-1.0, 1.0]
    for (size_t i = 0; i < sampleCount; i++) {
        output[i] = std::clamp(output[i], -1.0f, 1.0f);
    }
}

void AudioMixer::addSource(AudioSource* source)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto iter = std::find(m_sources.begin(), m_sources.end(), source);
    if (iter == m_sources.end()) {
        m_sources.push_back(source);
    }
}

void AudioMixer::removeSource(AudioSource* source)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto iter = std::find(m_sources.begin(), m_sources.end(), source);
    if (iter != m_sources.end()) {
        m_sources.erase(iter);
    }
}

} // namespace audio
} // namespace ocf
