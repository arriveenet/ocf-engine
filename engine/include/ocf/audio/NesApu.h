#pragma once
#include "ocf/audio/AudioSource.h"

#include <memory>

namespace ocf::audio {

class Apu;

class NesApu : public AudioSource {
public:
    static constexpr double CPU_FREQUENCY = 1789773.0;

    NesApu();
    ~NesApu() override;

    void render(float* output, uint32_t frameCount, uint32_t channels) override;

    // Write registers
    void writeRegister(uint16_t address, uint8_t data);

    // Status register
    uint8_t readStatusRegister();
    void writeStatusRegister(uint8_t data);

private:
    std::unique_ptr<Apu> m_apu;
    double m_cpuRemainder = 0.0;
};

} // namespace ocf::audio
