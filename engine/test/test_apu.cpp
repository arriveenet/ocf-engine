// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>

#include "ocf/audio/apu/Apu.h"
#include "ocf/audio/apu/DmcChannel.h"
#include "ocf/audio/apu/LengthCounter.h"
#include "ocf/audio/apu/PulseChannel.h"
#include "ocf/audio/apu/SweepUnit.h"
#include "ocf/audio/apu/TriangleChannel.h"

#include <ocf/audio/tracker/DpcmEncoder.h>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

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
    pulse.setLengthCounterEnabled(true);
    pulse.setRegister(0, 0xBF);  // 50% duty, length counter halted, constant volume 15
    pulse.setRegister(1, sweep); // By default disabled, negated so that low notes are not muted
    pulse.setRegister(2, period & 0xFF);
    pulse.setRegister(3, (period >> 8) & 0x07);
    return pulse;
}

TriangleChannel createTriangle(uint16_t period)
{
    TriangleChannel triangle;
    triangle.setLengthCounterEnabled(true);
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

TEST(ApuLengthCounterTest, LoadsFromTheTable)
{
    LengthCounter counter;
    counter.setEnabled(true);
    counter.load(0x01); // 254 half frames

    for (int i = 0; i < 253; i++) {
        counter.clock();
    }
    EXPECT_TRUE(counter.isActive());
    counter.clock();
    EXPECT_FALSE(counter.isActive());
}

TEST(ApuLengthCounterTest, HaltStopsCounting)
{
    LengthCounter counter;
    counter.setEnabled(true);
    counter.load(0x03); // 2 half frames
    counter.setHalt(true);

    counter.clock();
    counter.clock();
    EXPECT_TRUE(counter.isActive());
}

TEST(ApuLengthCounterTest, IgnoresLoadsWhileDisabled)
{
    // Disabled at power-up
    LengthCounter counter;
    counter.load(0x01);
    EXPECT_FALSE(counter.isActive());

    counter.setEnabled(true);
    counter.load(0x01);
    EXPECT_TRUE(counter.isActive());

    // Disabling clears the counter and blocks further loads
    counter.setEnabled(false);
    EXPECT_FALSE(counter.isActive());
    counter.load(0x01);
    EXPECT_FALSE(counter.isActive());
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

namespace {

constexpr uint16_t FRAME_COUNTER = 0x4017;
constexpr uint8_t FRAME_INTERRUPT = 0x40;

void clockApu(Apu& apu, int cycles)
{
    for (int cycle = 0; cycle < cycles; cycle++) {
        apu.clock();
    }
}

} // namespace

TEST(ApuFrameCounterTest, RaisesTheInterruptAtTheEndOf4StepSequence)
{
    Apu apu;
    clockApu(apu, 29827);
    EXPECT_EQ(apu.readStatusRegister() & FRAME_INTERRUPT, 0);

    clockApu(apu, 1);
    EXPECT_EQ(apu.readStatusRegister() & FRAME_INTERRUPT, FRAME_INTERRUPT);

    // Reading the status clears it
    EXPECT_EQ(apu.readStatusRegister() & FRAME_INTERRUPT, 0);
}

TEST(ApuFrameCounterTest, InhibitFlagClearsAndBlocksTheInterrupt)
{
    Apu apu;
    clockApu(apu, 29830);
    apu.writeRegister(FRAME_COUNTER, 0x40);
    EXPECT_EQ(apu.readStatusRegister() & FRAME_INTERRUPT, 0);

    clockApu(apu, 29830);
    EXPECT_EQ(apu.readStatusRegister() & FRAME_INTERRUPT, 0);
}

TEST(ApuFrameCounterTest, NoInterruptIn5StepMode)
{
    Apu apu;
    apu.writeRegister(FRAME_COUNTER, 0x80);
    clockApu(apu, 37282 * 2);
    EXPECT_EQ(apu.readStatusRegister() & FRAME_INTERRUPT, 0);
}

TEST(ApuFrameCounterTest, Writing5StepModeClocksAHalfFrame)
{
    Apu apu;
    apu.writeStatusRegister(0x01);
    apu.writeRegister(0x4000, 0x00);
    apu.writeRegister(0x4003, 0x18); // Length 2

    apu.writeRegister(FRAME_COUNTER, 0x80);
    EXPECT_EQ(apu.readStatusRegister() & 0x01, 0x01);
    apu.writeRegister(FRAME_COUNTER, 0x80);
    EXPECT_EQ(apu.readStatusRegister() & 0x01, 0x00);
}

TEST(ApuFrameCounterTest, WritingRestartsTheSequence)
{
    Apu apu;
    apu.writeStatusRegister(0x01);
    apu.writeRegister(0x4000, 0x00);
    apu.writeRegister(0x4003, 0x18); // Length 2

    // Half frames at 14913 and 29829 cycles after the write. Without the restart, the second
    // one would come 29829 cycles after power-up
    clockApu(apu, 10000);
    apu.writeRegister(FRAME_COUNTER, 0x00);
    clockApu(apu, 29828);
    EXPECT_EQ(apu.readStatusRegister() & 0x01, 0x01);
    clockApu(apu, 1);
    EXPECT_EQ(apu.readStatusRegister() & 0x01, 0x00);
}

namespace {

constexpr uint16_t DMC_BASE = 0x4010;
constexpr uint8_t DMC_ACTIVE = 0x10;
constexpr uint8_t DMC_INTERRUPT = 0x80;
constexpr uint8_t DMC_IRQ_ENABLE = 0x80;
constexpr uint8_t DMC_LOOP = 0x40;
constexpr int RATE15_BYTE_CYCLES = 8 * 54;

/** Starts a sample at $C000 of `length` bytes ($4013 = (length - 1) / 16). */
void startDmc(DmcChannel& dmc, uint8_t flags, uint8_t level, uint16_t length)
{
    dmc.setRegister(0, flags);
    dmc.setRegister(1, level);
    dmc.setRegister(2, 0x00);
    dmc.setRegister(3, static_cast<uint8_t>((length - 1) / 16));
    dmc.setEnabled(true);
}

/** The output level each time it changes. */
std::vector<uint8_t> recordLevelChanges(DmcChannel& dmc, int cycles)
{
    std::vector<uint8_t> levels;
    uint8_t previous = dmc.getOutput();
    for (int cycle = 0; cycle < cycles; cycle++) {
        dmc.clock();
        if (dmc.getOutput() != previous) {
            previous = dmc.getOutput();
            levels.push_back(previous);
        }
    }
    return levels;
}

} // namespace

TEST(ApuDmcTest, RatePeriodMatchesHardware)
{
    for (uint8_t rate : {0, 9, 15}) {
        DmcChannel dmc;
        const std::vector<uint8_t> ones(17, 0xFF);
        dmc.setMemory(ones);
        startDmc(dmc, rate, 0, 17);

        // CPU cycles between two steps of the output
        int previousChange = -1;
        int interval = -1;
        uint8_t previous = dmc.getOutput();
        for (int cycle = 0; cycle < 20 * 428 && interval < 0; cycle++) {
            dmc.clock();
            if (dmc.getOutput() != previous) {
                if (previousChange >= 0) {
                    interval = cycle - previousChange;
                }
                previousChange = cycle;
                previous = dmc.getOutput();
            }
        }
        EXPECT_EQ(interval, ocf::audio::tracker::DPCM_RATE_PERIODS[rate]) << "rate " << +rate;
    }
}

TEST(ApuDmcTest, PlaysBitsFromTheLowestBit)
{
    DmcChannel dmc;
    const std::vector<uint8_t> data = {0x0F}; // Up 4 times, then down 4 times
    dmc.setMemory(data);
    startDmc(dmc, 15, 64, 1);
    EXPECT_EQ(recordLevelChanges(dmc, 20 * 54),
              (std::vector<uint8_t>{66, 68, 70, 72, 70, 68, 66, 64}));
}

TEST(ApuDmcTest, LevelStaysWithinTheLimits)
{
    DmcChannel dmc;
    const std::vector<uint8_t> ones(17, 0xFF);
    dmc.setMemory(ones);
    startDmc(dmc, 15, 120, 17);
    EXPECT_EQ(recordLevelChanges(dmc, 20 * RATE15_BYTE_CYCLES), (std::vector<uint8_t>{122, 124, 126}));

    // An odd level goes up to 127, and down to 1
    DmcChannel odd;
    odd.setMemory(ones);
    startDmc(odd, 15, 121, 17);
    recordLevelChanges(odd, 20 * RATE15_BYTE_CYCLES);
    EXPECT_EQ(odd.getOutput(), 127);

    const std::vector<uint8_t> zeros(17, 0x00);
    DmcChannel down;
    down.setMemory(zeros);
    startDmc(down, 15, 5, 17);
    recordLevelChanges(down, 20 * RATE15_BYTE_CYCLES);
    EXPECT_EQ(down.getOutput(), 1);
}

TEST(ApuDmcTest, DirectLoad)
{
    DmcChannel dmc;
    dmc.setRegister(1, 0xFF);
    EXPECT_EQ(dmc.getOutput(), 127);
    dmc.setRegister(1, 33);
    EXPECT_EQ(dmc.getOutput(), 33);

    // Without a sample, the level holds
    for (int cycle = 0; cycle < 10000; cycle++) {
        dmc.clock();
    }
    EXPECT_EQ(dmc.getOutput(), 33);
}

TEST(ApuDmcTest, PlaysWhatTheEncoderEncoded)
{
    // The levels of the APU must be the levels that the encoder aimed at
    using namespace ocf::audio::tracker;
    std::vector<float> sine(4410);
    for (size_t i = 0; i < sine.size(); i++) {
        sine[i] = static_cast<float>(0.8 * std::sin(2.0 * std::numbers::pi * 300.0 * i / 44100.0));
    }
    for (uint8_t rate : {0, 15}) {
        DpcmEncodeOptions options;
        options.rate = rate;
        auto result = DpcmEncoder::encode(sine, 44100, options);
        ASSERT_TRUE(result.isOk());
        const std::vector<uint8_t>& data = result.unwrap().data;

        DmcChannel dmc;
        dmc.setMemory(data);
        startDmc(dmc, rate, options.restLevel, static_cast<uint16_t>(data.size()));
        const int cycles = static_cast<int>((data.size() * 8 + 16) * DPCM_RATE_PERIODS[rate]);
        const std::vector<uint8_t> played = recordLevelChanges(dmc, cycles);

        // The decoded levels, as changes from the rest level
        std::vector<uint8_t> expected = decodeDpcm(data, options.restLevel);
        expected.insert(expected.begin(), options.restLevel);
        expected.erase(std::unique(expected.begin(), expected.end()), expected.end());
        expected.erase(expected.begin());
        EXPECT_EQ(played, expected) << "rate " << +rate;
        EXPECT_FALSE(dmc.isActive());
    }
}

TEST(ApuDmcTest, AddressWrapsTo8000)
{
    // $FFC0-$FFFF are ones, then $8000 reads 0
    std::vector<uint8_t> memory(ocf::audio::tracker::DPCM_MEMORY_SIZE, 0);
    std::fill(memory.end() - 64, memory.end(), 0xFF);
    DmcChannel dmc;
    dmc.setMemory(memory);
    dmc.setRegister(0, 15);
    dmc.setRegister(1, 0);
    dmc.setRegister(2, 0xFF); // $FFC0
    dmc.setRegister(3, 0x04); // 65 bytes
    dmc.setEnabled(true);

    const std::vector<uint8_t> levels = recordLevelChanges(dmc, 80 * RATE15_BYTE_CYCLES);
    ASSERT_FALSE(levels.empty());
    EXPECT_EQ(*std::max_element(levels.begin(), levels.end()), 126);
    EXPECT_EQ(levels.back(), 126 - 8 * 2); // One byte of zeros from $8000
}

TEST(ApuDmcTest, StatusAndInterrupt)
{
    Apu apu;
    const std::vector<uint8_t> data(17, 0x55);
    apu.setDmcMemory(data);
    apu.writeRegister(DMC_BASE + 0, DMC_IRQ_ENABLE | 15);
    apu.writeRegister(DMC_BASE + 3, 0x01); // 17 bytes
    EXPECT_EQ(apu.readStatusRegister() & DMC_ACTIVE, 0);

    apu.writeStatusRegister(DMC_ACTIVE);
    EXPECT_EQ(apu.readStatusRegister() & (DMC_ACTIVE | DMC_INTERRUPT), DMC_ACTIVE);

    // The last byte is read when the one before it starts playing: after the silent first output
    // cycle and 15 bytes
    clockApu(apu, 15 * RATE15_BYTE_CYCLES);
    EXPECT_EQ(apu.readStatusRegister() & DMC_ACTIVE, DMC_ACTIVE);
    clockApu(apu, 2 * RATE15_BYTE_CYCLES);
    EXPECT_EQ(apu.readStatusRegister() & (DMC_ACTIVE | DMC_INTERRUPT), DMC_INTERRUPT);

    // Reading does not clear the interrupt. Writing $4015 does
    EXPECT_EQ(apu.readStatusRegister() & DMC_INTERRUPT, DMC_INTERRUPT);
    apu.writeStatusRegister(0x00);
    EXPECT_EQ(apu.readStatusRegister() & DMC_INTERRUPT, 0);

    // So does disabling the interrupt in $4010
    apu.writeStatusRegister(DMC_ACTIVE);
    clockApu(apu, 20 * RATE15_BYTE_CYCLES);
    EXPECT_EQ(apu.readStatusRegister() & DMC_INTERRUPT, DMC_INTERRUPT);
    apu.writeRegister(DMC_BASE + 0, 15);
    EXPECT_EQ(apu.readStatusRegister() & DMC_INTERRUPT, 0);
}

TEST(ApuDmcTest, LoopKeepsPlaying)
{
    Apu apu;
    const std::vector<uint8_t> data(17, 0x55);
    apu.setDmcMemory(data);
    apu.writeRegister(DMC_BASE + 0, DMC_IRQ_ENABLE | DMC_LOOP | 15);
    apu.writeRegister(DMC_BASE + 3, 0x01);
    apu.writeStatusRegister(DMC_ACTIVE);
    clockApu(apu, 100 * RATE15_BYTE_CYCLES);
    EXPECT_EQ(apu.readStatusRegister() & (DMC_ACTIVE | DMC_INTERRUPT), DMC_ACTIVE);
}

TEST(ApuDmcTest, DisablingStopsAfterTheBufferedBits)
{
    DmcChannel dmc;
    const std::vector<uint8_t> ones(17, 0xFF);
    dmc.setMemory(ones);
    startDmc(dmc, 15, 0, 17);
    recordLevelChanges(dmc, 12 * 54); // Into the first byte

    dmc.setEnabled(false);
    EXPECT_FALSE(dmc.isActive());
    // At most the rest of this byte and the buffered byte
    const std::vector<uint8_t> levels = recordLevelChanges(dmc, 100 * RATE15_BYTE_CYCLES);
    EXPECT_LE(levels.size(), 16u);
    EXPECT_LT(dmc.getOutput(), 126);

    // Enabling again restarts the sample
    dmc.setEnabled(true);
    EXPECT_TRUE(dmc.isActive());
}

TEST(ApuDmcTest, MixesIntoTheOutput)
{
    Apu apu;
    const float silent = apu.getOutput();
    apu.writeRegister(DMC_BASE + 1, 64);
    EXPECT_GT(apu.getOutput(), silent);
}
