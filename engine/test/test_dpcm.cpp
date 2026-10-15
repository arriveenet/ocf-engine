// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>
#include <ocf/audio/tracker/Dpcm.h>
#include <ocf/audio/tracker/DpcmEncoder.h>

#include <cmath>
#include <numbers>
#include <vector>

using namespace ocf::audio::tracker;

namespace {

std::vector<float> makeSine(double frequency, double amplitude, uint32_t sampleRate, double seconds)
{
    std::vector<float> samples(static_cast<size_t>(sampleRate * seconds));
    for (size_t i = 0; i < samples.size(); i++) {
        samples[i] = static_cast<float>(
            amplitude * std::sin(2.0 * std::numbers::pi * frequency * i / sampleRate));
    }
    return samples;
}

DpcmEncodeResult encode(const std::vector<float>& samples, uint32_t sampleRate,
                        const DpcmEncodeOptions& options = {})
{
    auto result = DpcmEncoder::encode(samples, sampleRate, options);
    EXPECT_TRUE(result.isOk()) << (result.isErr() ? result.unwrapErr() : "");
    return result.isOk() ? std::move(result).unwrap() : DpcmEncodeResult{};
}

bool isPlayableLength(size_t bytes)
{
    return bytes >= 1 && bytes <= DPCM_MAX_SAMPLE_BYTES && (bytes - 1) % 16 == 0;
}

} // namespace

TEST(DpcmTest, StepLevel)
{
    EXPECT_EQ(stepDpcmLevel(64, true), 66);
    EXPECT_EQ(stepDpcmLevel(64, false), 62);
    EXPECT_EQ(stepDpcmLevel(125, true), 127);
    EXPECT_EQ(stepDpcmLevel(126, true), 126);
    EXPECT_EQ(stepDpcmLevel(127, true), 127);
    EXPECT_EQ(stepDpcmLevel(2, false), 0);
    EXPECT_EQ(stepDpcmLevel(1, false), 1);
    EXPECT_EQ(stepDpcmLevel(0, false), 0);
}

TEST(DpcmTest, DecodeReadsTheLowestBitFirst)
{
    const std::vector<uint8_t> data = {0x01};
    const std::vector<uint8_t> levels = decodeDpcm(data, 64);
    ASSERT_EQ(levels.size(), 8u);
    EXPECT_EQ(levels[0], 66);
    for (size_t i = 1; i < levels.size(); i++) {
        EXPECT_EQ(levels[i], 66 - 2 * static_cast<int>(i));
    }
}

TEST(DpcmTest, PlayableLength)
{
    EXPECT_EQ(getDpcmPlayableLength(0), 1u);
    EXPECT_EQ(getDpcmPlayableLength(1), 1u);
    EXPECT_EQ(getDpcmPlayableLength(2), 17u);
    EXPECT_EQ(getDpcmPlayableLength(17), 17u);
    EXPECT_EQ(getDpcmPlayableLength(18), 33u);
    EXPECT_EQ(getDpcmPlayableLength(DPCM_MAX_SAMPLE_BYTES), DPCM_MAX_SAMPLE_BYTES);
}

TEST(DpcmTest, RateFrequency)
{
    EXPECT_NEAR(getDpcmRateFrequency(0), 4181.7, 0.1);
    EXPECT_NEAR(getDpcmRateFrequency(15), 33143.9, 0.1);
}

TEST(DpcmEncoderTest, PackBitsPadsWithAlternatingBits)
{
    const std::vector<uint8_t> bits = {1, 1, 0};
    const std::vector<uint8_t> data = DpcmEncoder::packBits(bits);
    ASSERT_EQ(data.size(), 1u);
    // 1, 1, 0, then 1, 0, 1, 0, 1
    EXPECT_EQ(data[0], 0b10101011);

    const std::vector<uint8_t> moreBits(9, 1);
    const std::vector<uint8_t> padded = DpcmEncoder::packBits(moreBits);
    ASSERT_EQ(padded.size(), 17u);
    EXPECT_EQ(padded[0], 0xFF);
    EXPECT_EQ(padded[1], 0b10101011);
    EXPECT_EQ(padded[2], 0b10101010); // The alternation continues across bytes
}

