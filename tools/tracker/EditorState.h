// SPDX-License-Identifier: MIT
#pragma once

#include <ocf/audio/tracker/Song.h>

#include <algorithm>
#include <array>
#include <utility>

namespace ocf::tracker_tool {

using namespace audio::tracker;

/*
 * Columns of one channel in the pattern editor:
 *   0: note, 1-2: instrument (high and low digit), 3: volume,
 *   then for each effect column: effect type, parameter high digit, parameter low digit.
 */
constexpr int FIXED_COLUMN_COUNT = 4;
constexpr int COLUMNS_PER_EFFECT = 3;

enum class FieldType : uint8_t {
    Note,
    Instrument,
    Volume,
    EffectType,
    EffectParam,
};

struct Field {
    FieldType type = FieldType::Note;
    int digit = 0;  // 0: high, 1: low
    int effect = 0; // Effect column
};

inline Field getField(int column)
{
    if (column == 0) {
        return {FieldType::Note, 0, 0};
    }
    if (column <= 2) {
        return {FieldType::Instrument, column - 1, 0};
    }
    if (column == 3) {
        return {FieldType::Volume, 0, 0};
    }
    const int effectColumn = column - FIXED_COLUMN_COUNT;
    const int effect = effectColumn / COLUMNS_PER_EFFECT;
    const int sub = effectColumn % COLUMNS_PER_EFFECT;
    if (sub == 0) {
        return {FieldType::EffectType, 0, effect};
    }
    return {FieldType::EffectParam, sub - 1, effect};
}

struct Cursor {
    int frame = 0;
    int row = 0;
    int channel = 0;
    int column = 0;
};

/** Everything the editor windows share. */
struct EditorState {
    Song song = Song::createDefault();
    Cursor cursor;
    int octave = 4;
    int editStep = 1;
    bool editMode = false;
    bool followPlayback = true;
    int currentInstrument = 0;
    std::array<bool, CHANNEL_COUNT> muted{};
    PlaybackStatus playback; // As of this frame

    bool songChanged = true; // The song must be sent to the player
    bool modified = false;   // Changed since it was created or opened
    int songRevision = 0;    // Incremented when the whole song is replaced

    void markModified()
    {
        songChanged = true;
        modified = true;
    }

    void replaceSong(Song newSong)
    {
        song = std::move(newSong);
        cursor = Cursor{};
        currentInstrument = 0;
        songChanged = true;
        modified = false;
        songRevision++;
    }

    int getColumnCount(int channel) const
    {
        return FIXED_COLUMN_COUNT + COLUMNS_PER_EFFECT * song.effectColumns[channel];
    }

    ChannelType getCursorChannel() const { return static_cast<ChannelType>(cursor.channel); }

    /** Adds the pattern if the frame refers to one that does not exist yet. */
    Pattern& getPattern(int frame, int channel)
    {
        return song.getPattern(static_cast<ChannelType>(channel), song.frames[frame][channel]);
    }

    Cell& getCursorCell() { return getPattern(cursor.frame, cursor.channel).rows[cursor.row]; }

    uint8_t getCurrentInstrument() const
    {
        if (currentInstrument < 0 ||
            currentInstrument >= static_cast<int>(song.instruments.size())) {
            return VALUE_NONE;
        }
        return static_cast<uint8_t>(currentInstrument);
    }

    void clampCursor()
    {
        cursor.frame = std::clamp(cursor.frame, 0, static_cast<int>(song.frames.size()) - 1);
        cursor.row = std::clamp(cursor.row, 0, song.rowsPerPattern - 1);
        cursor.channel = std::clamp(cursor.channel, 0, static_cast<int>(CHANNEL_COUNT) - 1);
        cursor.column = std::clamp(cursor.column, 0, getColumnCount(cursor.channel) - 1);
    }
};

inline const char* getChannelName(ChannelType channel)
{
    switch (channel) {
    case ChannelType::Pulse1:   return "Pulse 1";
    case ChannelType::Pulse2:   return "Pulse 2";
    case ChannelType::Triangle: return "Triangle";
    case ChannelType::Noise:    return "Noise";
    default:                    return "DPCM";
    }
}

} // namespace ocf::tracker_tool
