#include "AudioMixer.h"

#include <algorithm>

namespace ocf {
namespace audio {

void AudioMixer::render(float* output, unsigned int frameCount, unsigned int channels) {

	for (unsigned int i = 0; i < frameCount * channels; ++i) {
        m_cpuRemainder += CPU_FREQUENCY / static_cast<double>(SAMPLE_RATE);
        while (m_cpuRemainder >= 1.0) {
            m_apu.clock();
            m_cpuRemainder -= 1.0;
        }
        const float pcm = std::clamp(m_apu.getOutput(), -1.0f, 1.0f);

        output[i] = pcm * 0.25f;
    }
}


} // namespace audio
} // namespace ocf