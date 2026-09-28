// SPDX-License-Identifier: MIT
#include "audio/AudioSource.h"

#include "audio/AudioBuffer.h"
#include "audio/AudioUtility.h"

#include "ocf/core/Logger.h"

namespace ocf {
namespace audio {

AudioSource::AudioSource(AudioBuffer* buffer)
    : m_buffer(buffer)
{
}

void AudioSource::update()
{
    if (m_buffer == nullptr) {
        return;
    }

    if (m_buffer->isEnd()) {
        if (m_looping) {
            m_buffer->seek(0);
        }
        else {
            m_state = AudioState::Stopped;
        }
    }
}

void AudioSource::render(float* output, uint32_t frameCount, uint32_t channels)
{
    if ((m_buffer == nullptr) || (m_state != AudioState::Playing)) {
        return;
    }

    const uint32_t samplesPulled = m_buffer->readFrames(output, frameCount, channels);

    const size_t bytesPerFrame = AudioUtility::getFormatSize(InternalFormat) * channels;
    const size_t requiredBytes = frameCount * bytesPerFrame;
    
    if (samplesPulled * bytesPerFrame < requiredBytes) {
        OCF_LOG_WARN("[Audio] AudioSource: Not enough samples pulled from buffer. Requested {} "
                     "frames, but only got {} frames.",
                     frameCount, samplesPulled);
    }
}

void AudioSource::play()
{
    if (m_buffer == nullptr) {
        OCF_LOG_ERROR("[Audio] Cannot play AudioSource: buffer is null");
        return;
    }

    if (m_state != AudioState::Paused) {
        m_buffer->reset();
    }

    m_state = AudioState::Playing;
}

void AudioSource::pause()
{
    if (m_state == AudioState::Playing) {
        m_state = AudioState::Paused;
    }
}

void AudioSource::stop()
{
    if (m_buffer != nullptr) {
        m_buffer->reset();
    }
    m_state = AudioState::Stopped;
}

} // namespace audio
} // namespace ocf
