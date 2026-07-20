#pragma once
#include "EnvelopeUnit.h"
#include "SweepUnit.h"
#include <stdint.h>

struct PulseChannel {
    PulseChannel();

    void clock();

    void clockFrameCounterQuarterFrame();
    void clockFrameCounterHalfFrame();

    uint8_t getOutput();

    void setRegister(uint8_t registerNumber, uint8_t data);

    bool getLengthCounterOutput() const;
    void setLengthCounterEnabled(bool enabled);

private:
    uint8_t m_lengthCounter = 0;

    uint16_t m_timer = 0;
    uint16_t m_timerCounter = 0;
    uint8_t m_dutyCycle = 0;
    uint8_t m_sequenceStep = 0;
    EnvelopeUnit m_envelopeUnit;
    SweepUnit m_sweepUnit;
};
