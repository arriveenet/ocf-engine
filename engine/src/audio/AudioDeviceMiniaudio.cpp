// SPDX-License-Identifier: MIT
#include "audio/AudioDeviceMiniaudio.h"

#include "audio/AudioMixer.h"
#include "audio/AudioUtility.h"

namespace ocf {
namespace audio {

AudioDeviceMiniaudio::AudioDeviceMiniaudio()
    : m_device{}
{
}

AudioDeviceMiniaudio::~AudioDeviceMiniaudio()
{
}

bool AudioDeviceMiniaudio::initialize(AudioMixer* mixer)
{
    m_mixer = mixer;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);

    config.playback.format = AudioUtility::getMiniaudioFormat(InternalFormat);
    config.playback.channels = InternalChannels;
    config.sampleRate = InternalSampleRate;
    config.dataCallback = &AudioDeviceMiniaudio::dataCallback;
    config.pUserData = this;

    ma_result result = ma_device_init(nullptr, &config, &m_device);
    return result == MA_SUCCESS;
}

void AudioDeviceMiniaudio::shutdown()
{
    ma_device_uninit(&m_device);
}

bool AudioDeviceMiniaudio::start()
{
    ma_result result = ma_device_start(&m_device);
    return result == MA_SUCCESS;
}

void AudioDeviceMiniaudio::stop()
{
    ma_device_stop(&m_device);
}

void AudioDeviceMiniaudio::dataCallback(ma_device* device, void* output, const void* input,
                                    ma_uint32 frameCount)
{
    AudioDeviceMiniaudio* self = static_cast<AudioDeviceMiniaudio*>(device->pUserData);
    float* out = static_cast<float*>(output);

    if (self->m_mixer != nullptr) {
        self->m_mixer->render(out, frameCount, device->playback.channels);
    }
    else {
        ma_silence_pcm_frames(out, frameCount, AudioUtility::getMiniaudioFormat(InternalFormat),
                              device->playback.channels);
    }
}

} // namespace audio
} // namespace ocf
