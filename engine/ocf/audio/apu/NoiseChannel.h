// SPDX-License-Identifier: MIT
#pragma once
#include "ocf/audio/apu/EnvelopeUnit.h"
#include "ocf/audio/apu/LengthCounter.h"

#include <stdint.h>

namespace ocf::audio {

struct NoiseChannel {
    enum class Mode : uint8_t {
        LongPeriod = 0,
        ShortPeriod = 1
    };

    NoiseChannel();

    void clock();

    void clockFrameCounterQuarterFrame();
    void clockFrameCounterHalfFrame();

    uint8_t getOutput();

    void setRegister(uint8_t registerNumber, uint8_t data);

    bool getLengthCounterOutput() const;
    void setLengthCounterEnabled(bool enabled);

private:
    LengthCounter m_lengthCounter;
    EnvelopeUnit m_envelopeUnit;
    Mode m_mode = Mode::LongPeriod;
    uint16_t m_timer = 4; // Period index 0 at power-up
    uint16_t m_timerCounter = 0;
    uint16_t m_shiftRegister = 1; // 15-bit LFSR, initialized to 1
};

} // namespace ocf::audio
