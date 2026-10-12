// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>
#include <ocf/audio/tracker/Song.h>

using namespace ocf::audio::tracker;

TEST(TrackerSongTest, CreateDefault)
{
    const Song song = Song::createDefault();
    EXPECT_EQ(song.speed, DEFAULT_SPEED);
    EXPECT_EQ(song.tempo, DEFAULT_TEMPO);
    ASSERT_EQ(song.frames.size(), 1u);
    ASSERT_EQ(song.instruments.size(), 1u);

    for (size_t channel = 0; channel < CHANNEL_COUNT; channel++) {
        EXPECT_EQ(song.frames[0][channel], 0);
        ASSERT_EQ(song.patterns[channel].size(), 1u);
        EXPECT_EQ(song.patterns[channel][0].rows.size(), song.rowsPerPattern);
        EXPECT_TRUE(song.patterns[channel][0].isEmpty());
    }
}

TEST(TrackerSongTest, CellIsEmpty)
{
    Cell cell;
    EXPECT_TRUE(cell.isEmpty());

    cell.effects[3] = Effect{EffectType::Halt, 0};
    EXPECT_FALSE(cell.isEmpty());

    cell = Cell{};
    cell.volume = 0;
    EXPECT_FALSE(cell.isEmpty());
}

TEST(TrackerSongTest, GetPatternAddsEmptyPatterns)
{
    Song song = Song::createDefault();
    song.setRowsPerPattern(32);

    Pattern& pattern = song.getPattern(ChannelType::Noise, 3);
    EXPECT_EQ(pattern.rows.size(), 32u);
    EXPECT_EQ(song.patterns[toIndex(ChannelType::Noise)].size(), 4u);
    EXPECT_EQ(song.patterns[toIndex(ChannelType::Pulse1)].size(), 1u);

    EXPECT_NE(song.findPattern(ChannelType::Noise, 3), nullptr);
    EXPECT_EQ(song.findPattern(ChannelType::Noise, 4), nullptr);
}

TEST(TrackerSongTest, FindCell)
{
    Song song = Song::createDefault();
    song.frames.push_back(Frame{1, 0, 0, 0, 0});
    song.getPattern(ChannelType::Pulse1, 1).rows[5].note = 49;

    const Cell* cell = song.findCell(ChannelType::Pulse1, 1, 5);
    ASSERT_NE(cell, nullptr);
    EXPECT_EQ(cell->note, 49);

    EXPECT_EQ(song.findCell(ChannelType::Pulse1, 0, 5)->note, NOTE_NONE);
    EXPECT_EQ(song.findCell(ChannelType::Pulse1, 2, 0), nullptr);
    EXPECT_EQ(song.findCell(ChannelType::Pulse1, 0, song.rowsPerPattern), nullptr);

    // A frame may refer to a pattern that does not exist yet
    song.frames[1][toIndex(ChannelType::Pulse2)] = 9;
    EXPECT_EQ(song.findCell(ChannelType::Pulse2, 1, 0), nullptr);
}

TEST(TrackerSongTest, SetRowsPerPattern)
{
    Song song = Song::createDefault();
    song.getPattern(ChannelType::Triangle, 2);

    song.setRowsPerPattern(16);
    EXPECT_EQ(song.rowsPerPattern, 16);
    for (const std::vector<Pattern>& pool : song.patterns) {
        for (const Pattern& pattern : pool) {
            EXPECT_EQ(pattern.rows.size(), 16u);
        }
    }

    song.setRowsPerPattern(0);
    EXPECT_EQ(song.rowsPerPattern, 1);
    song.setRowsPerPattern(1000);
    EXPECT_EQ(song.rowsPerPattern, MAX_ROWS);
}

TEST(TrackerSongTest, FindInstrument)
{
    Song song = Song::createDefault();
    EXPECT_NE(song.findInstrument(0), nullptr);
    EXPECT_EQ(song.findInstrument(1), nullptr);
    EXPECT_EQ(song.findInstrument(VALUE_NONE), nullptr);
}
