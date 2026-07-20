#pragma once
#include <stdint.h>

struct Apu;

struct FrameSequencer {

    explicit FrameSequencer(Apu& apu);

    void clock();

    bool getInterrupt() const { return m_interrup; }

    void setDisableInterrupt(bool disable);
    void setSequence5StepMode(bool sequence5stepMode);

private:
    static constexpr uint16_t SEQUENCE_4STEP_CLOCKS[] = {29830, 7457, 14913, 22371, 29828, 29829};

    static constexpr uint16_t SEQUENCE_5STEP_CLOCKS[] = {37282, 7457, 14913, 22371, 37281};

    void clock4StepSequence();
    void clock5StepSequence();

    void clockQuarterFrame();
    void clockHalfFrame();

    Apu& m_apu;
    uint16_t m_counter = 0;
    bool m_interrup;
    bool m_disableInterrupt;
    bool m_sequence5StepMode;
};

