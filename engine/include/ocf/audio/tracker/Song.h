// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/audio/tracker/TrackerTypes.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ocf::audio::tracker {

struct Effect {
    EffectType type = EffectType::None;
    uint8_t param = 0;

    bool operator==(const Effect&) const = default;
};

/** One row of one channel. */
struct Cell {
    uint8_t note = NOTE_NONE;
    uint8_t instrument = VALUE_NONE;
    uint8_t volume = VALUE_NONE;
    std::array<Effect, MAX_EFFECT_COLUMNS> effects{};

    bool isEmpty() const;

    bool operator==(const Cell&) const = default;
};

struct Pattern {
    std::vector<Cell> rows;

    bool isEmpty() const;
};

/**
 * Per-tick values of an instrument. While the note is held, the sequence waits at releasePoint
 * (looping back to loopPoint if it comes before releasePoint). After the note is released, it
 * continues from releasePoint + 1. At the end it loops back to loopPoint, or holds the last value.
 */
struct Sequence {
    std::vector<int8_t> values;
    int16_t loopPoint = -1;    // -1: none
    int16_t releasePoint = -1; // -1: none

    bool isEmpty() const { return values.empty(); }

    bool operator==(const Sequence&) const = default;
};

struct Instrument {
    std::string name;
    std::array<Sequence, SEQUENCE_TYPE_COUNT> sequences;

    Sequence& getSequence(SequenceType type) { return sequences[toIndex(type)]; }
    const Sequence& getSequence(SequenceType type) const { return sequences[toIndex(type)]; }
};

/** Pattern index of each channel. */
using Frame = std::array<uint8_t, CHANNEL_COUNT>;

/**
 * A song for the NES APU. The song plays its frames in order, and each frame plays one pattern
 * per channel. Every pattern has rowsPerPattern rows.
 */
struct Song {
    std::string title;
    std::string author;
    uint8_t speed = DEFAULT_SPEED;
    uint8_t tempo = DEFAULT_TEMPO;
    uint16_t rowsPerPattern = DEFAULT_ROWS_PER_PATTERN;
    std::array<uint8_t, CHANNEL_COUNT> effectColumns{1, 1, 1, 1, 1}; // Shown in the editor
    std::vector<Frame> frames;
    std::array<std::vector<Pattern>, CHANNEL_COUNT> patterns;
    std::vector<Instrument> instruments;

    /** One frame of empty patterns and one instrument. */
    static Song createDefault();

    /** Adds empty patterns as needed, so that the pattern exists. */
    Pattern& getPattern(ChannelType channel, uint8_t index);

    const Pattern* findPattern(ChannelType channel, uint8_t index) const;

    const Cell* findCell(ChannelType channel, size_t frame, size_t row) const;

    const Instrument* findInstrument(uint8_t index) const;

    /** Resizes every pattern. The value is clamped to 1..MAX_ROWS. */
    void setRowsPerPattern(uint16_t rows);
};

} // namespace ocf::audio::tracker
