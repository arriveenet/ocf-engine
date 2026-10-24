// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>

namespace ocf::audio {

/**
 * Length counter shared by the pulse, triangle and noise channels. It silences the channel when
 * it reaches zero, unless halted.
 * https://www.nesdev.org/wiki/APU_Length_Counter
 */
struct LengthCounter {
    // Clocked by the half frames of the frame sequencer
    void clock();

    // Loads the counter from the 5-bit index written to bits 3-7 of the channel's last register.
    // Ignored while the channel is disabled
    void load(uint8_t index);

    // The halt flag shares its bit with the envelope loop flag (pulse, noise) or the linear
    // counter control flag (triangle)
    void setHalt(bool halt) { m_halt = halt; }

    // $4015. Disabling the channel clears the counter and keeps it at zero until enabled again.
    // All channels are disabled at power-up
    void setEnabled(bool enabled);

    bool isActive() const { return m_counter > 0; }

private:
    bool m_enabled = false;
    bool m_halt = false;
    uint8_t m_counter = 0;
};

} // namespace ocf::audio
