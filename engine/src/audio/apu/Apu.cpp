// SPDX-License-Identifier: MIT
#include "Apu.h"

namespace ocf::audio {

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
    m_noiseChannel.clock();
    m_dmcChannel.clock();
}

void Apu::writeRegister(uint16_t address, uint8_t data)
{
    // Pulse registers
    if (address < 0x4008) {
        const uint8_t channel = static_cast<uint8_t>((address - 0x4000) / 4);
        m_pulseChannel[channel].setRegister(address & 0x3, data);
    }
    // Triangle register
    else if (address < 0x400C) {
        m_triangleChannel.setRegister(address & 0x3, data);
    }
    // Noise register
    else if (address < 0x4010) {
        m_noiseChannel.setRegister(address & 0x3, data);
    }
    // DMC register
    else if (address < 0x4014) {
        m_dmcChannel.setRegister(address & 0x3, data);
    }
    // Frame counter
    else if (address == 0x4017) {
        m_frameSequencer.setRegister(data);
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
    if (m_noiseChannel.getLengthCounterOutput()) {
        status |= 0x08;
    }
    if (m_dmcChannel.isActive()) {
        status |= 0x10;
    }

    // Reading the status clears the frame interrupt
    if (m_frameSequencer.getInterrupt()) {
        status |= 0x40;
    }
    m_frameSequencer.clearInterrupt();

    // Unlike the frame interrupt, reading does not clear the DMC interrupt
    if (m_dmcChannel.getInterrupt()) {
        status |= 0x80;
    }

    return status;
}

void Apu::writeStatusRegister(uint8_t data)
{
    m_pulseChannel[0].setLengthCounterEnabled(data & 0x01);
    m_pulseChannel[1].setLengthCounterEnabled(data & 0x02);
    m_triangleChannel.setLengthCounterEnabled(data & 0x04);
    m_noiseChannel.setLengthCounterEnabled(data & 0x08);
    m_dmcChannel.setEnabled(data & 0x10);
}

void Apu::setDmcMemory(std::span<const uint8_t> data)
{
    m_dmcChannel.setMemory(data);
}

void Apu::writeDmcMemory(size_t offset, std::span<const uint8_t> data)
{
    m_dmcChannel.writeMemory(offset, data);
}

void Apu::clockFrameCounterQuarterFrame()
{
    m_pulseChannel[0].clockFrameCounterQuarterFrame();
    m_pulseChannel[1].clockFrameCounterQuarterFrame();
    m_triangleChannel.clockFrameCounterQuarterFrame();
    m_noiseChannel.clockFrameCounterQuarterFrame();
}

void Apu::clockFrameCounterHalfFrame()
{
    m_pulseChannel[0].clockFrameCounterHalfFrame();
    m_pulseChannel[1].clockFrameCounterHalfFrame();
    m_triangleChannel.clockFrameCounterHalfFrame();
    m_noiseChannel.clockFrameCounterHalfFrame();
}

float Apu::getOutput()
{
    /*
     * Mixer formula from NES APU documentation:
     * https://www.nesdev.org/wiki/APU_Mixer
     */
    float pulseOut = 0.0f;
    uint8_t pulseSum = m_pulseChannel[0].getOutput() + m_pulseChannel[1].getOutput();
    if (pulseSum != 0) {
        pulseOut = 95.88f / ((8128.0f / pulseSum) + 100.0f);
    }
 
    float tndOut = 0.0f;
    uint8_t triangle = m_triangleChannel.getOutput();
    uint8_t noise = m_noiseChannel.getOutput();
    uint8_t dmc = m_dmcChannel.getOutput();
    if (triangle + noise + dmc > 0) {
        tndOut = 159.79f /
                 (1.0f / ((triangle / 8227.0f) + (noise / 12241.0f) + (dmc / 22638.0f)) + 100.0f);
    }

    return pulseOut + tndOut;
}

} // namespace ocf::audio
