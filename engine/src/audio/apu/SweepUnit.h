// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>

namespace ocf::audio {

/**
 * Sweep unit of a pulse channel. It holds the channel's timer period, and when enabled, moves
 * it to the target period every (divider period + 1) half frames.
 * https://www.nesdev.org/wiki/APU_Sweep
 */
struct SweepUnit {
    // Pulse 1 negates with one's complement, pulse 2 with two's complement
    explicit SweepUnit(bool onesComplement = false);

    // Clocked by the half frames of the frame sequencer
    void clock();

    // $4001 / $4005
    void setRegister(uint8_t data);

    void setPeriod(uint16_t period) { m_period = period; }

    uint16_t getPeriod() const { return m_period; }

    uint16_t getTargetPeriod() const;

    // The channel is muted while the period is below 8 or the target period is above $7FF,
    // even when the sweep is disabled
    bool isMuted() const;

private:
    bool m_onesComplement = false;
    bool m_enabled = false;
    bool m_negateFlag = false;
    bool m_reloadFlag = false;
    uint8_t m_divider = 0;
    uint8_t m_dividerPeriod = 0;
    uint8_t m_shiftCount = 0;
    uint16_t m_period = 0;
};

} // namespace ocf::audio
