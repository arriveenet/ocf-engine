// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>

namespace ocf::audio {

class Apu;

/**
 * Frame counter. It clocks the envelopes and the linear counter every quarter frame, the length
 * counters and the sweep units every half frame, and raises the frame interrupt in 4-step mode.
 * https://www.nesdev.org/wiki/APU_Frame_Counter
 */
struct FrameSequencer {

    explicit FrameSequencer(Apu& apu);

    void clock();

    // $4017
    void setRegister(uint8_t data);

    bool getInterrupt() const { return m_interrupt; }
    void clearInterrupt() { m_interrupt = false; }

private:
    static constexpr uint16_t SEQUENCE_4STEP_CLOCKS[] = {29830, 7457, 14913, 22371, 29828, 29829};

    static constexpr uint16_t SEQUENCE_5STEP_CLOCKS[] = {37282, 7457, 14913, 22371, 37281};

    void clock4StepSequence();
    void clock5StepSequence();

    void clockQuarterFrame();
    void clockHalfFrame();

    void raiseInterrupt();

    Apu& m_apu;
    uint16_t m_counter = 0;
    bool m_interrupt = false;
    bool m_disableInterrupt = false;
    bool m_sequence5StepMode = false;
};

} // namespace ocf::audio
