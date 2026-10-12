// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>

#include "audio/tracker/NoteTable.h"
#include "audio/tracker/SongPlayer.h"

#include <map>
#include <optional>
#include <vector>

using namespace ocf::audio::tracker;

namespace {

constexpr uint8_t A4 = 58;
constexpr uint8_t C4 = 49;

/** Records the register writes of a SongPlayer. */
class SongPlayerTest : public ::testing::Test {
protected:
    SongPlayerTest()
        : m_player([this](uint16_t address, uint8_t data) {
            m_writes.push_back({address, data});
            m_registers[address] = data;
        })
    {
        m_song = Song::createDefault();
        m_song.setRowsPerPattern(4);
    }

    struct Write {
        uint16_t address;
        uint8_t data;
    };

    Cell& cell(ChannelType channel, uint8_t pattern, uint16_t row)
    {
        return m_song.getPattern(channel, pattern).rows[row];
    }

    void start(uint16_t frame = 0, uint16_t row = 0)
    {
        m_player.setSong(std::make_shared<const Song>(m_song));
        m_player.play(frame, row);
    }

    void tick(int count = 1)
    {
        for (int i = 0; i < count; i++) {
            m_writes.clear();
            m_player.tick();
        }
    }

    std::optional<uint8_t> written(uint16_t address) const
    {
        for (auto it = m_writes.rbegin(); it != m_writes.rend(); ++it) {
            if (it->address == address) {
                return it->data;
            }
        }
        return std::nullopt;
    }

    uint8_t reg(uint16_t address) const
    {
        const auto it = m_registers.find(address);
        return it != m_registers.end() ? it->second : 0;
    }

    uint8_t pulse1Volume() const { return reg(0x4000) & 0x0F; }

    Song m_song;
    SongPlayer m_player;
    std::vector<Write> m_writes;
    std::map<uint16_t, uint8_t> m_registers;
};

} // namespace

TEST(NoteTableTest, Periods)
{
    EXPECT_EQ(getPulsePeriod(A4), 253);
    EXPECT_EQ(getTrianglePeriod(A4), 126);
    EXPECT_EQ(getPulsePeriod(NOTE_MIN), 0x7FF); // Clamped
    EXPECT_LT(getPulsePeriod(NOTE_MAX), getPulsePeriod(NOTE_MAX - 1));
    EXPECT_EQ(getNoisePeriodIndex(1), 0x0F);
    EXPECT_EQ(getNoisePeriodIndex(16), 0x00);
}

TEST_F(SongPlayerTest, InitializesApuOnFirstTick)
{
    tick();
    EXPECT_EQ(m_writes.front().address, 0x4015);
    EXPECT_EQ(m_writes.front().data, 0x0F);
    EXPECT_EQ(written(0x4001), 0x08);
    EXPECT_EQ(written(0x4005), 0x08);
}

TEST_F(SongPlayerTest, PlaysPulseNote)
{
    cell(ChannelType::Pulse1, 0, 0).note = A4;
    start();
    tick();

    EXPECT_EQ(written(0x4002), 253);
    ASSERT_TRUE(written(0x4003));
    EXPECT_EQ(*written(0x4003) & 0x07, 0);
    EXPECT_EQ(written(0x4000), 0x30 | 15); // Duty 0, halted, constant volume 15
}

TEST_F(SongPlayerTest, WritesPeriodHighOnlyWhenNeeded)
{
    cell(ChannelType::Pulse1, 0, 0).note = A4;
    start();
    tick();
    tick();
    EXPECT_TRUE(written(0x4002));
    EXPECT_FALSE(written(0x4003));
}

TEST_F(SongPlayerTest, PlaysTriangleNote)
{
    cell(ChannelType::Triangle, 0, 0).note = A4;
    cell(ChannelType::Triangle, 0, 1).note = NOTE_OFF;
    start();
    tick();
    EXPECT_EQ(written(0x4008), 0xFF);
    EXPECT_EQ(written(0x400A), 126);
    EXPECT_TRUE(written(0x400B));

    tick(6);
    EXPECT_EQ(written(0x4008), 0x00);
    EXPECT_FALSE(written(0x400A));
}

TEST_F(SongPlayerTest, AdvancesRowsBySpeedAndTempo)
{
    start();
    tick();
    EXPECT_EQ(m_player.getStatus().row, 0);
    tick(5);
    EXPECT_EQ(m_player.getStatus().row, 0);
    tick();
    EXPECT_EQ(m_player.getStatus().row, 1);

    // Speed 3 at tempo 150 gives 3 ticks per row
    m_song.speed = 3;
    start();
    tick(4);
    EXPECT_EQ(m_player.getStatus().row, 1);
}

