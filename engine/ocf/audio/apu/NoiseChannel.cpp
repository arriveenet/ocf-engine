// SPDX-License-Identifier: MIT
#include "ocf/audio/apu/NoiseChannel.h"

namespace ocf::audio {

static constexpr uint16_t NOISE_PERIOD_TABLE[16] = {4,   8,   16,  32,  64,  96,   128,  160,
                                                    202, 254, 380, 508, 762, 1016, 2034, 4068};

NoiseChannel::NoiseChannel()
{
}

void NoiseChannel::clock()
{
    if (m_timerCounter == 0) {
        m_timerCounter = m_timer - 1;

        const uint16_t tap = (m_mode == Mode::ShortPeriod) ? 6 : 1;
        const uint16_t feedback = (m_shiftRegister & 1u) ^ ((m_shiftRegister >> tap) & 1u);

        m_shiftRegister >>= 1;
        m_shiftRegister |= feedback << 14;
    }
    else {
        m_timerCounter--;
    }
}

void NoiseChannel::clockFrameCounterQuarterFrame()
{
    m_envelopeUnit.clock();
}

void NoiseChannel::clockFrameCounterHalfFrame()
{
    m_lengthCounter.clock();
}

uint8_t NoiseChannel::getOutput()
{
    if (!m_lengthCounter.isActive() || (m_shiftRegister & 1)) {
        return 0;
    }

    return m_envelopeUnit.getOutput();
}

void NoiseChannel::setRegister(uint8_t registerNumber, uint8_t data)
{
    if (registerNumber == 0) {
        // Envelope loop flag, which also halts the length counter
        m_envelopeUnit.setLoop((data & 0x20) != 0);
        m_lengthCounter.setHalt((data & 0x20) != 0);

        // Constant volume flag
        m_envelopeUnit.setConstantVolumeFlag((data & 0x10) != 0);

        // Volume
        m_envelopeUnit.setDividerPeriod(data & 0x0F);
    }
    else if (registerNumber == 1) {
        // Unused
    }
    else if (registerNumber == 2) {
        // Noise period
        m_timer = NOISE_PERIOD_TABLE[data & 0x0F];
        
        // Mode
        m_mode = (data & 0x80) ? Mode::ShortPeriod : Mode::LongPeriod;
    }
    else if (registerNumber == 3) {
        // Length counter load
        m_lengthCounter.load((data & 0xF8) >> 3);

        m_envelopeUnit.start();
    }
}

bool NoiseChannel::getLengthCounterOutput() const
{
    return m_lengthCounter.isActive();
}

void NoiseChannel::setLengthCounterEnabled(bool enabled)
{
    m_lengthCounter.setEnabled(enabled);
}

} // namespace ocf::audio
