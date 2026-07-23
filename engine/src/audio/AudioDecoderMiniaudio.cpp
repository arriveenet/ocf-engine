#include "audio/AudioDecoderMiniaudio.h"

#include "audio/AudioUtility.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

namespace ocf::audio {

AudioDecoderMiniaudio::AudioDecoderMiniaudio()
    : m_decoder{}
{
}

AudioDecoderMiniaudio::~AudioDecoderMiniaudio()
{
    close();
}

bool AudioDecoderMiniaudio::open(std::string_view filename)
{
    ma_decoder_config config = ma_decoder_config_init_default();
    if (ma_decoder_init_file(filename.data(), &config, &m_decoder) != MA_SUCCESS) {
        return false;
    }

    m_format = AudioUtility::getAudioFormat(m_decoder.outputFormat);
    m_sampleRate = m_decoder.outputSampleRate;
    m_channelCount = m_decoder.outputChannels;
    m_bytesPerBlock = ma_get_bytes_per_frame(m_decoder.outputFormat, m_decoder.outputChannels);
    
    ma_uint64 totalFrames = 0;
    ma_decoder_get_length_in_pcm_frames(&m_decoder, &totalFrames);
    m_totalFrames = static_cast<uint32_t>(totalFrames);

    m_isOpened = true;

    return true;
}

void AudioDecoderMiniaudio::close()
{
    if (m_isOpened) {
        ma_decoder_uninit(&m_decoder);
        m_isOpened = false;
    }
}

uint32_t AudioDecoderMiniaudio::read(void* buffer, uint32_t frameCount)
{
    ma_uint64 totalFrameRead = 0;

    while (totalFrameRead < frameCount) {
        ma_uint64 framesReadThisTime = 0;

        uint8_t* pWritePointer =
            static_cast<uint8_t*>(buffer) +
            (totalFrameRead * ma_get_bytes_per_frame(m_decoder.outputFormat, m_decoder.outputChannels));

        ma_result result = ma_decoder_read_pcm_frames(
            &m_decoder, pWritePointer, frameCount - totalFrameRead, &framesReadThisTime);

        totalFrameRead += framesReadThisTime;

        if (framesReadThisTime == 0 || result == MA_AT_END) {
            break;
        }
    }

    return static_cast<uint32_t>(totalFrameRead);
}

bool AudioDecoderMiniaudio::seek(uint32_t frameOffset)
{
    return ma_decoder_seek_to_pcm_frame(&m_decoder, frameOffset) == MA_SUCCESS;
}

} // namespace ocf::audio
