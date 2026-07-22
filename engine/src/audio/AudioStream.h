#pragma once

#include "audio/AudioConverter.h"
#include "audio/AudioSource.h"
#include "audio/AudioMixer.h"

#include <miniaudio.h>

#include <atomic>

namespace ocf {
namespace audio {

class AudioDecoder;

class AudioStream : public AudioSource {
public:
    static constexpr size_t BytesPerFrame = sizeof(float) * InternalChannels;
    static constexpr size_t RingBufferSize = InternalSampleRate * BytesPerFrame / 10; // 100ms

    explicit AudioStream(AudioDecoder* decoder);
    ~AudioStream() override;

    void render(float* output, uint32_t frameCount, uint32_t channels) override;

    void update() override;

    bool needsMoreData();

    void decodeTask();

private:
    AudioDecoder* m_decoder = nullptr;
    AudioConverter m_converter;
    ma_rb m_ringBuffer;
    size_t m_lowWatermark = RingBufferSize / 2; // 50ms
    std::atomic<bool> m_needsMoreData{false};
    std::atomic<bool> m_isDecoding{false};
};

} // namespace audio
} // namespace ocf
