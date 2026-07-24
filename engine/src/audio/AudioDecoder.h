// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/audio/AudioEnums.h"

#include <cstdint>
#include <string_view>

namespace ocf {
namespace audio {

class AudioDecoder {
public:
    AudioDecoder() = default;
    virtual ~AudioDecoder() = default;

    virtual bool open(std::string_view filename) = 0;

    virtual void close() = 0;

    virtual uint32_t read(void* buffer, uint32_t frameCount) = 0;

    virtual uint32_t readFixedFrames(void* buffer, uint32_t frameCount);

    virtual bool seek(uint32_t frameOffset) = 0;

    virtual bool isOpened() const;

    virtual uint32_t getTotalFrames() const;

    virtual uint32_t framesToBytes(uint32_t frames) const;

    virtual uint32_t bytesToFrames(uint32_t bytes) const;

    virtual uint32_t getSampleRate() const;

    virtual uint32_t getChannelCount() const;

    virtual uint32_t getSamplesPerBlock() const;

    virtual AudioFormat getFormat() const;

protected:
    bool m_isOpened = false;
    uint32_t m_totalFrames = 0;
    uint32_t m_bytesPerBlock = 0;
    uint32_t m_samplesPerBlock = 1;
    uint32_t m_sampleRate = 0;
    uint32_t m_channelCount = 0;
    AudioFormat m_format = AudioFormat::Unknown;
};

} // namespace audio
} // namespace ocf
