// SPDX-License-Identifier: MIT
#include "audio/tracker/NoteTable.h"

#include "audio/AudioNesApu.h"
#include "ocf/audio/tracker/TrackerTypes.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace ocf::audio::tracker {

namespace {

constexpr uint16_t MAX_PERIOD = 0x7FF;
constexpr int A4_SEMITONE = 4 * 12 + 9;

using PeriodTable = std::array<uint16_t, NOTE_MAX>;

// The pulse timer runs at half the CPU clock with 8 steps (16 cycles per step at the period),
// and the triangle timer runs at the CPU clock with 32 steps
PeriodTable createPeriodTable(double cyclesPerStep)
{
    PeriodTable table{};
    for (int semitone = 0; semitone < NOTE_MAX; semitone++) {
        const double frequency = 440.0 * std::pow(2.0, (semitone - A4_SEMITONE) / 12.0);
        const long period = std::lround(AudioNesApu::CPU_FREQUENCY / (cyclesPerStep * frequency)) - 1;
        table[semitone] = static_cast<uint16_t>(std::clamp<long>(period, 0, MAX_PERIOD));
    }
    return table;
}

size_t toSemitone(uint8_t note)
{
    return std::clamp<uint8_t>(note, NOTE_MIN, NOTE_MAX) - 1;
}

} // namespace

uint16_t getPulsePeriod(uint8_t note)
{
    static const PeriodTable table = createPeriodTable(16.0);
    return table[toSemitone(note)];
}

uint16_t getTrianglePeriod(uint8_t note)
{
    static const PeriodTable table = createPeriodTable(32.0);
    return table[toSemitone(note)];
}

uint8_t getNoisePeriodIndex(uint8_t note)
{
    // As in FamiTracker, "F-#" is the highest pitch, which is the shortest period
    return static_cast<uint8_t>(0x0F - ((note - 1) & 0x0F));
}

} // namespace ocf::audio::tracker
