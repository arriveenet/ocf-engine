// SPDX-License-Identifier: MIT
#include "ocf/audio/apu/FrameSequencer.h"
#include "ocf/audio/apu/Apu.h"
#include "ocf/audio/apu/TriangleChannel.h"

namespace ocf::audio {

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

void FrameSequencer::setRegister(uint8_t data)
{
    m_sequence5StepMode = (data & 0x80) != 0;

    // Setting the interrupt inhibit flag also clears the interrupt
    m_disableInterrupt = (data & 0x40) != 0;
    if (m_disableInterrupt) {
        m_interrupt = false;
    }

    // Restart the sequence. On hardware this happens 3 or 4 CPU cycles after the write
    m_counter = 0;

    // In 5-step mode, the write immediately clocks a quarter and a half frame
    if (m_sequence5StepMode) {
        clockQuarterFrame();
        clockHalfFrame();
    }
}

void FrameSequencer::clock4StepSequence()
{
    if (m_counter == SEQUENCE_4STEP_CLOCKS[0]) {
        raiseInterrupt();
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
    // Step4. The interrupt is raised on three consecutive cycles
    else if (m_counter == SEQUENCE_4STEP_CLOCKS[4]) {
        raiseInterrupt();
    }
    else if (m_counter == SEQUENCE_4STEP_CLOCKS[5]) {
        raiseInterrupt();
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

void FrameSequencer::raiseInterrupt()
{
    if (!m_disableInterrupt) {
        m_interrupt = true;
    }
}

} // namespace ocf::audio
