#include "FrameSequencer.h"
#include "Apu.h"
#include "TriangleChannel.h"


FrameSequencer::FrameSequencer(Apu& apu)
    : m_apu(apu)
{
}

void FrameSequencer::clock()
{
    ++m_counter;

    if (m_sequence5StepMode) {
        clock5StepSequence();
    }
    else {
        clock4StepSequence();
    }
}

void FrameSequencer::setDisableInterrupt(bool disable)
{
}

void FrameSequencer::setSequence5StepMode(bool sequence5stepMode)
{
    m_sequence5StepMode = sequence5stepMode;
}

void FrameSequencer::clock4StepSequence()
{
    if (m_counter == SEQUENCE_4STEP_CLOCKS[0]) {
        m_counter = 0;
    }
    // Step1
    else if (m_counter == SEQUENCE_4STEP_CLOCKS[1]) {
        clockQuarterFrame();
    }
    // Step2
    else if (m_counter == SEQUENCE_4STEP_CLOCKS[2]) {
        clockQuarterFrame();
        clockHalfFrame();
    }
    // Step3
    else if (m_counter == SEQUENCE_4STEP_CLOCKS[3]) {
        clockQuarterFrame();
    }
    // Step4 (first cycle for interupt)
    else if (m_counter == SEQUENCE_4STEP_CLOCKS[4]) {
    }
    // Step4
    else if (m_counter == SEQUENCE_4STEP_CLOCKS[5]) {
        clockQuarterFrame();
        clockHalfFrame();
    }
}

void FrameSequencer::clock5StepSequence()
{
    // Step1
    if (m_counter == SEQUENCE_5STEP_CLOCKS[0]) {
        m_counter = 0;
    }
    // Step2
    else if (m_counter == SEQUENCE_5STEP_CLOCKS[1]) {
        clockQuarterFrame();
    }
    // Step3
    else if (m_counter == SEQUENCE_5STEP_CLOCKS[2]) {
        clockQuarterFrame();
        clockHalfFrame();
    }
    // Step4
    else if (m_counter == SEQUENCE_5STEP_CLOCKS[3]) {
        clockQuarterFrame();
    }
    // Step5
    else if (m_counter == SEQUENCE_5STEP_CLOCKS[4]) {
        clockQuarterFrame();
        clockHalfFrame();
    }
}

void FrameSequencer::clockQuarterFrame()
{
    m_apu.clockFrameCounterQuarterFrame();
}

void FrameSequencer::clockHalfFrame()
{
    m_apu.clockFrameCounterHalfFrame();
}
