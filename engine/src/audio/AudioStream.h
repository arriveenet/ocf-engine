// SPDX-License-Identifier: MIT
#pragma once

#include "audio/AudioConverter.h"
#include "audio/AudioMixer.h"

#include "ocf/audio/AudioSource.h"

#include <miniaudio.h>

#include <atomic>
#include <memory>

namespace ocf {
namespace audio {

class AudioDecoder;

/**
 * @brief Streamed audio source that manages decoded PCM data in a ring buffer
 *        and serves frames on demand.
 *
 * Implements the `AudioSource` interface and provides audio to the mixer
 * through `render()`. Decoding is performed by `decodeTask()`, while
 * `update()` and `needsMoreData()` control refill timing.
 */
class AudioStream : public AudioSource {
public:
    /** @brief Number of bytes in one frame (based on internal channel count). */
    static constexpr size_t BytesPerFrame = sizeof(float) * InternalChannels;

    /** @brief Total ring buffer capacity (about 500 ms of internal-rate audio). */
    static constexpr size_t RingBufferSize = InternalSampleRate * BytesPerFrame / 2; // 500ms

    /**
     * @brief Constructs an audio stream.
     * @param decoder Owning pointer to the decoder used as the PCM source.
     */
    explicit AudioStream(std::unique_ptr<AudioDecoder> decoder);

    /** @brief Releases stream resources. */
    ~AudioStream() override;

    /**
     * @brief Writes audio frames to the output buffer.
     * @param output Destination buffer.
     * @param frameCount Number of frames requested.
     * @param channels Output channel count.
     */
    void render(float* output, uint32_t frameCount, uint32_t channels) override;

    /**
     * @brief Updates stream state.
     *
     * Typically used to evaluate low-watermark status and trigger decode work.
     */
    void update() override;

    /** @brief Starts or resumes playback. */
    void play() override;

    /** @brief Stops playback. */
    void stop() override;

    /**
     * @brief Returns whether the ring buffer needs refilling.
     * @return `true` if more decoded data is needed.
     */
    bool needsMoreData();

    /**
     * @brief Executes decoding and pushes data into the ring buffer.
     *
     * Concurrent execution is expected to be guarded by `m_isDecoding`.
     */
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
