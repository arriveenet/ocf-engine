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

class AudioStream : public AudioSource {
public:
    static constexpr size_t BytesPerFrame = sizeof(float) * InternalChannels;
    static constexpr size_t RingBufferSize = InternalSampleRate * BytesPerFrame / 10; // 100ms

    explicit AudioStream(std::unique_ptr<AudioDecoder> decoder);
    ~AudioStream() override;

    void render(float* output, uint32_t frameCount, uint32_t channels) override;

    void update() override;

    void play() override;

    void stop() override;

    bool needsMoreData();

    void decodeTask();

private:
    std::unique_ptr<AudioDecoder> m_decoder;
    AudioConverter m_converter;
    ma_rb m_ringBuffer;
    size_t m_lowWatermark = RingBufferSize / 2; // 50ms
    std::atomic<bool> m_needsMoreData{false};
    std::atomic<bool> m_isDecoding{false};
};

} // namespace audio
} // namespace ocf
