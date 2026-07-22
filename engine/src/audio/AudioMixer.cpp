#include "AudioMixer.h"

#include "audio/AudioSource.h"

namespace ocf {
namespace audio {

void AudioMixer::render(float* output, uint32_t frameCount, uint32_t channels)
{
    for (auto& source : m_sources) {
        if (source->getState() == AudioSource::State::Playing) {
            source->render(output, frameCount, channels);
        }
    }
}


} // namespace audio
} // namespace ocf