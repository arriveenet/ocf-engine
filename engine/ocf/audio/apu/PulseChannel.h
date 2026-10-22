// SPDX-License-Identifier: MIT
#pragma once
#include "ocf/audio/apu/EnvelopeUnit.h"
#include "ocf/audio/apu/LengthCounter.h"
#include "ocf/audio/apu/SweepUnit.h"

#include <stdint.h>

namespace ocf::audio {

struct PulseChannel {
    // Pulse 1 sweeps down with one's complement, pulse 2 with two's complement
    explicit PulseChannel(bool onesComplementSweep = false);

    void clock();

    void clockFrameCounterQuarterFrame();
    void clockFrameCounterHalfFrame();

    uint8_t getOutput();

    void setRegister(uint8_t registerNumber, uint8_t data);

    bool getLengthCounterOutput() const;
    void setLengthCounterEnabled(bool enabled);

private:
    LengthCounter m_lengthCounter;

    uint16_t m_timerCounter = 0;
    uint8_t m_dutyCycle = 0;
    uint8_t m_sequenceStep = 0;
    EnvelopeUnit m_envelopeUnit;
    SweepUnit m_sweepUnit;
};

} // namespace ocf::audio
