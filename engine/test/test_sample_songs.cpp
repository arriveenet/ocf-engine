// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>

#include "ocf/audio/tracker/SongPlayer.h"

#include <ocf/audio/tracker/SongSerializer.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

using namespace ocf::audio::tracker;

namespace {

const std::filesystem::path SONGS_DIR = std::filesystem::path(OCF_TEST_ASSETS_DIR) / "songs";
const std::filesystem::path CANON_IN_D = SONGS_DIR / "canon_in_d.json";
const std::filesystem::path AIR_ON_THE_G_STRING = SONGS_DIR / "air_on_the_g_string.json";

/** With LF line endings, as the serializer writes, even if the checkout converted them. */
std::string readFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::erase(text, '\r');
    return text;
}

void expectValidAndCanonical(const std::filesystem::path& path)
{
    const std::string json = readFile(path);
    ASSERT_FALSE(json.empty()) << path;

    const auto result = SongSerializer::loadFromString(json);
    ASSERT_TRUE(result.isOk()) << result.unwrapErr();

    // Already in the layout the serializer writes, so saving it from the tracker gives no diff
    EXPECT_EQ(SongSerializer::saveToString(result.unwrap()), json);
}

struct PlayedSong {
    std::shared_ptr<const Song> song;
    std::vector<int> frames; ///< Frames in the order they were played
    PlaybackStatus lastPlayed;
};

/** Plays the song from the start until it stops. */
void playUntilStop(const std::filesystem::path& path, PlayedSong& played)
{
    auto result = SongSerializer::load(path);
    ASSERT_TRUE(result.isOk()) << result.unwrapErr();
    played.song = std::make_shared<const Song>(std::move(result).unwrap());

    SongPlayer player([](uint16_t, uint8_t) {});
    player.setSong(played.song);
    player.play(0, 0);

    for (int tick = 0; tick == 0 || player.getStatus().playing; tick++) {
        ASSERT_LT(tick, 100000) << "the song does not stop";
        player.tick();
        const PlaybackStatus status = player.getStatus();
        if (!status.playing) {
            break;
        }
        played.lastPlayed = status;
        if (played.frames.empty() || played.frames.back() != status.frame) {
            played.frames.push_back(status.frame);
        }
    }
}

std::vector<int> allFrames(const Song& song)
{
    std::vector<int> frames(song.frames.size());
    for (size_t i = 0; i < frames.size(); i++) {
        frames[i] = static_cast<int>(i);
    }
    return frames;
}

} // namespace

TEST(SampleSongTest, CanonInDIsValidAndCanonical)
{
    expectValidAndCanonical(CANON_IN_D);
}

TEST(SampleSongTest, CanonInDPlaysEveryFrameAndStopsAfterTheLastChord)
{
    PlayedSong played;
    ASSERT_NO_FATAL_FAILURE(playUntilStop(CANON_IN_D, played));

    EXPECT_EQ(played.frames, allFrames(*played.song));

    // The last frame holds the final half note (16 rows), then the song stops
    EXPECT_EQ(played.lastPlayed.frame, played.song->frames.size() - 1);
    EXPECT_EQ(played.lastPlayed.row, 15);
}

TEST(SampleSongTest, AirOnTheGStringIsValidAndCanonical)
{
    expectValidAndCanonical(AIR_ON_THE_G_STRING);
}

TEST(SampleSongTest, AirOnTheGStringPlaysBothHalvesTwiceAndStopsAfterTheLastChord)
{
    PlayedSong played;
    ASSERT_NO_FATAL_FAILURE(playUntilStop(AIR_ON_THE_G_STRING, played));

    // Each half is repeated by listing its frames twice, so every frame plays once in order
    EXPECT_EQ(played.frames, allFrames(*played.song));
    ASSERT_EQ(played.song->frames.size(), 18u);

    // The last frame holds the final whole note to the end of the pattern, then the song stops
    EXPECT_EQ(played.lastPlayed.frame, played.song->frames.size() - 1);
    EXPECT_EQ(played.lastPlayed.row, 63);
}
