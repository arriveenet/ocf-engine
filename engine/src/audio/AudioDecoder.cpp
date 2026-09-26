// SPDX-License-Identifier: MIT
#include "audio/AudioDecoder.h"
#include <string.h>

namespace ocf {
namespace audio {

uint64_t AudioDecoder::readFixedFrames(void* buffer, uint64_t frameCount)
{
    uint64_t framesRead = 0;
    uint64_t framesReadOnce = 0;
    char* byteBuffer = static_cast<char*>(buffer);

    do {
        framesReadOnce = read(byteBuffer + framesToBytes(framesRead), frameCount - framesRead);
        framesRead += framesReadOnce;
    } while (framesReadOnce != 0 && framesRead < frameCount);

    // If we didn't read enough frames, fill the rest of the buffer with zeros
    if (framesRead < frameCount) {
        memset(byteBuffer + framesToBytes(framesRead), 0x0,
               framesToBytes(frameCount - framesRead));
    }

    return framesRead;
}

bool AudioDecoder::isOpened() const
{
    return m_isOpened;
}

uint64_t AudioDecoder::getTotalFrames() const
{
    return m_totalFrames;
}

uint64_t AudioDecoder::framesToBytes(uint64_t frames) const
{
    return m_bytesPerBlock * frames;
}

uint64_t AudioDecoder::bytesToFrames(uint64_t bytes) const
{
    return bytes / m_bytesPerBlock;
}

uint32_t AudioDecoder::getSampleRate() const
{
    return m_sampleRate;
}

uint32_t AudioDecoder::getChannelCount() const
{
    return m_channelCount;
}

uint32_t AudioDecoder::getSamplesPerBlock() const
{
    return m_samplesPerBlock;
}

AudioFormat AudioDecoder::getFormat() const
{
    return m_format;
}

} // namespace audio
} // namespace ocf
