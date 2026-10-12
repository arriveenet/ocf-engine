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

TEST(SampleSongTest, CanonInDLoopsBackToTheSecondFrame)
{
    auto result = loadSong(CANON_IN_D);
    ASSERT_TRUE(result.isOk()) << result.unwrapErr();
    const auto song = std::make_shared<const Song>(std::move(result).unwrap());

    SongPlayer player([](uint16_t, uint8_t) {});
    player.setSong(song);
    player.play(0, 0);

    // Seven frames: the six frames, then the jump back to the second one
    const int ticksPerRow = 150 * song->speed / song->tempo;
    const int ticks = 7 * song->rowsPerPattern * ticksPerRow;
    std::vector<int> frames;
    for (int tick = 0; tick < ticks; tick++) {
        player.tick();
        const PlaybackStatus status = player.getStatus();
        ASSERT_TRUE(status.playing);
        if (frames.empty() || frames.back() != status.frame) {
            frames.push_back(status.frame);
        }
    }
    EXPECT_EQ(frames, (std::vector<int>{0, 1, 2, 3, 4, 5, 2}));
}