TEST(DpcmEncoderTest, FindNonSilentRange)
{
    const std::vector<float> samples = {0.0f, 0.001f, 0.5f, 0.0f, -0.5f, 0.001f, 0.0f};
    EXPECT_EQ(DpcmEncoder::findNonSilentRange(samples, -40.0f),
              std::make_pair(size_t{2}, size_t{5}));
    EXPECT_EQ(DpcmEncoder::findNonSilentRange(samples, 0.0f), std::make_pair(size_t{0}, size_t{0}));
}

TEST(DpcmEncoderTest, ResampleKeepsLowFrequencies)
{
    const std::vector<float> input = makeSine(200.0, 0.5, 44100, 0.1);
    const double toRate = getDpcmRateFrequency(15);
    const std::vector<float> output = DpcmEncoder::resample(input, 44100, toRate);
    ASSERT_EQ(output.size(), static_cast<size_t>(std::ceil(input.size() * toRate / 44100)));
    // Away from the edges, the output is the same sine
    for (size_t i = 200; i < output.size() - 200; i++) {
        const double expected = 0.5 * std::sin(2.0 * std::numbers::pi * 200.0 * i / toRate);
        ASSERT_NEAR(output[i], expected, 0.01) << i;
    }
}

TEST(DpcmEncoderTest, ResampleRemovesFrequenciesAboveTheNyquistFrequency)
{
    // 10kHz cannot be played at about 4.2kHz, and must not alias
    const std::vector<float> input = makeSine(10000.0, 0.5, 44100, 0.1);
    const std::vector<float> output = DpcmEncoder::resample(input, 44100, getDpcmRateFrequency(0));
    for (size_t i = 100; i < output.size() - 100; i++) {
        ASSERT_NEAR(output[i], 0.0f, 0.01f) << i;
    }
}

TEST(DpcmEncoderTest, SilenceStaysAtTheRestLevel)
{
    const std::vector<float> silence(4410, 0.0f);
    for (DpcmEncodeMethod method : {DpcmEncodeMethod::Greedy, DpcmEncodeMethod::Optimal}) {
        DpcmEncodeOptions options;
        options.method = method;
        const DpcmEncodeResult result = encode(silence, 44100, options);
        EXPECT_TRUE(isPlayableLength(result.data.size()));
        for (uint8_t level : decodeDpcm(result.data, options.restLevel)) {
            ASSERT_GE(level, options.restLevel - 2);
            ASSERT_LE(level, options.restLevel + 2);
        }
    }
}

TEST(DpcmEncoderTest, QuietLowSineIsAccurate)
{
    // Amplitude 32 levels at 100Hz is well within the slope limit of rate 15 (330Hz)
    const std::vector<float> sine = makeSine(100.0, 32.0 / 63.0, 44100, 0.5);
    for (DpcmEncodeMethod method : {DpcmEncodeMethod::Greedy, DpcmEncodeMethod::Optimal}) {
        DpcmEncodeOptions options;
        options.method = method;
        const DpcmEncodeResult result = encode(sine, 44100, options);
        EXPECT_EQ(result.rate, 15);
        EXPECT_FALSE(result.truncated);
        EXPECT_TRUE(isPlayableLength(result.data.size()));
        EXPECT_GT(result.snrDb, 20.0);
        EXPECT_EQ(result.overloadRatio, 0.0);
    }
}

TEST(DpcmEncoderTest, OptimalIsNoWorseThanGreedy)
{
    // Loud and high: beyond the slope limit
    const std::vector<float> sine = makeSine(800.0, 0.9, 44100, 0.3);
    DpcmEncodeOptions options;
    options.method = DpcmEncodeMethod::Greedy;
    const DpcmEncodeResult greedy = encode(sine, 44100, options);
    options.method = DpcmEncodeMethod::Optimal;
    const DpcmEncodeResult optimal = encode(sine, 44100, options);
    EXPECT_GT(greedy.overloadRatio, 0.0);
    EXPECT_GE(optimal.snrDb, greedy.snrDb);
}

