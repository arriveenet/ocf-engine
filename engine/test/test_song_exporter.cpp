// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>

#include "ocf/audio/AudioTrackerSource.h"

#include <ocf/audio/tracker/SongExporter.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>

using namespace ocf::audio;
using namespace ocf::audio::tracker;

namespace {

constexpr uint8_t A4 = 58;
constexpr int ROWS = 16;
constexpr double TICK_RATE = 60.0988;

/** One frame of 16 rows: A-4 on pulse 1, at speed 6 (6 ticks per row). */
Song createSong(bool halt)
{
    Song song = Song::createDefault();
    song.setRowsPerPattern(ROWS);
    song.getPattern(ChannelType::Pulse1, 0).rows[0].note = A4;
    if (halt) {
        song.getPattern(ChannelType::Pulse1, 0).rows[ROWS - 1].effects[0] = Effect{EffectType::Halt, 0};
    }
    return song;
}

double secondsOfOnePlay()
{
    return ROWS * 6 / TICK_RATE;
}

struct Wav {
    uint32_t sampleRate = 0;
    uint16_t channels = 0;
    uint16_t bitsPerSample = 0;
    std::vector<int16_t> samples;
};

uint32_t readLittleEndian(const std::vector<char>& data, size_t offset, int bytes)
{
    uint32_t value = 0;
    for (int i = 0; i < bytes; i++) {
        value |= static_cast<uint32_t>(static_cast<uint8_t>(data[offset + i])) << (8 * i);
    }
    return value;
}

Wav readWav(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    const std::vector<char> data((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
    Wav wav;
    EXPECT_GE(data.size(), 44u);
    if (data.size() < 44) {
        return wav;
    }
    EXPECT_EQ(std::memcmp(data.data(), "RIFF", 4), 0);
    EXPECT_EQ(readLittleEndian(data, 4, 4), data.size() - 8);
    EXPECT_EQ(std::memcmp(data.data() + 8, "WAVEfmt ", 8), 0);
    EXPECT_EQ(readLittleEndian(data, 20, 2), 1u); // PCM
    wav.channels = static_cast<uint16_t>(readLittleEndian(data, 22, 2));
    wav.sampleRate = readLittleEndian(data, 24, 4);
    wav.bitsPerSample = static_cast<uint16_t>(readLittleEndian(data, 34, 2));
    EXPECT_EQ(std::memcmp(data.data() + 36, "data", 4), 0);
    const uint32_t dataSize = readLittleEndian(data, 40, 4);
    EXPECT_EQ(dataSize, data.size() - 44);
    wav.samples.resize(dataSize / 2);
    std::memcpy(wav.samples.data(), data.data() + 44, wav.samples.size() * 2);
    return wav;
}

double rms(const std::vector<int16_t>& samples, size_t begin, size_t end)
{
    double sum = 0.0;
    for (size_t i = begin; i < end; i++) {
        sum += static_cast<double>(samples[i]) * samples[i];
    }
    return std::sqrt(sum / static_cast<double>(end - begin)) / 32767.0;
}

class SongExporterTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_directory = std::filesystem::temp_directory_path() / "ocf_song_exporter_test";
        std::filesystem::remove_all(m_directory);
        std::filesystem::create_directories(m_directory);
        m_path = m_directory / "song.wav";
    }

    void TearDown() override { std::filesystem::remove_all(m_directory); }

    std::filesystem::path m_directory;
    std::filesystem::path m_path;
};

} // namespace

TEST(SongExporterMeasureTest, SongWithHalt)
{
    const WavExportLength length = SongExporter::measure(createSong(true), WavExportOptions{});
    EXPECT_FALSE(length.loops);
    EXPECT_NEAR(length.seconds, secondsOfOnePlay() + 0.25, 1.0 / TICK_RATE);
}

TEST(SongExporterMeasureTest, LoopingSongPlaysTheGivenNumberOfTimes)
{
    WavExportOptions options;
    options.fadeOutSeconds = 0.0f;
    for (int playCount : {1, 2, 3}) {
        options.playCount = playCount;
        const WavExportLength length = SongExporter::measure(createSong(false), options);
        EXPECT_TRUE(length.loops);
        EXPECT_NEAR(length.seconds, playCount * secondsOfOnePlay(), 1.0 / TICK_RATE) << playCount;
    }

    options.playCount = 2;
    options.fadeOutSeconds = 3.0f;
    EXPECT_NEAR(SongExporter::measure(createSong(false), options).seconds,
                2 * secondsOfOnePlay() + 3.0, 1.0 / TICK_RATE);
}

