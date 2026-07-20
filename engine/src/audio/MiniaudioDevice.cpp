#include "MiniaudioDevice.h"
#include "AudioMixer.h"

namespace ocf {
namespace audio {

MiniaudioDevice::MiniaudioDevice()
    : m_device{}
{
}

MiniaudioDevice::~MiniaudioDevice()
{
}

bool MiniaudioDevice::initialize(AudioMixer* mixer)
{
    m_mixer = mixer;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);

    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = 44100;
    config.dataCallback = &MiniaudioDevice::dataCallback;
    config.pUserData = this;

    ma_result result = ma_device_init(nullptr, &config, &m_device);
    return result == MA_SUCCESS;
}

void MiniaudioDevice::shutdown()
{
    ma_device_uninit(&m_device);
}

bool MiniaudioDevice::start()
{
    ma_result result = ma_device_start(&m_device);
    return result == MA_SUCCESS;
}

void MiniaudioDevice::stop()
{
    ma_device_stop(&m_device);
}

void MiniaudioDevice::dataCallback(ma_device* device, void* output, const void* input,
                                    ma_uint32 frameCount)
{
    MiniaudioDevice* self = static_cast<MiniaudioDevice*>(device->pUserData);
    float* out = static_cast<float*>(output);

    if (self->m_mixer != nullptr) {
        self->m_mixer->render(out, frameCount, self->m_device.playback.channels);
    }
}

} // namespace audio
} // namespace ocf
