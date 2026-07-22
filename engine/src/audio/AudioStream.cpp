#include "AudioStream.h"

#include "audio/AudioDecoder.h"
#include "audio/AudioMixer.h"
#include "audio/AudioUtility.h"

#include "ocf/core/job/JobSystem.h"

#include <cstring>

namespace ocf::audio {

AudioStream::AudioStream(AudioDecoder* decoder)
    : m_decoder(decoder)
    , m_converter()
    , m_ringBuffer()
{
    AudioSpec inputSpec = {.format = decoder->getFormat(),
                           .sampleRate = decoder->getSampleRate(),
                           .channelCount = decoder->getChannelCount()};
    AudioSpec outputSpec = {.format = InternalFormat,
                            .sampleRate = InternalSampleRate,
                            .channelCount = InternalChannels};
    m_converter.initialize(inputSpec, outputSpec);

    ma_rb_init(RingBufferSize, nullptr, nullptr, &m_ringBuffer);
}

AudioStream::~AudioStream()
{
    delete m_decoder;
    ma_rb_uninit(&m_ringBuffer);
}

void AudioStream::render(float* output, uint32_t frameCount, uint32_t channels)
{
    const size_t bytesPerFrame = AudioUtility::getFormatSize(InternalFormat) * channels;
    const size_t bytesToRead = frameCount * bytesPerFrame;

    std::memset(output, 0, bytesToRead);

    if (m_state != AudioSource::State::Playing) {
        return;
    }

    void* bufferOut = nullptr;
    size_t bytesAvailable = bytesToRead;

    ma_result result = ma_rb_acquire_read(&m_ringBuffer, &bytesAvailable, &bufferOut);
    if (result != MA_SUCCESS) {
        return;
    }

    std::memcpy(output, bufferOut, bytesAvailable);
    ma_rb_commit_read(&m_ringBuffer, bytesAvailable);

    if (needsMoreData()) {
        m_needsMoreData.store(true, std::memory_order_release);
    }
}

void AudioStream::update()
{
    if (m_state != AudioSource::State::Playing) {
        return;
    }

    auto& jobSystem = job::JobSystem::getInstance();

    if (m_needsMoreData.load(std::memory_order_acquire)) {
        bool expected = false;
        if (m_isDecoding.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
            auto job = jobSystem.createJob([this](void*) {
                decodeTask();
            });
            jobSystem.run(job);

            m_needsMoreData.store(false, std::memory_order_release);
        }
    }
}

bool AudioStream::needsMoreData()
{
    size_t bytesAvailable = ma_rb_available_read(&m_ringBuffer);
    return bytesAvailable < m_lowWatermark;
}

void AudioStream::decodeTask()
{
    if (m_decoder == nullptr) {
        return;
    }

    size_t availableWrite = ma_rb_available_write(&m_ringBuffer);
    if (availableWrite == 0) {
        return;
    }

    void* outputBuffer = nullptr;
    size_t outputBytes = availableWrite;
    if (ma_rb_acquire_write(&m_ringBuffer, &outputBytes, &outputBuffer) != MA_SUCCESS) {
        return;
    }

    constexpr size_t outputBytesPerFrame =
        AudioUtility::getFormatSize(InternalFormat) * InternalChannels;

    size_t outputFrameCount = outputBytes / outputBytesPerFrame;
    size_t inputFrameCount = m_converter.calculateInputFrameCount(outputFrameCount);

    void* inputBuffer =
        std::malloc(m_decoder->framesToBytes(static_cast<uint32_t>(inputFrameCount)));
    if (inputBuffer == nullptr) {
        ma_rb_commit_write(&m_ringBuffer, 0);
        return;
    }

    m_decoder->readFixedFrames(inputBuffer, static_cast<uint32_t>(inputFrameCount));

    size_t writtenFrameCount =
        m_converter.process(inputBuffer, inputFrameCount, outputBuffer, outputFrameCount);

    ma_rb_commit_write(&m_ringBuffer, writtenFrameCount * outputBytesPerFrame);

    std::free(inputBuffer);

    m_isDecoding.store(false, std::memory_order_release);
}

} // namespace ocf::audio