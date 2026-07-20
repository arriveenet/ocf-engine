#pragma once
#include "AudioDevice.h"

#include <miniaudio.h>

namespace ocf {
namespace audio {

class MiniaudioDevice : public AudioDevice {
public:
    MiniaudioDevice();
    ~MiniaudioDevice() override;

    bool initialize(AudioMixer* mixer) override;

    void shutdown() override;

    bool start() override;

    void stop() override;

private:
    static void dataCallback(ma_device* device, void* output, const void* input,
                             ma_uint32 frameCount);

private:
    ma_device m_device;
    AudioMixer* m_mixer = nullptr;
};

} // namespace audio
} // namespace ocf
