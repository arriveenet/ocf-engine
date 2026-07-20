#include "ocf/audio/AudioSystem.h"

#include "AudioDevice.h"
#include "AudioMixer.h"
#include "MiniaudioDevice.h"

namespace ocf {
namespace audio {

struct AudioSystem::Imple {
    std::unique_ptr<AudioDevice> m_audioDevice = nullptr;
    std::unique_ptr<AudioMixer> m_audioMixer = nullptr;
};


AudioSystem::AudioSystem()
{
    m_imple = std::make_unique<Imple>();

    m_imple->m_audioMixer = std::make_unique<AudioMixer>();
    m_imple->m_audioDevice = std::make_unique<MiniaudioDevice>();
}

AudioSystem::~AudioSystem()
{
    if (m_imple->m_audioDevice) {
        m_imple->m_audioDevice->shutdown();
        m_imple->m_audioDevice.reset();
    }

    if (m_imple->m_audioMixer) {
        m_imple->m_audioMixer.reset();
    }
}

bool AudioSystem::initialize()
{
    if (m_imple->m_audioDevice) {
        m_initialized = m_imple->m_audioDevice->initialize(m_imple->m_audioMixer.get());
    }

    if (m_initialized) {
        m_imple->m_audioDevice->start();
    }

    m_imple->m_audioMixer->writeStatusRegister(0x4);
    m_imple->m_audioMixer->writeRegister(0x4000, 0b10111111);
    m_imple->m_audioMixer->writeRegister(0x4001, 0x08);
    m_imple->m_audioMixer->writeRegister(0x4002, 0xfd);
    m_imple->m_audioMixer->writeRegister(0x4003, 0x00);
 



    return m_initialized;
}

void AudioSystem::shutdown()
{
    if (m_imple->m_audioDevice) {
        m_imple->m_audioDevice->stop();
        m_imple->m_audioDevice->shutdown();
    }
}

} // namespace audio
} // namespace ocf
