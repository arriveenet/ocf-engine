#pragma once
#include <stdint.h>

struct TriangleChannel {
    TriangleChannel();

    void clock();

    void clockFrameCounterQuarterFrame();
    void clockFrameCounterHalfFrame();

    uint8_t getOutput();

    void setRegister(uint8_t registerNumber, uint8_t data);

    bool getLengthCounterOutput() const;
    void setLengthCounterEnabled(bool enabled);

private:
    uint16_t m_frequency = 0;
    uint8_t m_lengthCounter = 0;

    uint16_t m_timer = 0;
    uint16_t m_timerCounter = 0;
    uint8_t m_sequenceStep = 0;
    uint8_t m_linearCounter = 0;
    uint8_t m_linearReloadValue = 0;
    bool m_linearReloadFlag = false;
    bool m_linearControlFlag = false;
};