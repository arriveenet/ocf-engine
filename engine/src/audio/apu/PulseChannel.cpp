// SPDX-License-Identifier: MIT
#include "PulseChannel.h"
#include <iostream>

static constexpr uint8_t DUTY_CYCLE_TABLE[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0}, // 12.5%
    {0, 1, 1, 0, 0, 0, 0, 0}, // 25%
    {0, 1, 1, 1, 1, 0, 0, 0}, // 50%
    {1, 0, 0, 1, 1, 1, 1, 1}  // 25% negated
};

namespace ocf::audio {

PulseChannel::PulseChannel(bool onesComplementSweep)
    : m_sweepUnit(onesComplementSweep)
{
}

void PulseChannel::clock()
{
    // The timer runs at half the CPU clock: a period of t steps every 2 * (t + 1) CPU cycles
    if (m_timerCounter == 0) {
        m_timerCounter = m_sweepUnit.getPeriod() * 2 + 1;
        m_sequenceStep = (m_sequenceStep + 1) & 0x07; // 0–7
    }
    else {
        m_timerCounter--;
    }
}

void PulseChannel::clockFrameCounterQuarterFrame()
{
    m_envelopeUnit.clock();
}

void PulseChannel::clockFrameCounterHalfFrame()
{
    m_lengthCounter.clock();
    m_sweepUnit.clock();
}

uint8_t PulseChannel::getOutput()
{
    if (!m_lengthCounter.isActive() || m_sweepUnit.isMuted())
        return 0;

    return DUTY_CYCLE_TABLE[m_dutyCycle][m_sequenceStep] * m_envelopeUnit.getOutput();
}

void PulseChannel::setRegister(uint8_t registerNumber, uint8_t data)
{
    // Duty cycle and envelope
    if (registerNumber == 0) {
        // Duty cycle
        m_dutyCycle = (data & 0xC0) >> 6;

        // Envelope loop flag, which also halts the length counter
        m_envelopeUnit.setLoop((data & 0x20) != 0);
        m_lengthCounter.setHalt((data & 0x20) != 0);

        // Constant volume flag
        m_envelopeUnit.setConstantVolumeFlag((data & 0x10) != 0);

        // Volume
        m_envelopeUnit.setDividerPeriod(data & 0x0F);
    }
    // Sweep unit
    else if (registerNumber == 1) {
        m_sweepUnit.setRegister(data);
    }
    // Timer low. The period is held by the sweep unit, which also changes it
    else if (registerNumber == 2) {
        m_sweepUnit.setPeriod((m_sweepUnit.getPeriod() & 0x700) | data);
    }
    // Timer high and length counter load
    else if (registerNumber == 3) {
        m_sweepUnit.setPeriod((m_sweepUnit.getPeriod() & 0xFF) | ((data & 0x07) << 8));

        // Restart the sequencer and the envelope. The timer is not reset
        m_sequenceStep = 0;
        m_envelopeUnit.start();

        // Length counter load
        m_lengthCounter.load((data & 0xF8) >> 3);
    }
}

bool PulseChannel::getLengthCounterOutput() const
{
    return m_lengthCounter.isActive();
}

void PulseChannel::setLengthCounterEnabled(bool enabled)
{
    m_lengthCounter.setEnabled(enabled);
}

} // namespace ocf::audio
