#include "Apu.h"

Apu::Apu()
    : m_frameSequencer(*this)
{
}

void Apu::clock()
{
    // Clock Frame Sequencer
    m_frameSequencer.clock();

    // Clock Channels
    m_pulseChannel[0].clock();
    m_pulseChannel[1].clock();
    m_triangleChannel.clock();
}

void Apu::writeRegister(uint16_t address, uint8_t data)
{
    // Pulse registers
    if (address < 0x4008) {
        const uint8_t channel = (address - 0x4000) / 4;
        m_pulseChannel[channel].setRegister(address & 0x3, data);
    }
    // Triangle register
    else if (address < 0x400C) {
        m_triangleChannel.setRegister(address & 0x3, data);
    }
}

uint8_t Apu::readStatusRegister()
{
    uint8_t status = 0;
    if (m_pulseChannel[0].getLengthCounterOutput()) {
        status |= 0x01;
    }
    if (m_pulseChannel[1].getLengthCounterOutput()) {
        status |= 0x02;
    }
    if (m_triangleChannel.getLengthCounterOutput()) {
        status |= 0x04;
    }
    return status;
}

void Apu::writeStatusRegister(uint8_t data)
{
    m_pulseChannel[0].setLengthCounterEnabled(data & 0x01);
    m_pulseChannel[1].setLengthCounterEnabled(data & 0x02);
    m_triangleChannel.setLengthCounterEnabled(data & 0x04);
}

void Apu::clockFrameCounterQuarterFrame()
{
    m_pulseChannel[0].clockFrameCounterQuarterFrame();
    m_pulseChannel[1].clockFrameCounterQuarterFrame();
    m_triangleChannel.clockFrameCounterQuarterFrame();
}

void Apu::clockFrameCounterHalfFrame()
{
    m_pulseChannel[0].clockFrameCounterHalfFrame();
    m_pulseChannel[1].clockFrameCounterHalfFrame();
    m_triangleChannel.clockFrameCounterHalfFrame();
}

float Apu::getOutput()
{
    /*
     * Mixer formula from NES APU documentation:
     * https: // www.nesdev.org/wiki/APU_Mixer
     */
    float pulseOut = 0.0f;
    uint8_t pulseSum = m_pulseChannel[0].getOutput() + m_pulseChannel[1].getOutput();
    if (pulseSum != 0) {
        pulseOut = 95.88f / ((8128.0f / pulseSum) + 100.0f);
    }
 
    float tndOut = 0.0f;
    uint8_t triangle = m_triangleChannel.getOutput();
    uint8_t noise = 0;
    uint8_t dmc = 0;
    if (triangle + noise + dmc > 0) {
        tndOut = 159.79f /
                 (1.0f / ((triangle / 8227.0f) + (noise / 12241.0f) + (dmc / 22638.0f)) + 100.0f);
    }

    return pulseOut + tndOut;
}
