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

    virtual uint64_t read(void* buffer, uint64_t frameCount) = 0;

    virtual uint64_t readFixedFrames(void* buffer, uint64_t frameCount);

    virtual bool seek(uint64_t frameOffset) = 0;

    virtual uint64_t tell() = 0;

    virtual bool isOpened() const;

    virtual uint64_t getTotalFrames() const;

    virtual uint64_t framesToBytes(uint64_t frames) const;

    virtual uint64_t bytesToFrames(uint64_t bytes) const;

    virtual uint32_t getSampleRate() const;

    virtual uint32_t getChannelCount() const;

    virtual uint32_t getSamplesPerBlock() const;

    virtual AudioFormat getFormat() const;

protected:
    bool m_isOpened = false;
    uint64_t m_totalFrames = 0;
    uint32_t m_bytesPerBlock = 0;
    uint32_t m_samplesPerBlock = 1;
    uint32_t m_sampleRate = 0;
    uint32_t m_channelCount = 0;
    AudioFormat m_format = AudioFormat::Unknown;
};

} // namespace audio
} // namespace ocf
