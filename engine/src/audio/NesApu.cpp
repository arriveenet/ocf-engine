#include "ocf/audio/NesApu.h"

#include "audio/apu/Apu.h"
#include "audio/AudioMixer.h"

namespace ocf::audio {

NesApu::NesApu()
    : m_apu(std::make_unique<Apu>())
{
}

NesApu::~NesApu()
{
}

void NesApu::render(float* output, uint32_t frameCount, uint32_t channels)
{
    for (uint32_t i = 0; i < frameCount; i++) {
        m_cpuRemainder += CPU_FREQUENCY / static_cast<double>(InternalSampleRate);
        while (m_cpuRemainder >= 1.0f) {
            m_apu->clock();
            m_cpuRemainder -= 1.0f;
        }
        const float pmc = std::clamp(m_apu->getOutput(), -1.0f, 1.0f);

        for (uint32_t c = 0; c < channels; c++) {
            output[i * channels + c] = pmc;
        }
    }
}

void NesApu::writeRegister(uint16_t address, uint8_t data)
{
    m_apu->writeRegister(address, data);
}

uint8_t NesApu::readStatusRegister()
{
    return m_apu->readStatusRegister();
}

void NesApu::writeStatusRegister(uint8_t data)
{
    m_apu->writeStatusRegister(data);
}

} // namespace ocf::audio
