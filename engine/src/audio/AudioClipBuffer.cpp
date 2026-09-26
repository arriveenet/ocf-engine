#include "audio/AudioClipBuffer.h"

#include <algorithm>
#include <cstring>

namespace ocf {
namespace audio {

AudioClipBuffer::AudioClipBuffer(std::vector<float> data, uint32_t sampleRate, uint32_t channels)
    : m_data(std::move(data))
    , m_sampleRate(sampleRate)
    , m_channels(channels)
    , m_position(0)
{
}

uint32_t AudioClipBuffer::getSampleRate() const noexcept
{
    return m_sampleRate;
}

uint32_t AudioClipBuffer::getChannelCount() const noexcept
{
    return m_channels;
}

uint64_t AudioClipBuffer::getLengthFrames() const noexcept
{
    if (m_channels == 0)
        return 0;
    return static_cast<uint64_t>(m_data.size()) / m_channels;
}

uint64_t AudioClipBuffer::tell() const noexcept
{
    return m_position;
}

void AudioClipBuffer::seek(uint64_t frame)
{
    uint64_t len = getLengthFrames();
    if (len == 0) {
        m_position = 0;
        return;
    }
    m_position = (frame > len) ? len : frame;
}

void AudioClipBuffer::reset()
{
    m_position = 0;
}

uint32_t AudioClipBuffer::readFrames(float* outBuffer, uint32_t frames, uint32_t channels)
{
    if (!outBuffer || frames == 0)
        return 0;

    std::lock_guard<std::mutex> lock(m_mutex);
    uint64_t available = getLengthFrames();
    if (m_position >= available)
        return 0;

    uint32_t framesToCopy =
        static_cast<uint32_t>(std::min<uint64_t>(frames, available - m_position));

    const uint32_t srcChannels = m_channels;
    for (uint32_t f = 0; f < framesToCopy; ++f) {
        uint64_t srcIndex = (m_position + f) * srcChannels;
        uint32_t dstIndex = f * channels;
        uint32_t common = std::min(srcChannels, channels);
        std::memcpy(outBuffer + dstIndex, &m_data[srcIndex], sizeof(float) * common);
        if (channels > common) {
            std::memset(outBuffer + dstIndex + common, 0, sizeof(float) * (channels - common));
        }
    }

    m_position += framesToCopy;
    return framesToCopy;
}

bool AudioClipBuffer::isEnd() const noexcept
{
    return m_position >= getLengthFrames();
}

} // namespace audio
} // namespace ocf