TEST(SongExporterMeasureTest, JumpBackCountsAsTheNextPlay)
{
    // Frame 0 is an intro: Bxx at the end of frame 1 jumps back to frame 1
    Song song = createSong(false);
    song.frames = {Frame{0, 0, 0, 0, 0}, Frame{1, 0, 0, 0, 0}};
    song.getPattern(ChannelType::Pulse1, 1).rows[ROWS - 1].effects[0] = Effect{EffectType::Jump, 1};

    WavExportOptions options;
    options.playCount = 2;
    options.fadeOutSeconds = 0.0f;
    // Intro, frame 1, then frame 1 again
    EXPECT_NEAR(SongExporter::measure(song, options).seconds, 3 * secondsOfOnePlay(),
                1.0 / TICK_RATE);
}

TEST_F(SongExporterTest, WritesSixteenBitMonoWav)
{
    WavExportOptions options;
    options.sampleRate = 44100;
    const auto result = SongExporter::exportWav(createSong(true), m_path, options);
    ASSERT_TRUE(result.isOk()) << result.unwrapErr();

    const Wav wav = readWav(m_path);
    EXPECT_EQ(wav.sampleRate, 44100u);
    EXPECT_EQ(wav.channels, 1);
    EXPECT_EQ(wav.bitsPerSample, 16);

    const double expected = SongExporter::measure(createSong(true), options).seconds * 44100.0;
    EXPECT_NEAR(static_cast<double>(wav.samples.size()), expected, 44100.0 / TICK_RATE + 2.0);

    // The note sounds, and it is silent after Cxx
    EXPECT_GT(rms(wav.samples, 4410, 8820), 0.01);
    EXPECT_LT(rms(wav.samples, wav.samples.size() - 2000, wav.samples.size()), 0.002);
    EXPECT_FALSE(std::filesystem::exists(m_directory / "song.wav.tmp"));
}

TEST_F(SongExporterTest, SoundsLikeTheRealTimePlayback)
{
    const Song song = createSong(true);
    ASSERT_TRUE(SongExporter::exportWav(song, m_path, WavExportOptions{}).isOk());
    const Wav wav = readWav(m_path);

    AudioTrackerSource source;
    source.setSong(std::make_shared<const Song>(song));
    source.playSong(0, 0);
    const size_t count = std::min<size_t>(wav.samples.size(), 9600);
    std::vector<float> output(count * InternalChannels);
    source.render(output.data(), static_cast<uint32_t>(count), InternalChannels);

    for (size_t i = 0; i < count; i++) {
        const auto expected = static_cast<int16_t>(std::lround(output[i * InternalChannels] * 32767.0f));
        ASSERT_EQ(wav.samples[i], expected) << "sample " << i;
    }
}

TEST_F(SongExporterTest, FadesOutAfterTheLastPlay)
{
    WavExportOptions options;
    options.playCount = 1;
    options.fadeOutSeconds = 0.5f;
    ASSERT_TRUE(SongExporter::exportWav(createSong(false), m_path, options).isOk());
    const Wav wav = readWav(m_path);

    const auto playSamples = static_cast<size_t>(secondsOfOnePlay() * 48000.0);
    const auto fadeSamples = static_cast<size_t>(0.5 * 48000.0);
    EXPECT_NEAR(static_cast<double>(wav.samples.size()), playSamples + fadeSamples, 48000.0 / TICK_RATE + 2.0);

    // Full volume before the fade, quieter in the middle of it, nearly silent at the end
    const double before = rms(wav.samples, playSamples - 4800, playSamples);
    const double middle = rms(wav.samples, playSamples + fadeSamples / 2 - 1200, playSamples + fadeSamples / 2 + 1200);
    const double end = rms(wav.samples, wav.samples.size() - 480, wav.samples.size());
    EXPECT_GT(before, 0.01);
    EXPECT_LT(middle, before * 0.7);
    EXPECT_GT(middle, before * 0.3);
    EXPECT_LT(end, before * 0.05);
}

TEST_F(SongExporterTest, CancellingKeepsTheExistingFile)
{
    {
        std::ofstream existing(m_path, std::ios::binary);
        existing << "existing";
    }

    WavExportOptions options;
    options.playCount = 4;
    int calls = 0;
    const auto result = SongExporter::exportWav(createSong(false), m_path, options, [&](double progress) {
        EXPECT_GE(progress, 0.0);
        EXPECT_LE(progress, 1.0);
        return ++calls < 3;
    });
    ASSERT_TRUE(result.isErr());
    EXPECT_EQ(calls, 3);

    std::ifstream file(m_path, std::ios::binary);
    const std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    EXPECT_EQ(text, "existing");
    EXPECT_FALSE(std::filesystem::exists(m_directory / "song.wav.tmp"));
}

TEST_F(SongExporterTest, ReportsProgressUpToOne)
{
    std::vector<double> progress;
    const auto result = SongExporter::exportWav(createSong(true), m_path, WavExportOptions{}, [&](double value) {
        progress.push_back(value);
        return true;
    });
    ASSERT_TRUE(result.isOk()) << result.unwrapErr();
    ASSERT_FALSE(progress.empty());
    EXPECT_TRUE(std::is_sorted(progress.begin(), progress.end()));
    EXPECT_DOUBLE_EQ(progress.back(), 1.0);
}