TEST(DpcmEncoderTest, ReturnsToTheRestLevel)
{
    // Ends at the top of a loud wave
    std::vector<float> rising(2000);
    for (size_t i = 0; i < rising.size(); i++) {
        rising[i] = static_cast<float>(i) / rising.size();
    }
    DpcmEncodeOptions options;
    options.rate = 0;
    options.restLevel = 40;

    const DpcmEncodeResult returned = encode(rising, 44100, options);
    const std::vector<uint8_t> levels = decodeDpcm(returned.data, options.restLevel);
    EXPECT_LE(std::abs(levels.back() - options.restLevel), 2);

    options.returnToRest = false;
    const DpcmEncodeResult kept = encode(rising, 44100, options);
    // Not 127: the resampler fades the abrupt end
    EXPECT_GT(decodeDpcm(kept.data, options.restLevel).back(), 80);
}

TEST(DpcmEncoderTest, TooLongIsAnErrorUnlessTruncated)
{
    // 1.5 seconds does not fit at rate 15 (0.98 seconds)
    const std::vector<float> sine = makeSine(100.0, 0.3, 22050, 1.5);
    DpcmEncodeOptions options;
    auto error = DpcmEncoder::encode(sine, 22050, options);
    ASSERT_TRUE(error.isErr());
    EXPECT_NE(error.unwrapErr().find("Rate 13 fits"), std::string::npos) << error.unwrapErr();

    options.truncate = true;
    const DpcmEncodeResult truncated = encode(sine, 22050, options);
    EXPECT_TRUE(truncated.truncated);
    EXPECT_EQ(truncated.data.size(), DPCM_MAX_SAMPLE_BYTES);
    const std::vector<uint8_t> levels = decodeDpcm(truncated.data, options.restLevel);
    EXPECT_LE(std::abs(levels.back() - options.restLevel), 2);
}

TEST(DpcmEncoderTest, AutoRateChoosesTheHighestRateThatFits)
{
    const std::vector<float> sine = makeSine(100.0, 0.3, 22050, 1.5);
    DpcmEncodeOptions options;
    options.rate = std::nullopt;
    const DpcmEncodeResult result = encode(sine, 22050, options);
    EXPECT_EQ(result.rate, 13); // 1.53 seconds
    EXPECT_FALSE(result.truncated);

    // Too long for any rate
    const std::vector<float> longSine = makeSine(100.0, 0.3, 8000, 10.0);
    EXPECT_TRUE(DpcmEncoder::encode(longSine, 8000, options).isErr());
    options.truncate = true;
    const DpcmEncodeResult truncated = encode(longSine, 8000, options);
    EXPECT_EQ(truncated.rate, 0);
    EXPECT_TRUE(truncated.truncated);
}

TEST(DpcmEncoderTest, TrimAndNormalize)
{
    std::vector<float> samples(1000, 0.0f);
    const std::vector<float> sine = makeSine(100.0, 0.1, 44100, 0.1);
    samples.insert(samples.end(), sine.begin(), sine.end());
    samples.resize(samples.size() + 1000, 0.0f);

    DpcmEncodeOptions options;
    const size_t untrimmed = encode(samples, 44100, options).data.size();
    options.trimThresholdDb = -50.0f;
    const size_t trimmed = encode(samples, 44100, options).data.size();
    EXPECT_LT(trimmed, untrimmed);

    // Normalized to full scale: the levels reach near the limits
    options.normalize = true;
    options.method = DpcmEncodeMethod::Optimal;
    const DpcmEncodeResult result = encode(samples, 44100, options);
    const std::vector<uint8_t> levels = decodeDpcm(result.data, options.restLevel);
    EXPECT_GE(*std::max_element(levels.begin(), levels.end()), 120);
    EXPECT_LE(*std::min_element(levels.begin(), levels.end()), 8);

    EXPECT_TRUE(
        DpcmEncoder::encode(std::vector<float>(100, 0.0f), 44100, options).isErr()); // Silent
}

TEST(DpcmEncoderTest, InvalidInput)
{
    const std::vector<float> samples(100, 0.0f);
    EXPECT_TRUE(DpcmEncoder::encode({}, 44100, {}).isErr());
    EXPECT_TRUE(DpcmEncoder::encode(samples, 0, {}).isErr());
    DpcmEncodeOptions options;
    options.rate = 16;
    EXPECT_TRUE(DpcmEncoder::encode(samples, 44100, options).isErr());
    options = {};
    options.restLevel = 128;
    EXPECT_TRUE(DpcmEncoder::encode(samples, 44100, options).isErr());
    options = {};
    options.gain = -1.0f;
    EXPECT_TRUE(DpcmEncoder::encode(samples, 44100, options).isErr());
}
