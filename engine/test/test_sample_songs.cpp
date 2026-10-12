// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>

#include "audio/tracker/SongPlayer.h"

#include <ocf/audio/tracker/SongSerializer.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

using namespace ocf::audio::tracker;

namespace {

const std::filesystem::path CANON_IN_D =
    std::filesystem::path(OCF_TEST_ASSETS_DIR) / "songs" / "canon_in_d.json";

/** With LF line endings, as the serializer writes, even if the checkout converted them. */
std::string readFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::erase(text, '\r');
    return text;
}

} // namespace

TEST(SampleSongTest, CanonInDIsValidAndCanonical)
{
    const std::string json = readFile(CANON_IN_D);
    ASSERT_FALSE(json.empty()) << CANON_IN_D;

    const auto result = loadSongFromString(json);
    ASSERT_TRUE(result.isOk()) << result.unwrapErr();

    // Already in the layout the serializer writes, so saving it from the tracker gives no diff
    EXPECT_EQ(saveSongToString(result.unwrap()), json);
}

TEST(SampleSongTest, CanonInDPlaysEveryFrameAndStopsAfterTheLastChord)
{
    auto result = loadSong(CANON_IN_D);
    ASSERT_TRUE(result.isOk()) << result.unwrapErr();
    const auto song = std::make_shared<const Song>(std::move(result).unwrap());

    SongPlayer player([](uint16_t, uint8_t) {});
    player.setSong(song);
    player.play(0, 0);

    std::vector<int> frames;
    PlaybackStatus lastPlayed;
    for (int tick = 0; tick == 0 || player.getStatus().playing; tick++) {
        ASSERT_LT(tick, 100000) << "the song does not stop";
        player.tick();
        const PlaybackStatus status = player.getStatus();
        if (!status.playing) {
            break;
        }
        lastPlayed = status;
        if (frames.empty() || frames.back() != status.frame) {
            frames.push_back(status.frame);
        }
    }

    std::vector<int> expectedFrames(song->frames.size());
    for (size_t i = 0; i < expectedFrames.size(); i++) {
        expectedFrames[i] = static_cast<int>(i);
    }
    EXPECT_EQ(frames, expectedFrames);

    // The last frame holds the final half note (16 rows), then the song stops
    EXPECT_EQ(lastPlayed.frame, song->frames.size() - 1);
    EXPECT_EQ(lastPlayed.row, 15);
}
