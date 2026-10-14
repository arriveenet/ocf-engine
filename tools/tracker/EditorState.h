// SPDX-License-Identifier: MIT
#pragma once

#include <ocf/audio/tracker/Song.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <optional>
#include <utility>
#include <vector>

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

/*
 * Fields of one channel, in column order: 0: note, 1: instrument, 2: volume, then the effect
 * columns. Selections, copy and paste work on whole fields.
 */
constexpr int FIXED_FIELD_COUNT = 3;

inline int getFieldIndex(int column)
{
    const Field field = getField(column);
    switch (field.type) {
    case FieldType::Note:       return 0;
    case FieldType::Instrument: return 1;
    case FieldType::Volume:     return 2;
    default:                    return FIXED_FIELD_COUNT + field.effect;
    }
}

struct Cursor {
    int frame = 0;
    int row = 0;
    int channel = 0;
    int column = 0;
};

/** A selection, as the cursor positions where it started and where it ends. */
struct Selection {
    Cursor anchor;
    Cursor end;
};

/** The rows and fields a selection covers. */
struct SelectionBlock {
    int rowStart = 0; // Inclusive
    int rowEnd = 0;   // Inclusive
    int channelStart = 0;
    int channelEnd = 0;
    int fieldStart = 0; // Field of channelStart
    int fieldEnd = 0;   // Field of channelEnd

    int getFirstField(int channel) const { return channel == channelStart ? fieldStart : 0; }

    int getLastField(int channel, int fieldCount) const
    {
        return channel == channelEnd ? std::min(fieldEnd, fieldCount - 1) : fieldCount - 1;
    }

    bool operator==(const SelectionBlock&) const = default;
};

/** What undo and redo bring back. */
struct Snapshot {
    Song song;
    Cursor cursor;
    int currentInstrument = 0;
};

constexpr size_t MAX_UNDO_STEPS = 100;

/** Everything the editor windows share. */
struct EditorState {
    Song song = Song::createDefault();
    Cursor cursor;
    int octave = 4;
    int editStep = 1;
    bool editMode = false;
    bool followPlayback = true;
    int currentInstrument = 0;
    int currentDpcmSample = 0;
    uint8_t dpcmKeyNote = 37; // C-3, the key edited in the instrument editor
    bool showDpcmSamples = false;
    std::array<bool, CHANNEL_COUNT> muted{};
    std::optional<Selection> selection; // In the cursor's frame
    PlaybackStatus playback;            // As of this frame

    bool songChanged = true;        // The song must be sent to the player
    bool modified = false;          // Changed since it was created, opened or saved
    int songRevision = 0;           // Incremented when the whole song is replaced, also by undo
    std::filesystem::path filePath; // Empty until the song is saved or opened

    /*
     * Undo and redo keep copies of the whole song. The edits are recorded as one step when no
     * item is active, so that typing in a text field or dragging a slider is one step.
     */
    std::vector<Snapshot> undoStack;
    std::vector<Snapshot> redoStack;
    Song committedSong = song; // As of the last step
    bool editPending = false;  // Edited since the last step
    Cursor editCursor;         // Where the pending edit began
    int editInstrument = 0;

    void markModified()
    {
        songChanged = true;
        modified = true;
        editPending = true;
    }

    void replaceSong(Song newSong, std::filesystem::path path = {})
    {
        song = std::move(newSong);
        filePath = std::move(path);
        cursor = Cursor{};
        currentInstrument = 0;
        currentDpcmSample = 0;
        selection.reset();
        songChanged = true;
        modified = false;
        songRevision++;

        undoStack.clear();
        redoStack.clear();
        committedSong = song;
        editPending = false;
    }

    /** Called at the start of each frame, before anything is edited. */
    void beginFrame()
    {
        if (!editPending) {
            editCursor = cursor;
            editInstrument = currentInstrument;
        }
    }

    /** Records the pending edit as one undo step. */
    void commitEdit()
    {
        if (!editPending) {
            return;
        }
        editPending = false;
        undoStack.push_back({std::move(committedSong), editCursor, editInstrument});
        if (undoStack.size() > MAX_UNDO_STEPS) {
            undoStack.erase(undoStack.begin());
        }
        redoStack.clear();
        committedSong = song;
    }

    bool canUndo() const { return editPending || !undoStack.empty(); }

    bool canRedo() const { return !editPending && !redoStack.empty(); }

    void undo()
    {
        commitEdit();
        if (undoStack.empty()) {
            return;
        }
        redoStack.push_back({std::move(song), cursor, currentInstrument});
        restore(std::move(undoStack.back()));
        undoStack.pop_back();
    }

    void redo()
    {
        if (!canRedo()) {
            return;
        }
        undoStack.push_back({std::move(song), cursor, currentInstrument});
        restore(std::move(redoStack.back()));
        redoStack.pop_back();
    }

    void restore(Snapshot snapshot)
    {
        song = std::move(snapshot.song);
        committedSong = song;
        cursor = snapshot.cursor;
        currentInstrument = snapshot.currentInstrument;
        selection.reset();
        clampCursor();
        songChanged = true;
        modified = true;
        songRevision++;
    }

    int getColumnCount(int channel) const
    {
        return FIXED_COLUMN_COUNT + COLUMNS_PER_EFFECT * song.effectColumns[channel];
    }

    int getFieldCount(int channel) const { return FIXED_FIELD_COUNT + song.effectColumns[channel]; }

    SelectionBlock getBlock(const Selection& selected) const
    {
        const Cursor& anchor = selected.anchor;
        const Cursor& end = selected.end;
        const int lastRow = song.rowsPerPattern - 1;
        SelectionBlock block;
        block.rowStart = std::min(std::min(anchor.row, end.row), lastRow);
        block.rowEnd = std::min(std::max(anchor.row, end.row), lastRow);

        const std::pair anchorField(anchor.channel, getFieldIndex(anchor.column));
        const std::pair endField(end.channel, getFieldIndex(end.column));
        const auto& [first, last] = std::minmax(anchorField, endField);
        block.channelStart = first.first;
        block.fieldStart = first.second;
        block.channelEnd = last.first;
        block.fieldEnd = last.second;
        return block;
    }

    std::optional<SelectionBlock> getSelectedBlock() const
    {
        if (!selection || selection->anchor.frame != cursor.frame) {
            return std::nullopt;
        }
        return getBlock(*selection);
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
