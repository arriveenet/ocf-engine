// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/audio/tracker/Dpcm.h"
#include "ocf/audio/tracker/TrackerTypes.h"

#include <array>
#include <cstdint>
#include <optional>
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

/** What a note of an instrument plays on the DPCM channel. */
struct DpcmKey {
    uint8_t sample = VALUE_NONE; // VALUE_NONE: nothing
    uint8_t rate = DPCM_MAX_RATE;
    bool loop = false;

    bool isAssigned() const { return sample != VALUE_NONE; }

    bool operator==(const DpcmKey&) const = default;
};

struct Instrument {
    std::string name;
    std::array<Sequence, SEQUENCE_TYPE_COUNT> sequences;
    std::array<DpcmKey, NOTE_MAX> dpcmKeys{}; // By note, from NOTE_MIN

    Sequence& getSequence(SequenceType type) { return sequences[toIndex(type)]; }
    const Sequence& getSequence(SequenceType type) const { return sequences[toIndex(type)]; }

    /** nullptr for notes that are not NOTE_MIN-NOTE_MAX. */
    DpcmKey* findDpcmKey(uint8_t note);
    const DpcmKey* findDpcmKey(uint8_t note) const;
};

/** A sample of the DMC channel. See Dpcm.h. */
struct DpcmSample {
    std::string name;
    std::vector<uint8_t> data; // 16n + 1 bytes, at most DPCM_MAX_SAMPLE_BYTES. Empty: nothing
    uint8_t rate = DPCM_MAX_RATE; // The rate it was made for, the initial rate of new keys
    uint8_t restLevel = DPCM_DEFAULT_LEVEL; // It starts from this level: $4011 is set to it
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
    std::vector<DpcmSample> dpcmSamples;

    /** One frame of empty patterns and one instrument. */
    static Song createDefault();

    /** Adds empty patterns as needed, so that the pattern exists. */
    Pattern& getPattern(ChannelType channel, uint8_t index);

    const Pattern* findPattern(ChannelType channel, uint8_t index) const;

    const Cell* findCell(ChannelType channel, size_t frame, size_t row) const;

    const Instrument* findInstrument(uint8_t index) const;

    /**
     * Where the sample is in the DPCM memory ($C000-$FFFF), as an offset from $C000. The samples
     * are placed in order at 64-byte boundaries. nullopt when there is no such sample, it is
     * empty, or it does not fit in the memory.
     */
    std::optional<size_t> getDpcmSampleOffset(size_t index) const;

    /** The DPCM memory that the samples take, with the alignment. */
    size_t getDpcmMemoryUsed() const;

    /** Resizes every pattern. The value is clamped to 1..MAX_ROWS. */
    void setRowsPerPattern(uint16_t rows);
};

} // namespace ocf::audio::tracker