TEST_F(SongPlayerTest, SpeedEffect)
{
    cell(ChannelType::Pulse2, 0, 0).effects[0] = Effect{EffectType::Speed, 2};
    start();
    tick(3);
    EXPECT_EQ(m_player.getStatus().row, 1);
}

TEST_F(SongPlayerTest, TempoEffect)
{
    // Tempo 75 at speed 6: 3600 / 300 = 12 ticks per row
    cell(ChannelType::Pulse1, 0, 0).effects[1] = Effect{EffectType::Speed, 75};
    start();
    tick(12);
    EXPECT_EQ(m_player.getStatus().row, 0);
    tick();
    EXPECT_EQ(m_player.getStatus().row, 1);
}

TEST_F(SongPlayerTest, LoopsToFirstFrameAfterLastFrame)
{
    m_song.speed = 1;
    m_song.frames.push_back(Frame{});
    start();
    tick(8); // 2 frames of 4 rows
    EXPECT_EQ(m_player.getStatus().frame, 1);
    EXPECT_EQ(m_player.getStatus().row, 3);
    tick();
    EXPECT_EQ(m_player.getStatus().frame, 0);
    EXPECT_EQ(m_player.getStatus().row, 0);
}

TEST_F(SongPlayerTest, JumpEffect)
{
    m_song.speed = 1;
    m_song.frames = {Frame{0}, Frame{1}, Frame{2}};
    cell(ChannelType::Pulse1, 0, 1).effects[0] = Effect{EffectType::Jump, 2};
    start();
    tick(3);
    EXPECT_EQ(m_player.getStatus().frame, 2);
    EXPECT_EQ(m_player.getStatus().row, 0);
}

TEST_F(SongPlayerTest, SkipEffect)
{
    m_song.speed = 1;
    m_song.frames = {Frame{0}, Frame{1}};
    cell(ChannelType::Dpcm, 0, 0).effects[0] = Effect{EffectType::Skip, 2};
    start();
    tick(2);
    EXPECT_EQ(m_player.getStatus().frame, 1);
    EXPECT_EQ(m_player.getStatus().row, 2);
}

TEST_F(SongPlayerTest, HaltEffect)
{
    m_song.speed = 1;
    cell(ChannelType::Pulse1, 0, 0).note = A4;
    cell(ChannelType::Pulse1, 0, 1).effects[0] = Effect{EffectType::Halt, 0};
    start();
    tick(2);
    EXPECT_TRUE(m_player.getStatus().playing);
    EXPECT_EQ(pulse1Volume(), 15);

    tick();
    EXPECT_FALSE(m_player.getStatus().playing);
    EXPECT_EQ(pulse1Volume(), 0);
}

TEST_F(SongPlayerTest, VolumeColumnScalesInstrumentVolume)
{
    m_song.instruments[0].getSequence(SequenceType::Volume).values = {10};
    Cell& first = cell(ChannelType::Pulse1, 0, 0);
    first.note = A4;
    first.instrument = 0;
    first.volume = 6;
    start();
    tick();
    EXPECT_EQ(pulse1Volume(), 4); // 10 * 6 / 15

    m_song.instruments[0].getSequence(SequenceType::Volume).values = {1};
    first.volume = 1;
    start();
    tick();
    EXPECT_EQ(pulse1Volume(), 1); // Rounded up so that the note is not silent
}

TEST_F(SongPlayerTest, DutySequence)
{
    m_song.instruments[0].getSequence(SequenceType::Duty).values = {2, 1};
    Cell& first = cell(ChannelType::Pulse2, 0, 0);
    first.note = C4;
    first.instrument = 0;
    start();
    tick();
    EXPECT_EQ(reg(0x4004) >> 6, 2);
    tick();
    EXPECT_EQ(reg(0x4004) >> 6, 1);
    tick();
    EXPECT_EQ(reg(0x4004) >> 6, 1); // Holds the last value
}

TEST_F(SongPlayerTest, VolumeSequenceLoops)
{
    Sequence& volume = m_song.instruments[0].getSequence(SequenceType::Volume);
    volume.values = {15, 10, 5};
    volume.loopPoint = 1;
    Cell& first = cell(ChannelType::Pulse1, 0, 0);
    first.note = A4;
    first.instrument = 0;
    start();

    std::vector<int> volumes;
    for (int i = 0; i < 6; i++) {
        tick();
        volumes.push_back(pulse1Volume());
    }
    EXPECT_EQ(volumes, (std::vector<int>{15, 10, 5, 10, 5, 10}));
}

