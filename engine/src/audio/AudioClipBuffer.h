// SPDX-License-Identifier: MIT
#pragma once

#include "AudioBuffer.h"
#include <mutex>
#include <vector>

namespace ocf {
namespace audio {

class AudioClipBuffer : public AudioBuffer {
public:
    AudioClipBuffer() = default;
    explicit AudioClipBuffer(std::vector<float> data, uint32_t sampleRate, uint32_t channels);

    AudioClipBuffer(const AudioClipBuffer&) = delete;
    AudioClipBuffer& operator=(const AudioClipBuffer&) = delete;

    uint32_t getSampleRate() const noexcept override;

    uint32_t getChannelCount() const noexcept override;

    uint64_t getLengthFrames() const noexcept override;

    uint64_t tell() const noexcept override;

    void seek(uint64_t frame) override;

    void reset() override;

    uint32_t readFrames(float* outBuffer, uint32_t frames, uint32_t channels) override;

    bool isEnd() const noexcept override;

    Type getType() const noexcept override { return Type::Clip; }

private:
    std::vector<float> m_data;
    uint32_t m_sampleRate = 0;
    uint32_t m_channels = 0;
    uint64_t m_position = 0; // in frames
    mutable std::mutex m_mutex;
};

} // namespace audio
} // namespace ocf
