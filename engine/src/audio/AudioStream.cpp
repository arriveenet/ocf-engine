// SPDX-License-Identifier: MIT
#include "AudioStream.h"

#include "audio/AudioDecoder.h"
#include "audio/AudioMixer.h"
#include "audio/AudioUtility.h"

#include "ocf/core/job/JobSystem.h"
#include "ocf/core/Logger.h"

#include <cstring>

namespace ocf::audio {

AudioStream::AudioStream(std::unique_ptr<AudioDecoder> decoder)
    : m_decoder(std::move(decoder))
    , m_converter()
    , m_ringBuffer()
{
    AudioSpec inputSpec = {.format = m_decoder->getFormat(),
                           .sampleRate = m_decoder->getSampleRate(),
                           .channelCount = m_decoder->getChannelCount()};
    AudioSpec outputSpec = {.format = InternalFormat,
                            .sampleRate = InternalSampleRate,
                            .channelCount = InternalChannels};
    m_converter.initialize(inputSpec, outputSpec);

    ma_rb_init(RingBufferSize, nullptr, nullptr, &m_ringBuffer);

    OCF_LOG_DEBUG("[Audio] format: {}, Sample rate: {}, Channels: {}, Total frames: {}",
                  AudioUtility::getAudioFormatString(m_decoder->getFormat()),
                  m_decoder->getSampleRate(), m_decoder->getChannelCount(),
                  m_decoder->getTotalFrames());
}

AudioStream::~AudioStream()
{
    ma_rb_uninit(&m_ringBuffer);
}

void AudioStream::render(float* output, uint32_t frameCount, uint32_t channels)
{
    const size_t bytesPerFrame = AudioUtility::getFormatSize(InternalFormat) * channels;
    const size_t bytesToRead = frameCount * bytesPerFrame;

    std::memset(output, 0, bytesToRead);

    if (!isPlaying()) {
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
    if (!isPlaying()) {
        return;
    }

    auto& jobSystem = job::JobSystem::getInstance();

    if (m_needsMoreData.load(std::memory_order_acquire)) {
        bool expected = false;
        if (m_isDecoding.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
            // TODO: Consider using a thread pool or a dedicated audio decoding thread for better
            // performance.
            auto job = jobSystem.createJob([this](void*) {
                decodeTask();
                m_isDecoding.store(false, std::memory_order_release);
            });
            jobSystem.run(job);
        }
    }
}

void AudioStream::play()
{
    if (getState() != AudioSource::State::Paused) {
        m_decoder->seek(0);
        ma_rb_reset(&m_ringBuffer);
    }

    AudioSource::play();
}

void AudioStream::stop()
{
    m_decoder->seek(0);
    ma_rb_reset(&m_ringBuffer);
     
    AudioSource::stop();
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

    uint32_t framesRead = m_decoder->readFixedFrames(inputBuffer, static_cast<uint32_t>(inputFrameCount));

    if (framesRead == 0 ) {
        if (isLooping()) {
            m_decoder->seek(0);
            framesRead = m_decoder->readFixedFrames(inputBuffer, static_cast<uint32_t>(inputFrameCount));
        }
        else {
            stop();
        }
    }

    size_t writtenFrameCount =
        m_converter.process(inputBuffer, inputFrameCount, outputBuffer, outputFrameCount);

    ma_rb_commit_write(&m_ringBuffer, writtenFrameCount * outputBytesPerFrame);

    std::free(inputBuffer);

    if (needsMoreData()) {
        m_needsMoreData.store(false, std::memory_order_release);
    }
}

} // namespace ocf::audio