TEST_F(SongPlayerTest, VolumeSequenceWaitsForRelease)
{
    Sequence& volume = m_song.instruments[0].getSequence(SequenceType::Volume);
    volume.values = {15, 12, 8, 4, 0};
    volume.releasePoint = 2;
    Cell& first = cell(ChannelType::Pulse1, 0, 0);
    first.note = A4;
    first.instrument = 0;
    cell(ChannelType::Pulse1, 0, 1).note = NOTE_RELEASE;
    start();

    std::vector<int> volumes;
    for (int i = 0; i < 9; i++) {
        tick();
        volumes.push_back(pulse1Volume());
    }
    // Waits at index 2 until the release on the next row, then continues from index 3
    EXPECT_EQ(volumes, (std::vector<int>{15, 12, 8, 8, 8, 8, 4, 0, 0}));
}

TEST_F(SongPlayerTest, VolumeSequenceLoopsBeforeRelease)
{
    Sequence& volume = m_song.instruments[0].getSequence(SequenceType::Volume);
    volume.values = {15, 10, 5, 2};
    volume.loopPoint = 1;
    volume.releasePoint = 2;
    Cell& first = cell(ChannelType::Pulse1, 0, 0);
    first.note = A4;
    first.instrument = 0;
    cell(ChannelType::Pulse1, 0, 1).note = NOTE_RELEASE;
    start();

    std::vector<int> volumes;
    for (int i = 0; i < 9; i++) {
        tick();
        volumes.push_back(pulse1Volume());
    }
    // Loops 10, 5 until the release, then plays the rest and holds the last value
    EXPECT_EQ(volumes, (std::vector<int>{15, 10, 5, 10, 5, 10, 2, 2, 2}));
}

TEST_F(SongPlayerTest, NoteOffSilencesChannel)
{
    cell(ChannelType::Pulse1, 0, 0).note = A4;
    cell(ChannelType::Pulse1, 0, 1).note = NOTE_OFF;
    start();
    tick();
    EXPECT_EQ(pulse1Volume(), 15);
    tick(6);
    EXPECT_EQ(pulse1Volume(), 0);
}

TEST_F(SongPlayerTest, MutedChannelIsSilent)
{
    cell(ChannelType::Pulse1, 0, 0).note = A4;
    cell(ChannelType::Triangle, 0, 0).note = A4;
    m_player.setChannelMuted(ChannelType::Pulse1, true);
    m_player.setChannelMuted(ChannelType::Triangle, true);
    start();
    tick();
    EXPECT_EQ(pulse1Volume(), 0);
    EXPECT_EQ(reg(0x4008), 0x00);

    m_player.setChannelMuted(ChannelType::Pulse1, false);
    tick();
    EXPECT_EQ(pulse1Volume(), 15);
}

TEST_F(SongPlayerTest, NoiseNote)
{
    Cell& first = cell(ChannelType::Noise, 0, 0);
    first.note = 16; // "F-#"
    first.volume = 9;
    start();
    tick();
    EXPECT_EQ(written(0x400C), 0x30 | 9);
    EXPECT_EQ(written(0x400E), 0x00);
    EXPECT_TRUE(written(0x400F));
}

TEST_F(SongPlayerTest, DpcmIsNotPlayed)
{
    cell(ChannelType::Dpcm, 0, 0).note = C4;
    start();
    tick(3);
    for (const auto& [address, data] : m_registers) {
        EXPECT_TRUE(address < 0x4010 || address == 0x4015) << std::hex << address;
    }
}

TEST_F(SongPlayerTest, PreviewNoteWhileStopped)
{
    m_player.setSong(std::make_shared<const Song>(m_song));
    m_player.playNote(ChannelType::Pulse2, A4, 0);
    tick();
    EXPECT_FALSE(m_player.getStatus().playing);
    EXPECT_EQ(reg(0x4004) & 0x0F, 15);
    EXPECT_EQ(written(0x4006), 253);

    m_player.stopNote(ChannelType::Pulse2);
    tick();
    EXPECT_EQ(reg(0x4004) & 0x0F, 0);
}

TEST_F(SongPlayerTest, SetSongKeepsPosition)
{
    m_song.speed = 1;
    m_song.frames.push_back(Frame{});
    start(1, 2);
    tick();
    EXPECT_EQ(m_player.getStatus().frame, 1);
    EXPECT_EQ(m_player.getStatus().row, 2);

    // A new song with fewer frames: the position wraps when moving to the next row
    m_song.frames.pop_back();
    m_player.setSong(std::make_shared<const Song>(m_song));
    tick();
    EXPECT_TRUE(m_player.getStatus().playing);
    EXPECT_EQ(m_player.getStatus().frame, 0);
}

TEST_F(SongPlayerTest, SetSongReturnsPreviousSong)
{
    auto first = std::make_shared<const Song>(m_song);
    EXPECT_EQ(m_player.setSong(first), nullptr);
    EXPECT_EQ(m_player.setSong(std::make_shared<const Song>(m_song)), first);
}
