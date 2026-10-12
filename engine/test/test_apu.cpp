// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>

#include "audio/apu/PulseChannel.h"
#include "audio/apu/SweepUnit.h"
#include "audio/apu/TriangleChannel.h"

using namespace ocf::audio;

namespace {

constexpr uint16_t A4_PULSE_PERIOD = 253;
constexpr uint16_t A4_TRIANGLE_PERIOD = 126;
constexpr int MAX_CYCLES = 200000;

/** CPU cycles between two consecutive changes of the output from `low` to above it. */
template <typename Channel>
int measureCycles(Channel& channel, uint8_t low)
{
    uint8_t previous = channel.getOutput();
    int first = -1;
    for (int cycle = 0; cycle < MAX_CYCLES; cycle++) {
        channel.clock();
        const uint8_t output = channel.getOutput();
        if (previous == low && output > low) {
            if (first >= 0) {
                return cycle - first;
            }
            first = cycle;
        }
        previous = output;
    }
    return -1;
}

template <typename Channel>
bool isSilent(Channel& channel, int cycles)
{
    for (int cycle = 0; cycle < cycles; cycle++) {
        channel.clock();
        if (channel.getOutput() != 0) {
            return false;
        }
    }
    return true;
}

PulseChannel createPulse(uint16_t period, uint8_t sweep = 0x08)
{
    PulseChannel pulse;
    pulse.setRegister(0, 0xBF);  // 50% duty, length counter halted, constant volume 15
    pulse.setRegister(1, sweep); // By default disabled, negated so that low notes are not muted
    pulse.setRegister(2, period & 0xFF);
    pulse.setRegister(3, (period >> 8) & 0x07);
    return pulse;
}

TriangleChannel createTriangle(uint16_t period)
{
    TriangleChannel triangle;
    triangle.setRegister(0, 0xFF); // Linear counter halted at its maximum
    triangle.setRegister(2, period & 0xFF);
    triangle.setRegister(3, (period >> 8) & 0x07);
    triangle.clockFrameCounterQuarterFrame(); // Loads the linear counter
    return triangle;
}

} // namespace

TEST(ApuPulseTest, PeriodMatchesHardware)
{
    // 16 * (t + 1) CPU cycles per period: 440.4 Hz for A-4
    PulseChannel pulse = createPulse(A4_PULSE_PERIOD);
    EXPECT_EQ(measureCycles(pulse, 0), 16 * (A4_PULSE_PERIOD + 1));
}

TEST(ApuPulseTest, WritingTimerLowChangesThePeriod)
{
    PulseChannel pulse = createPulse(A4_PULSE_PERIOD);
    measureCycles(pulse, 0);

    // Only $4002: the period changes without writing $4003
    pulse.setRegister(2, 0x7F);
    EXPECT_EQ(measureCycles(pulse, 0), 16 * (0x7F + 1));
}

TEST(ApuPulseTest, WritingTimerHighKeepsTheLowBits)
{
    PulseChannel pulse = createPulse(0x07F);
    pulse.setRegister(3, 0x01);
    EXPECT_EQ(measureCycles(pulse, 0), 16 * (0x17F + 1));
}

TEST(ApuPulseTest, MutedBelowPeriod8)
{
    PulseChannel muted = createPulse(7);
    EXPECT_TRUE(isSilent(muted, 1000));

    PulseChannel audible = createPulse(8);
    EXPECT_FALSE(isSilent(audible, 1000));
}

TEST(ApuPulseTest, LowNotesAreMutedUnlessNegated)
{
    // With shift 0 and no negate, the target period (2 * $400) overflows even if disabled
    PulseChannel muted = createPulse(0x400, 0x00);
    EXPECT_TRUE(isSilent(muted, 100000));

    PulseChannel audible = createPulse(0x400, 0x08);
    EXPECT_EQ(measureCycles(audible, 0), 16 * (0x400 + 1));
}

TEST(ApuSweepTest, SweepsUpEveryHalfFrame)
{
    SweepUnit sweep;
    sweep.setPeriod(0x100);
    sweep.setRegister(0x81); // Enabled, divider period 0, shift 1

    sweep.clock();
    EXPECT_EQ(sweep.getPeriod(), 0x180);
    sweep.clock();
    EXPECT_EQ(sweep.getPeriod(), 0x240);
}

TEST(ApuSweepTest, DividerPeriod)
{
    SweepUnit sweep;
    sweep.setPeriod(0x100);
    sweep.setRegister(0xA1); // Enabled, divider period 2, shift 1

    // Changes on the first half frame, then every 3 half frames
    const uint16_t expected[] = {0x180, 0x180, 0x180, 0x240, 0x240, 0x240, 0x360};
    for (uint16_t period : expected) {
        sweep.clock();
        EXPECT_EQ(sweep.getPeriod(), period);
    }
}

TEST(ApuSweepTest, NegateDependsOnTheChannel)
{
    SweepUnit pulse1(true);
    pulse1.setPeriod(0x100);
    pulse1.setRegister(0x89); // Enabled, negate, shift 1
    EXPECT_EQ(pulse1.getTargetPeriod(), 0x100 - 0x80 - 1);

    SweepUnit pulse2(false);
    pulse2.setPeriod(0x100);
    pulse2.setRegister(0x89);
    EXPECT_EQ(pulse2.getTargetPeriod(), 0x100 - 0x80);
}

TEST(ApuSweepTest, KeepsThePeriodWhenDisabledOrMuted)
{
    SweepUnit disabled;
    disabled.setPeriod(0x100);
    disabled.setRegister(0x01); // Shift 1 but not enabled
    disabled.clock();
    EXPECT_EQ(disabled.getPeriod(), 0x100);

    SweepUnit muted;
    muted.setPeriod(0x600);
    muted.setRegister(0x81); // The target 0x900 overflows
    EXPECT_TRUE(muted.isMuted());
    muted.clock();
    EXPECT_EQ(muted.getPeriod(), 0x600);
}

TEST(ApuTriangleTest, PeriodMatchesHardware)
{
    // 32 * (t + 1) CPU cycles per period: 440.4 Hz for A-4
    TriangleChannel triangle = createTriangle(A4_TRIANGLE_PERIOD);
    EXPECT_EQ(measureCycles(triangle, 0), 32 * (A4_TRIANGLE_PERIOD + 1));
}

TEST(ApuTriangleTest, HoldsItsLevelWhenSilenced)
{
    TriangleChannel triangle = createTriangle(A4_TRIANGLE_PERIOD);
    for (int cycle = 0; cycle < 1000; cycle++) {
        triangle.clock();
    }

    // The linear counter reloads to zero, which stops the sequencer
    triangle.setRegister(0, 0x00);
    triangle.clockFrameCounterQuarterFrame();

    const uint8_t level = triangle.getOutput();
    for (int cycle = 0; cycle < 10000; cycle++) {
        triangle.clock();
        ASSERT_EQ(triangle.getOutput(), level);
    }
}
