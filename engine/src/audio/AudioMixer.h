#pragma once

#include "audio/apu/Apu.h"

namespace ocf {
namespace audio {

static constexpr float SAMPLE_RATE = 44100.0f;
static constexpr double CPU_FREQUENCY = 1789773.0;

class AudioMixer {
public:
    void render(float* output, unsigned int frameCount, unsigned int channels);

    void writeRegister(uint16_t address, uint8_t value) { m_apu.writeRegister(address, value); }

    void writeStatusRegister(uint8_t value) { m_apu.writeStatusRegister(value); }

private:
    Apu m_apu;
    double m_cpuRemainder = 0.0;
};

} // namespace audio
} // namespace ocf
