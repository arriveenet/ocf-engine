// SPDX-License-Identifier: MIT
#include "audio/tracker/SongSynthesizer.h"

#include "audio/AudioNesApu.h"
#include "audio/apu/Apu.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace ocf::audio::tracker {

namespace {

constexpr uint16_t STATUS_REGISTER = 0x4015;

constexpr double CYCLES_PER_TICK = AudioNesApu::CPU_FREQUENCY / SongSynthesizer::TICK_RATE;

// The APU output is always positive. A first order high-pass filter at this frequency removes
// the DC offset, which would otherwise click when the song starts and stops
constexpr double HIGH_PASS_FREQUENCY = 38.3;

} // namespace

SongSynthesizer::SongSynthesizer(uint32_t sampleRate)
    : m_apu(std::make_unique<Apu>())
    , m_player([this](uint16_t address, uint8_t data) { writeRegister(address, data); })
    , m_sampleRate(sampleRate)
    , m_cyclesPerSample(AudioNesApu::CPU_FREQUENCY / static_cast<double>(sampleRate))
    , m_highPassCoefficient(static_cast<float>(
          std::exp(-2.0 * std::numbers::pi * HIGH_PASS_FREQUENCY / static_cast<double>(sampleRate))))
{
    // The triangle rests at a non-zero level. Starting the filter there avoids a click
    m_previousInput = m_apu->getOutput();
}

SongSynthesizer::~SongSynthesizer()
{
}

std::shared_ptr<const Song> SongSynthesizer::setSong(std::shared_ptr<const Song> song)
{
    std::shared_ptr<const Song> previous = m_player.setSong(std::move(song));
    loadDpcmSamples();
    return previous;
}

float SongSynthesizer::renderSample()
{
    m_cpuRemainder += m_cyclesPerSample;
    while (m_cpuRemainder >= 1.0) {
        m_tickRemainder -= 1.0;
        if (m_tickRemainder <= 0.0) {
            m_player.tick();
            m_tickRemainder += CYCLES_PER_TICK;
        }
        m_apu->clock();
        m_cpuRemainder -= 1.0;
    }

    const float input = m_apu->getOutput();
    const float filtered = input - m_previousInput + m_highPassCoefficient * m_previousOutput;
    m_previousInput = input;
    m_previousOutput = filtered;
    return std::clamp(filtered, -1.0f, 1.0f);
}

void SongSynthesizer::loadDpcmSamples()
{
    // A sample that is playing keeps playing from the new memory, which is the same unless the
    // samples were changed
    m_apu->setDmcMemory({});
    const Song* song = m_player.getSong();
    if (song == nullptr) {
        return;
    }
    for (size_t i = 0; i < song->dpcmSamples.size(); i++) {
        if (const std::optional<size_t> offset = song->getDpcmSampleOffset(i)) {
            m_apu->writeDmcMemory(*offset, song->dpcmSamples[i].data);
        }
    }
}

void SongSynthesizer::writeRegister(uint16_t address, uint8_t data)
{
    if (address == STATUS_REGISTER) {
        m_apu->writeStatusRegister(data);
    }
    else {
        m_apu->writeRegister(address, data);
    }
}

} // namespace ocf::audio::tracker
