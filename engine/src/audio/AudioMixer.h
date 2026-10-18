// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <mutex>
#include <vector>

namespace ocf {
namespace audio {

class AudioSource;

/**
 * Mixes the playing sources on the audio thread. Sources may be added and removed from any
 * thread: once removeSource() returns, the audio thread no longer uses the source, so it may be
 * deleted.
 */
class AudioMixer {
public:
    void render(float* output, uint32_t frameCount, uint32_t channels);

    void addSource(AudioSource* source);

    void removeSource(AudioSource* source);

private:
    // Held during the whole render(), so that a removed source is never in use
    std::mutex m_mutex;
    std::vector<AudioSource*> m_sources;
    std::vector<float> m_sourceBuffer;
};

} // namespace audio
} // namespace ocf
