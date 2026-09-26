#include "audio/AudioStreamBuffer.h"

#include "audio/AudioUtility.h"

#include "ocf/core/Logger.h"

#include <cstddef>
#include <string.h>
#include <utility>
#include <vector>

namespace ocf {
namespace audio {

AudioStreamBuffer::AudioStreamBuffer(std::unique_ptr<AudioDecoder> decoder)
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

        OCF_LOG_DEBUG("[Audio] format: {}, Sample rate: {}, Channels: {}, Total frames: {}, "
                  "RingBuffer: {}ms",
                  AudioUtility::getAudioFormatString(m_decoder->getFormat()),
                  m_decoder->getSampleRate(), m_decoder->getChannelCount(),
                  m_decoder->getTotalFrames(),
                  (RingBufferSize * 1000) / (InternalSampleRate * BytesPerFrame));
}

AudioStreamBuffer::~AudioStreamBuffer()
{
    ma_rb_uninit(&m_ringBuffer);
}

uint32_t AudioStreamBuffer::getSampleRate() const noexcept
{
    return m_decoder->getSampleRate();
}

uint32_t AudioStreamBuffer::getChannelCount() const noexcept
{
    return m_decoder->getChannelCount();
}

uint64_t AudioStreamBuffer::getLengthFrames() const noexcept
{
    return m_decoder->getTotalFrames();
}

uint64_t AudioStreamBuffer::tell() const noexcept
{
    return m_decoder->tell();
}

void AudioStreamBuffer::seek(uint64_t frame)
{
    m_decoder->seek(frame);
}

void AudioStreamBuffer::reset()
{
    seek(0);
    ma_rb_reset(&m_ringBuffer);
}

uint32_t AudioStreamBuffer::readFrames(float* outBuffer, uint32_t frames, uint32_t channels)
{
    const size_t bytesPerFrame = AudioUtility::getFormatSize(InternalFormat) * channels;
    const size_t bytesToRead = frames * bytesPerFrame;

    std::memset(outBuffer, 0, bytesToRead);

    void* bufferOut = nullptr;
    size_t bytesAvailable = bytesToRead;
    size_t totalBytesRead = 0;

    ma_result result = ma_rb_acquire_read(&m_ringBuffer, &bytesAvailable, &bufferOut);
    if (result != MA_SUCCESS) {
        return 0;
    }

    if (bytesAvailable > 0) {
        std::memcpy(outBuffer, bufferOut, bytesAvailable);
        ma_rb_commit_read(&m_ringBuffer, bytesAvailable);
        totalBytesRead += bytesAvailable;
    }

    if (totalBytesRead < bytesToRead) {
        size_t remainingBytes = bytesToRead - totalBytesRead;
        bufferOut = nullptr;
        bytesAvailable = remainingBytes;

        result = ma_rb_acquire_read(&m_ringBuffer, &bytesAvailable, &bufferOut);
        if (result == MA_SUCCESS && bytesAvailable > 0) {
            std::memcpy(reinterpret_cast<uint8_t*>(outBuffer) + totalBytesRead, bufferOut,
                        bytesAvailable);
            ma_rb_commit_read(&m_ringBuffer, bytesAvailable);
            totalBytesRead += bytesAvailable;
        }
    }

    return static_cast<uint32_t>(totalBytesRead / bytesPerFrame);
}

bool AudioStreamBuffer::isEnd() const noexcept
{
    return m_decoder->tell() >= m_decoder->getTotalFrames();
}

bool AudioStreamBuffer::needsMoreData()
{
    size_t bytesAvailable = ma_rb_available_read(&m_ringBuffer);
    return bytesAvailable < m_lowWatermark;
}

void AudioStreamBuffer::decodeTask()
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

    static thread_local std::vector<std::byte> s_inputBuffer;
    s_inputBuffer.resize(m_decoder->framesToBytes(static_cast<uint32_t>(inputFrameCount)));

    void* inputBuffer = s_inputBuffer.data();
    if (inputBuffer == nullptr) {
        ma_rb_commit_write(&m_ringBuffer, 0);
        return;
    }

    size_t readFrameCount = m_decoder->readFixedFrames(inputBuffer, static_cast<uint32_t>(inputFrameCount));
    if (readFrameCount == 0) {
        ma_rb_commit_write(&m_ringBuffer, 0);
        return;
    }

    size_t writtenFrameCount =
        m_converter.process(inputBuffer, readFrameCount, outputBuffer, outputFrameCount);

    ma_rb_commit_write(&m_ringBuffer, writtenFrameCount * outputBytesPerFrame);

    if (!needsMoreData()) {
        m_needsMoreData.store(false, std::memory_order_release);
    }
}

} // namespace audio
} // namespace ocf
