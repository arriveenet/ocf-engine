// SPDX-License-Identifier: MIT
#include "DmcChannel.h"

#include <algorithm>

namespace ocf::audio {

DmcChannel::DmcChannel()
{
}

void DmcChannel::clock()
{
    fillSampleBuffer();

    if (m_timerCounter == 0) {
        m_timerCounter = m_timer - 1;
        clockOutputUnit();
    }
    else {
        m_timerCounter--;
    }
}

void DmcChannel::clockOutputUnit()
{
    if (!m_silence) {
        m_outputLevel = tracker::stepDpcmLevel(m_outputLevel, (m_shiftRegister & 1) != 0);
    }
    m_shiftRegister >>= 1;

    // A new output cycle takes the next byte, or is silent when there is none
    m_bitsRemaining--;
    if (m_bitsRemaining == 0) {
        m_bitsRemaining = 8;
        if (m_sampleBufferEmpty) {
            m_silence = true;
        }
        else {
            m_silence = false;
            m_shiftRegister = m_sampleBuffer;
            m_sampleBufferEmpty = true;
            fillSampleBuffer();
        }
    }
}

void DmcChannel::fillSampleBuffer()
{
    if (!m_sampleBufferEmpty || m_bytesRemaining == 0) {
        return;
    }

    m_sampleBuffer = readMemory(m_currentAddress);
    m_sampleBufferEmpty = false;
    m_currentAddress = (m_currentAddress == 0xFFFF) ? 0x8000 : static_cast<uint16_t>(m_currentAddress + 1);

    m_bytesRemaining--;
    if (m_bytesRemaining == 0) {
        if (m_loop) {
            restartSample();
        }
        else if (m_interruptEnabled) {
            m_interrupt = true;
        }
    }
}

void DmcChannel::restartSample()
{
    m_currentAddress = m_sampleAddress;
    m_bytesRemaining = m_sampleLength;
}

uint8_t DmcChannel::readMemory(uint16_t address) const
{
    return (address >= MEMORY_START) ? m_memory[address - MEMORY_START] : 0;
}

void DmcChannel::setRegister(uint8_t registerNumber, uint8_t data)
{
    if (registerNumber == 0) {
        // IRQ enable, loop and rate
        m_interruptEnabled = (data & 0x80) != 0;
        if (!m_interruptEnabled) {
            m_interrupt = false;
        }
        m_loop = (data & 0x40) != 0;
        m_timer = tracker::DPCM_RATE_PERIODS[data & 0x0F];
    }
    else if (registerNumber == 1) {
        // Direct load of the output level
        m_outputLevel = data & 0x7F;
    }
    else if (registerNumber == 2) {
        // Sample address: $C000 + A * 64
        m_sampleAddress = static_cast<uint16_t>(MEMORY_START + data * 64);
    }
    else if (registerNumber == 3) {
        // Sample length: L * 16 + 1 bytes
        m_sampleLength = static_cast<uint16_t>(data * 16 + 1);
    }
}

void DmcChannel::setMemory(std::span<const uint8_t> data)
{
    m_memory.fill(0);
    writeMemory(0, data);
}

void DmcChannel::writeMemory(size_t offset, std::span<const uint8_t> data)
{
    if (offset >= m_memory.size()) {
        return;
    }
    const size_t size = std::min(data.size(), m_memory.size() - offset);
    std::copy_n(data.begin(), size, m_memory.begin() + offset);
}

void DmcChannel::setEnabled(bool enabled)
{
    m_interrupt = false;
    if (!enabled) {
        // The bits already read still play
        m_bytesRemaining = 0;
    }
    else if (m_bytesRemaining == 0) {
        restartSample();
    }
}

} // namespace ocf::audio
