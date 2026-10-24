// SPDX-License-Identifier: MIT
#pragma once
#include "ocf/audio/tracker/Dpcm.h"

#include <array>
#include <span>
#include <stdint.h>

namespace ocf::audio {

/**
 * Delta modulation channel: plays 1-bit delta samples from memory at $C000-$FFFF.
 * https://www.nesdev.org/wiki/APU_DMC
 *
 * There is no CPU, so the samples are read from a copy of that memory (setMemory()), and reading
 * does not stall anything. Addresses $8000-$BFFF, reached by wrapping around after $FFFF, read 0.
 */
struct DmcChannel {
    static constexpr uint16_t MEMORY_START = 0xC000;

    DmcChannel();

    // Every CPU cycle
    void clock();

    uint8_t getOutput() const { return m_outputLevel; }

    // $4010-$4013
    void setRegister(uint8_t registerNumber, uint8_t data);

    /** Copies the data to $C000. The rest of the memory is zero. */
    void setMemory(std::span<const uint8_t> data);

    /** Copies the data to $C000 + offset, as far as it fits. */
    void writeMemory(size_t offset, std::span<const uint8_t> data);

    // $4015. Writing also clears the interrupt
    void setEnabled(bool enabled);
    bool isActive() const { return m_bytesRemaining > 0; }
    bool getInterrupt() const { return m_interrupt; }

private:
    void clockOutputUnit();
    void fillSampleBuffer();
    void restartSample();
    uint8_t readMemory(uint16_t address) const;

    std::array<uint8_t, tracker::DPCM_MEMORY_SIZE> m_memory{};

    // $4010-$4013
    bool m_interruptEnabled = false;
    bool m_loop = false;
    uint16_t m_timer = tracker::DPCM_RATE_PERIODS[0];
    uint16_t m_sampleAddress = MEMORY_START;
    uint16_t m_sampleLength = 1;

    // Memory reader
    uint16_t m_currentAddress = MEMORY_START;
    uint16_t m_bytesRemaining = 0;
    uint8_t m_sampleBuffer = 0;
    bool m_sampleBufferEmpty = true;

    // Output unit
    uint16_t m_timerCounter = 0;
    uint8_t m_shiftRegister = 0;
    uint8_t m_bitsRemaining = 8;
    bool m_silence = true;
    uint8_t m_outputLevel = 0;

    bool m_interrupt = false;
};

} // namespace ocf::audio
