// SPDX-License-Identifier: MIT
#pragma once

#include "audio/AudioBuffer.h"
#include "audio/AudioConverter.h"
#include "audio/AudioDecoder.h"
#include "audio/AudioMixer.h"

#include <miniaudio.h>

#include <atomic>
#include <functional>
#include <memory>

namespace ocf {
namespace audio {

class AudioStreamBuffer : public AudioBuffer {
public:
    /** @brief Number of bytes in one frame (based on internal channel count). */
    static constexpr size_t BytesPerFrame = sizeof(float) * InternalChannels;

    /** @brief Total ring buffer capacity (about 500 ms of internal-rate audio). */
    static constexpr size_t RingBufferSize = InternalSampleRate * BytesPerFrame / 2; // 500ms

    using ReadCallback = std::function<uint32_t(float* out, uint32_t frames, uint32_t channels)>;

    AudioStreamBuffer(std::unique_ptr<AudioDecoder> decoder);
    AudioStreamBuffer(const AudioStreamBuffer&) = delete;
    AudioStreamBuffer& operator=(const AudioStreamBuffer&) = delete;

    ~AudioStreamBuffer() override;

    uint32_t getSampleRate() const noexcept override;

    uint32_t getChannelCount() const noexcept override;

    uint64_t getLengthFrames() const noexcept override;

    uint64_t tell() const noexcept override;

    void seek(uint64_t frame) override;

    void reset() override;

    uint32_t readFrames(float* outBuffer, uint32_t frames, uint32_t channels) override;

    bool isEnd() const noexcept override;

    Type getType() const noexcept override { return Type::Stream; }

    bool needsMoreData();

    void decodeTask();

private:
    std::unique_ptr<AudioDecoder> m_decoder;    //!< Decoder that provides source audio data
    AudioConverter m_converter;                 //!< Converter from decoder output format to internal format
    ma_rb m_ringBuffer;                         //!< Ring buffer shared between playback and decode operations
    size_t m_lowWatermark = RingBufferSize / 4; //!< Refill threshold (about 125 ms)
    std::atomic<bool> m_needsMoreData{false};   //!< Flag indicating that buffer refill is required
    std::atomic<bool> m_isDecoding{false};      //!< Flag indicating decode work is currently running
};

} // namespace audio
} // namespace ocf
