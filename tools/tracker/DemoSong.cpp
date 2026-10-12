// SPDX-License-Identifier: MIT
#include "DemoSong.h"

#include <ocf/audio/tracker/SongNotation.h>

#include <initializer_list>
#include <string_view>

namespace ocf::tracker_tool {

using namespace audio::tracker;

namespace {

constexpr uint8_t LEAD = 0;
constexpr uint8_t ECHO = 1;
constexpr uint8_t BASS = 2;
constexpr uint8_t HIHAT = 3;

struct NoteEvent {
    uint16_t row;
    std::string_view note;
};

void addInstrument(Song& song, std::string_view name, std::string_view volume,
                   std::string_view duty)
{
    Instrument instrument;
    instrument.name = name;
    instrument.getSequence(SequenceType::Volume) = sequenceFromMml(volume).value_or(Sequence{});
    instrument.getSequence(SequenceType::Duty) = sequenceFromMml(duty).value_or(Sequence{});
    song.instruments.push_back(instrument);
}

/** The instrument is set on the first note only, as a tracker user would write it. */
void writeNotes(Song& song, ChannelType channel, uint8_t pattern, uint8_t instrument,
                std::initializer_list<NoteEvent> events, int rowOffset = 0)
{
    Pattern& target = song.getPattern(channel, pattern);
    bool first = true;
    for (const NoteEvent& event : events) {
        const int row = event.row + rowOffset;
        if (row >= static_cast<int>(target.rows.size())) {
            continue;
        }
        Cell& cell = target.rows[row];
        cell.note = noteFromString(channel, event.note).value_or(NOTE_NONE);
        if (first && cell.note != NOTE_OFF && cell.note != NOTE_RELEASE) {
            cell.instrument = instrument;
            first = false;
        }
    }
}

} // namespace

Song createDemoSong()
{
    Song song = Song::createDefault();
    song.title = "Demo Song";
    song.author = "ocf-engine";
    song.setRowsPerPattern(32);
    song.frames = {Frame{0, 0, 0, 0, 0}, Frame{1, 1, 1, 0, 0}};

    song.instruments.clear();
    addInstrument(song, "Lead", "15 14 13 12 11 10 10 9 9 / 8 5 3 1 0", "2");
    addInstrument(song, "Echo", "6 6 5 5 4 4 3 3 2", "1");
    addInstrument(song, "Bass", "15", "");
    addInstrument(song, "Hi-hat", "10 6 3 1 0", "0");

    const auto melody0 = {NoteEvent{0, "C-5"},  NoteEvent{4, "G-4"},  NoteEvent{8, "E-4"},
                          NoteEvent{12, "G-4"}, NoteEvent{16, "A-4"}, NoteEvent{18, "B-4"},
                          NoteEvent{20, "C-5"}, NoteEvent{24, "G-4"}, NoteEvent{28, "==="}};
    const auto melody1 = {NoteEvent{0, "F-4"},  NoteEvent{4, "A-4"},  NoteEvent{8, "C-5"},
                          NoteEvent{12, "A-4"}, NoteEvent{16, "G-4"}, NoteEvent{20, "E-4"},
                          NoteEvent{24, "D-4"}, NoteEvent{28, "==="}};

    // Lead on pulse 1, and its echo two rows later on pulse 2
    writeNotes(song, ChannelType::Pulse1, 0, LEAD, melody0);
    writeNotes(song, ChannelType::Pulse1, 1, LEAD, melody1);
    writeNotes(song, ChannelType::Pulse2, 0, ECHO, melody0, 2);
    writeNotes(song, ChannelType::Pulse2, 1, ECHO, melody1, 2);
    song.getPattern(ChannelType::Pulse2, 0).rows[30].note = NOTE_OFF;
    song.getPattern(ChannelType::Pulse2, 1).rows[30].note = NOTE_OFF;

    // Bass on the triangle, cut just before each note
    writeNotes(song, ChannelType::Triangle, 0, BASS,
               {{0, "C-3"}, {7, "---"}, {8, "G-2"}, {15, "---"}, {16, "A-2"}, {23, "---"},
                {24, "G-2"}, {31, "---"}});
    writeNotes(song, ChannelType::Triangle, 1, BASS,
               {{0, "F-2"}, {7, "---"}, {8, "C-3"}, {15, "---"}, {16, "G-2"}, {23, "---"},
                {24, "G-2"}, {31, "---"}});

    // Hi-hat on the noise channel, quieter on the off-beats. Silent until the APU has noise
    Pattern& hihat = song.getPattern(ChannelType::Noise, 0);
    for (uint16_t row = 0; row < song.rowsPerPattern; row += 2) {
        Cell& cell = hihat.rows[row];
        cell.note = noteFromString(ChannelType::Noise, (row % 4 == 0) ? "C-#" : "E-#").value();
        cell.volume = (row % 4 == 0) ? 12 : 6;
    }
    hihat.rows[0].instrument = HIHAT;

    return song;
}

} // namespace ocf::tracker_tool
