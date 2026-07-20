#include "TriangleChannel.h"
#include <iostream>

static constexpr uint8_t LENGTH_COUNTER_TABLE[] = {
    0x0A, 0xFE, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06, 0xA0, 0x08, 0x3C, 0x0A, 0x0E, 0x0C, 0x1A, 0x0E,
    0x0C, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16, 0xC0, 0x18, 0x48, 0x1A, 0x10, 0x1C, 0x20, 0x1E,
};

static constexpr uint8_t TRIANGLE_TABLE[32] = {15, 14, 13, 12, 11, 10, 9,  8,  7,  6, 5,
                                               4,  3,  2,  1,  0,  0,  1,  2,  3,  4, 5,
                                               6,  7,  8,  9,  10, 11, 12, 13, 14, 15};

TriangleChannel::TriangleChannel()
{
}

void TriangleChannel::clock()
{
    if (m_timerCounter == 0) {
        m_timerCounter = m_frequency + 1;
        m_sequenceStep = (m_sequenceStep + 1) & 31; // 0–31
    }
    else {
        m_timerCounter--;
    }

}

void TriangleChannel::clockFrameCounterQuarterFrame()
{
    if (m_linearReloadFlag) {
        m_linearCounter = m_linearReloadValue;
    }
    else if (m_linearCounter > 0) {
        m_linearCounter--;
    }
    if (!m_linearControlFlag) {
        m_linearReloadFlag = false;
    }
}

void TriangleChannel::clockFrameCounterHalfFrame()
{
    if (m_lengthCounter > 0 && !m_linearControlFlag) {
        m_lengthCounter--;
    }
}

uint8_t TriangleChannel::getOutput()
{
    if (m_lengthCounter == 0 || m_linearCounter == 0)
        return 0;

    return TRIANGLE_TABLE[m_sequenceStep];
}

void TriangleChannel::setRegister(uint8_t registerNumber, uint8_t data)
{
    // Linear counter and length load
    if (registerNumber == 0) {
        m_linearReloadValue = data & 0x7F;
        m_linearControlFlag = (data & 0x80) != 0;
    }
    // Unused
    else if (registerNumber == 1) {
    }
    // Timer low
    else if (registerNumber == 2) {
        m_frequency = (m_frequency & 0x700) | data;

        // double freq = 1789773.0 / (32.0 * (m_frequency + 1));
        // std::cout << "Triangle Channel Frequency set to: " << freq << " Hz" << std::endl;
    }
    // Timer high and length counter load
    else if (registerNumber == 3) {
        m_frequency = (m_frequency & 0x00FF) | ((data & 0x07) << 8);
        // double freq = 1789773.0 / (32.0 * (m_frequency + 1));
        // std::cout << "Triangle Channel Frequency set to: " << freq << " Hz" << std::endl;
       
        m_lengthCounter = LENGTH_COUNTER_TABLE[(data & 0xF8) >> 3];

        m_linearReloadFlag = true;
    }
}

bool TriangleChannel::getLengthCounterOutput() const
{
    return m_lengthCounter > 0;
}

void TriangleChannel::setLengthCounterEnabled(bool enabled)
{
    if (!enabled) {
        m_lengthCounter = 0;
    }
}
