// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>

#include "ocf/audio/AudioTrackerSource.h"

#include <cmath>
#include <vector>

using namespace ocf::audio;
using namespace ocf::audio::tracker;

namespace {

constexpr uint8_t A4 = 58;

std::vector<float> render(AudioTrackerSource& source, uint32_t frameCount)
{
    std::vector<float> output(frameCount * InternalChannels);
    source.render(output.data(), frameCount, InternalChannels);
    return output;
}

/** RMS of the first channel over [begin, end) frames. */
float rms(const std::vector<float>& samples, size_t begin, size_t end)
{
    double sum = 0.0;
    for (size_t i = begin; i < end; i++) {
        const double sample = samples[i * InternalChannels];
        sum += sample * sample;
    }
    return static_cast<float>(std::sqrt(sum / static_cast<double>(end - begin)));
}

int countZeroCrossings(const std::vector<float>& samples, size_t begin, size_t end)
{
    int count = 0;
    for (size_t i = begin + 1; i < end; i++) {
        const bool previous = samples[(i - 1) * InternalChannels] >= 0.0f;
        const bool current = samples[i * InternalChannels] >= 0.0f;
        if (previous != current) {
            count++;
        }
    }
    return count;
}

std::shared_ptr<const Song> createSong(ChannelType channel, uint8_t note)
{
    Song song = Song::createDefault();
    song.getPattern(channel, 0).rows[0].note = note;
    return std::make_shared<const Song>(song);
}

} // namespace

TEST(AudioTrackerSourceTest, SilentUntilPlayed)
{
    AudioTrackerSource source;
    source.setSong(createSong(ChannelType::Pulse1, A4));
    const std::vector<float> output = render(source, InternalSampleRate / 10);
    EXPECT_LT(rms(output, 0, InternalSampleRate / 10), 1e-4f);
    EXPECT_FALSE(source.getStatus().playing);
}

TEST(AudioTrackerSourceTest, PlaysPulseAtThePitchOfTheNote)
{
    AudioTrackerSource source;
    source.setSong(createSong(ChannelType::Pulse1, A4));
    source.playSong(0, 0);

    // 0.1 s after the first tick: about 44 cycles of 440 Hz, two zero crossings each
    const uint32_t frameCount = InternalSampleRate / 5;
    const std::vector<float> output = render(source, frameCount);
    EXPECT_TRUE(source.getStatus().playing);
    EXPECT_GT(rms(output, frameCount / 2, frameCount), 0.01f);
    EXPECT_NEAR(countZeroCrossings(output, frameCount / 2, frameCount), 88, 4);

    // Both channels carry the same sample
    for (size_t i = 0; i < output.size(); i += InternalChannels) {
        ASSERT_EQ(output[i], output[i + 1]);
    }
}

TEST(AudioTrackerSourceTest, PlaysTriangle)
{
    AudioTrackerSource source;
    source.setSong(createSong(ChannelType::Triangle, A4));
    source.playSong(0, 0);

    const uint32_t frameCount = InternalSampleRate / 5;
    const std::vector<float> output = render(source, frameCount);
    EXPECT_GT(rms(output, frameCount / 2, frameCount), 0.01f);
    EXPECT_NEAR(countZeroCrossings(output, frameCount / 2, frameCount), 88, 4);
}

TEST(AudioTrackerSourceTest, StopSilences)
{
    AudioTrackerSource source;
    source.setSong(createSong(ChannelType::Pulse1, A4));
    source.playSong(0, 0);
    render(source, InternalSampleRate / 10);

    source.stopSong();
    const uint32_t frameCount = InternalSampleRate / 10;
    const std::vector<float> output = render(source, frameCount);
    EXPECT_FALSE(source.getStatus().playing);
    // The high-pass filter settles within a few milliseconds
    EXPECT_LT(rms(output, frameCount / 2, frameCount), 1e-3f);
}

TEST(AudioTrackerSourceTest, PreviewNoteWithoutSong)
{
    AudioTrackerSource source;
    source.playNote(ChannelType::Pulse2, A4, VALUE_NONE);
    const uint32_t frameCount = InternalSampleRate / 10;
    const std::vector<float> output = render(source, frameCount);
    EXPECT_GT(rms(output, frameCount / 2, frameCount), 0.01f);

    source.stopNote(ChannelType::Pulse2);
    const std::vector<float> stopped = render(source, frameCount);
    EXPECT_LT(rms(stopped, frameCount / 2, frameCount), 1e-3f);
}

TEST(AudioTrackerSourceTest, ReleasesReplacedSongOnUpdate)
{
    AudioTrackerSource source;
    auto first = createSong(ChannelType::Pulse1, A4);
    std::weak_ptr<const Song> weakFirst = first;
    source.setSong(std::move(first));
    render(source, 64);
    source.setSong(createSong(ChannelType::Pulse1, A4));
    render(source, 64);

    // Kept alive until update(), so that it is not freed on the audio thread
    EXPECT_FALSE(weakFirst.expired());
    source.update();
    EXPECT_TRUE(weakFirst.expired());
}

TEST(AudioTrackerSourceTest, PlaysDpcm)
{
    // A square wave of 16 levels: 8 bits up, 8 bits down
    Song song = Song::createDefault();
    song.dpcmSamples.resize(1);
    for (size_t i = 0; i < DPCM_MAX_SAMPLE_BYTES; i++) {
        song.dpcmSamples[0].data.push_back((i % 2 == 0) ? 0xFF : 0x00);
    }
    *song.instruments[0].findDpcmKey(A4) = DpcmKey{0, DPCM_MAX_RATE, false};
    song.getPattern(ChannelType::Dpcm, 0).rows[0] = Cell{A4, 0, VALUE_NONE, {}};

    AudioTrackerSource source;
    source.setSong(std::make_shared<const Song>(song));
    source.playSong(0, 0);

    // 33144 / 16 = about 2071 Hz
    const uint32_t frameCount = InternalSampleRate / 5;
    const std::vector<float> output = render(source, frameCount);
    EXPECT_GT(rms(output, frameCount / 2, frameCount), 0.01f);
    EXPECT_NEAR(countZeroCrossings(output, frameCount / 2, frameCount), 2 * 2071 / 10, 20);

    source.stopSong();
    render(source, InternalSampleRate / 10); // The buffered bits, and the high-pass filter settle
    const std::vector<float> stopped = render(source, InternalSampleRate / 10);
    EXPECT_LT(rms(stopped, 0, InternalSampleRate / 10), 1e-3f);
}
