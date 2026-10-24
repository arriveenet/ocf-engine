// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>

#include "ocf/audio/AudioMixer.h"
#include "ocf/audio/AudioSource.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace ocf::audio;

namespace {

constexpr uint32_t FRAME_COUNT = 64;
constexpr uint32_t CHANNELS = 2;

/** Writes a constant value, or nothing when the value is negative. */
class TestSource : public AudioSource {
public:
    explicit TestSource(float value, std::chrono::milliseconds renderTime = {})
        : m_value(value)
        , m_renderTime(renderTime)
    {
        m_state = AudioState::Playing;
    }

    void render(float* output, uint32_t frameCount, uint32_t channels) override
    {
        m_rendering = true;
        if (m_renderTime.count() > 0) {
            std::this_thread::sleep_for(m_renderTime);
        }
        if (m_value >= 0.0f) {
            std::fill(output, output + frameCount * channels, m_value);
        }
        m_renderCount++;
        m_rendering = false;
    }

    void setState(AudioState state) { m_state = state; }

    std::atomic<bool> m_rendering = false;
    std::atomic<int> m_renderCount = 0;

private:
    float m_value;
    std::chrono::milliseconds m_renderTime;
};

std::vector<float> render(AudioMixer& mixer)
{
    std::vector<float> output(FRAME_COUNT * CHANNELS, -1.0f);
    mixer.render(output.data(), FRAME_COUNT, CHANNELS);
    return output;
}

} // namespace

TEST(AudioMixerTest, MixesSourcesWithVolume)
{
    AudioMixer mixer;
    TestSource first(0.25f);
    TestSource second(0.5f);
    second.setVolume(0.5f);
    mixer.addSource(&first);
    mixer.addSource(&second);

    for (float sample : render(mixer)) {
        ASSERT_FLOAT_EQ(sample, 0.5f);
    }
}

TEST(AudioMixerTest, OutputIsSilentWithoutSources)
{
    AudioMixer mixer;
    for (float sample : render(mixer)) {
        ASSERT_EQ(sample, 0.0f);
    }
}

TEST(AudioMixerTest, SourceThatWritesNothingAddsNothing)
{
    AudioMixer mixer;
    TestSource playing(0.5f);
    TestSource silent(-1.0f);
    mixer.addSource(&playing);
    mixer.addSource(&silent);

    for (float sample : render(mixer)) {
        ASSERT_FLOAT_EQ(sample, 0.5f);
    }
}

TEST(AudioMixerTest, RemovesStoppedSourcesWithoutSkippingOthers)
{
    AudioMixer mixer;
    TestSource stopped(0.4f);
    TestSource first(0.1f);
    TestSource second(0.2f);
    stopped.setState(AudioState::Stopped);
    mixer.addSource(&stopped);
    mixer.addSource(&first);
    mixer.addSource(&second);

    for (float sample : render(mixer)) {
        ASSERT_FLOAT_EQ(sample, 0.3f);
    }
    EXPECT_EQ(stopped.m_renderCount, 0);

    // It was removed, so it stays silent even when it plays again
    stopped.setState(AudioState::Playing);
    for (float sample : render(mixer)) {
        ASSERT_FLOAT_EQ(sample, 0.3f);
    }
}

TEST(AudioMixerTest, AddSourceTwiceMixesOnce)
{
    AudioMixer mixer;
    TestSource source(0.25f);
    mixer.addSource(&source);
    mixer.addSource(&source);

    for (float sample : render(mixer)) {
        ASSERT_FLOAT_EQ(sample, 0.25f);
    }
}

TEST(AudioMixerTest, RemoveSourceWaitsUntilTheSourceIsNotRendered)
{
    AudioMixer mixer;
    TestSource source(0.25f, std::chrono::milliseconds(20));
    mixer.addSource(&source);

    std::atomic<bool> running = true;
    std::thread audioThread([&] {
        std::vector<float> output(FRAME_COUNT * CHANNELS);
        while (running) {
            mixer.render(output.data(), FRAME_COUNT, CHANNELS);
        }
    });

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!source.m_rendering && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
    }
    ASSERT_TRUE(source.m_rendering);

    // Removing while the audio thread renders the source: the source may be deleted afterwards
    mixer.removeSource(&source);
    EXPECT_FALSE(source.m_rendering);

    const int renderCount = source.m_renderCount;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(source.m_renderCount, renderCount);

    running = false;
    audioThread.join();
}
