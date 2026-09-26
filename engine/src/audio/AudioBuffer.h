// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>

namespace ocf {
namespace audio {

/** @brief Abstract base class for audio buffers. */
class AudioBuffer {
public:
    enum class Type {
        Clip,   //!< Audio data is fully loaded into memory
        Stream  //!< Audio data is streamed from disk or another source
    };

    virtual ~AudioBuffer() = default;

    /** @brief Get the sample rate of the audio buffer. */
    virtual uint32_t getSampleRate() const noexcept = 0;

    /** @brief Get the number of channels in the audio buffer. */
    virtual uint32_t getChannelCount() const noexcept = 0;

    /** @brief Get the total length of the audio buffer in frames. */
    virtual uint64_t getLengthFrames() const noexcept = 0;

    /** @brief Get the current read position in frames. */
    virtual uint64_t tell() const noexcept = 0;

    /** @brief Seek to the specified frame index. */
    virtual void seek(uint64_t frame) = 0;

    /** @brief Reset the read position to the start of the audio buffer. */
    virtual void reset() = 0;

    /**
     * @brief Read frames into outBuffer. outBuffer must be large enough for frames * channels floats.
     * @param outBuffer The buffer to write the samples into.
     * @param frames The number of frames to read.
     * @param channels The number of channels in the audio buffer.
     * @return Number of frames read.
     */
    virtual uint32_t readFrames(float* outBuffer, uint32_t frames, uint32_t channels) = 0;

    /** @brief Check if the end of the audio buffer has been reached. */
    virtual bool isEnd() const noexcept = 0;

    /**
     * @brief Get the type of the audio buffer.
     * @return The type of the audio buffer.
     */
    virtual Type getType() const noexcept = 0;
};

} // namespace audio
} // namespace ocf
