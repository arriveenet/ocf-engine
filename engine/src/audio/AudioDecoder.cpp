#include "audio/AudioDecoder.h"
#include <string.h>

namespace ocf {
namespace audio {

uint32_t AudioDecoder::readFixedFrames(void* buffer, uint32_t frameCount)
{
    uint32_t framesRead = 0;
    uint32_t framesReadOnce = 0;
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

uint32_t AudioDecoder::getTotalFrames() const
{
    return m_totalFrames;
}

uint32_t AudioDecoder::framesToBytes(uint32_t frames) const
{
    return m_bytesPerBlock * frames;
}

uint32_t AudioDecoder::bytesToFrames(uint32_t bytes) const
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
