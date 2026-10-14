// SPDX-License-Identifier: MIT
#include "ocf/audio/tracker/DpcmEncoder.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <limits>
#include <numbers>

namespace ocf::audio::tracker {

namespace {

using EncodeResult = Result<DpcmEncodeResult, std::string>;

constexpr size_t MAX_BITS = DPCM_MAX_SAMPLE_BYTES * 8;
constexpr int LEVEL_COUNT = DPCM_LEVEL_MAX + 1;
constexpr float FULL_SCALE_LEVELS = 63.0f; // The amplitude of 1.0

// The low-pass filter of the resampler
constexpr double CUTOFF_RATIO = 0.45;  // Of the lower Nyquist frequency times 2
constexpr double ZERO_CROSSINGS = 16.0; // On each side of the kernel

double sinc(double x)
{
    if (std::abs(x) < 1e-9) {
        return 1.0;
    }
    const double angle = std::numbers::pi * x;
    return std::sin(angle) / angle;
}

/** The cutoff of the resampler's low-pass filter, in cycles per input sample. */
double getCutoff(double fromRate, double toRate)
{
    return CUTOFF_RATIO * std::min(fromRate, toRate) / fromRate;
}

/** In input samples. */
double getKernelHalfWidth(double fromRate, double toRate)
{
    return ZERO_CROSSINGS / (2.0 * getCutoff(fromRate, toRate));
}

/** u in [-1, 1]. */
double blackman(double u)
{
    return 0.42 + 0.5 * std::cos(std::numbers::pi * u) + 0.08 * std::cos(2.0 * std::numbers::pi * u);
}

std::vector<uint8_t> encodeGreedy(std::span<const float> targets, uint8_t startLevel)
{
    std::vector<uint8_t> bits;
    bits.reserve(targets.size());
    uint8_t level = startLevel;
    bool previous = false;
    for (float target : targets) {
        const float upError = std::abs(stepDpcmLevel(level, true) - target);
        const float downError = std::abs(stepDpcmLevel(level, false) - target);
        // On a tie, alternate to hold the level
        const bool bit = (upError == downError) ? !previous : (upError < downError);
        level = stepDpcmLevel(level, bit);
        bits.push_back(bit);
        previous = bit;
    }
    return bits;
}

/** Viterbi search over the 128 levels: the path with the least total squared error. */
std::vector<uint8_t> encodeOptimal(std::span<const float> targets, uint8_t startLevel)
{
    constexpr double INF = std::numeric_limits<double>::infinity();
    constexpr uint8_t BIT_FLAG = 0x80;

    std::array<double, LEVEL_COUNT> cost;
    cost.fill(INF);
    cost[startLevel] = 0.0;

    // For each bit and each level: the level before it, with the bit in BIT_FLAG
    std::vector<std::array<uint8_t, LEVEL_COUNT>> previous(targets.size());
    for (size_t i = 0; i < targets.size(); i++) {
        std::array<double, LEVEL_COUNT> next;
        next.fill(INF);
        for (int level = 0; level < LEVEL_COUNT; level++) {
            if (cost[level] == INF) {
                continue;
            }
            for (bool bit : {false, true}) {
                const uint8_t to = stepDpcmLevel(static_cast<uint8_t>(level), bit);
                const double error = to - targets[i];
                const double total = cost[level] + error * error;
                if (total < next[to]) {
                    next[to] = total;
                    previous[i][to] = static_cast<uint8_t>(level | (bit ? BIT_FLAG : 0));
                }
            }
        }
        cost = next;
    }

    int level = static_cast<int>(std::min_element(cost.begin(), cost.end()) - cost.begin());
    std::vector<uint8_t> bits(targets.size());
    for (size_t i = targets.size(); i-- > 0;) {
        bits[i] = (previous[i][level] & BIT_FLAG) ? 1 : 0;
        level = previous[i][level] & ~BIT_FLAG;
    }
    return bits;
}

/** Encodes at one rate. nullopt when the sample is too long and must not be truncated. */
std::optional<DpcmEncodeResult> encodeAtRate(std::span<const float> input, uint32_t sampleRate,
                                             uint8_t rate, float scale,
                                             const DpcmEncodeOptions& options, bool truncate)
{
    const double frequency = getDpcmRateFrequency(rate);
    const double estimatedBits = std::ceil(input.size() * frequency / sampleRate);
    if (estimatedBits > MAX_BITS) {
        if (!truncate) {
            return std::nullopt;
        }
        // Resample only what fits, and what the resampler's kernel reaches beyond it
        const double needed =
            MAX_BITS * sampleRate / frequency + getKernelHalfWidth(sampleRate, frequency) + 1.0;
        input = input.first(std::min(input.size(), static_cast<size_t>(std::ceil(needed))));
    }

    std::vector<float> targets = detail::resample(input, sampleRate, frequency);
    for (float& target : targets) {
        target = std::clamp(options.center + target * scale * FULL_SCALE_LEVELS, 0.0f,
                            static_cast<float>(DPCM_LEVEL_MAX));
    }

    DpcmEncodeResult result;
    result.rate = rate;
    if (targets.size() > MAX_BITS) {
        if (!truncate) {
            return std::nullopt;
        }
        targets.resize(MAX_BITS);
        result.truncated = true;
    }

    const uint8_t rest = options.restLevel;
    std::vector<uint8_t> bits = detail::encodeBits(targets, rest, options.method);
    std::vector<uint8_t> levels;
    levels.reserve(bits.size());
    uint8_t level = rest;
    for (uint8_t bit : bits) {
        level = stepDpcmLevel(level, bit);
        levels.push_back(level);
    }

    if (options.returnToRest) {
        // The level keeps the parity of the rest level, so it gets back exactly
        auto getReturnBits = [&](size_t count) {
            const int end = (count == 0) ? rest : levels[count - 1];
            return static_cast<size_t>(std::abs(end - rest) / 2);
        };
        size_t kept = bits.size();
        while (kept > 0 && kept + getReturnBits(kept) > MAX_BITS) {
            kept--;
        }
        if (kept < bits.size()) {
            if (!truncate) {
                return std::nullopt;
            }
            bits.resize(kept);
            levels.resize(kept);
            targets.resize(kept);
            result.truncated = true;
        }
        level = levels.empty() ? rest : levels.back();
        while (level != rest) {
            const bool bit = level < rest;
            level = stepDpcmLevel(level, bit);
            bits.push_back(bit);
        }
    }
    result.data = detail::packBits(bits);

    double signal = 0.0;
    double noise = 0.0;
    size_t overloads = 0;
    for (size_t i = 0; i < targets.size(); i++) {
        const double error = levels[i] - targets[i];
        signal += (targets[i] - options.center) * (targets[i] - options.center);
        noise += error * error;
        if (std::abs(error) > 2.0) {
            overloads++;
        }
    }
    result.snrDb = (noise == 0.0) ? std::numeric_limits<double>::infinity()
                                  : 10.0 * std::log10(signal / noise);
    result.overloadRatio = targets.empty() ? 0.0 : static_cast<double>(overloads) / targets.size();
    return result;
}

} // namespace

namespace detail {

std::pair<size_t, size_t> findNonSilentRange(std::span<const float> samples, float thresholdDb)
{
    const float threshold = std::pow(10.0f, thresholdDb / 20.0f);
    auto isLoud = [threshold](float sample) { return std::abs(sample) > threshold; };
    const auto first = std::find_if(samples.begin(), samples.end(), isLoud);
    if (first == samples.end()) {
        return {0, 0};
    }
    const auto last = std::find_if(samples.rbegin(), samples.rend(), isLoud).base();
    return {static_cast<size_t>(first - samples.begin()),
            static_cast<size_t>(last - samples.begin())};
}

std::vector<float> resample(std::span<const float> samples, double fromRate, double toRate)
{
    if (samples.empty()) {
        return {};
    }

    const double step = fromRate / toRate; // Input samples per output sample
    const size_t count = std::max<size_t>(1, static_cast<size_t>(std::ceil(samples.size() / step)));
    const double cutoff = getCutoff(fromRate, toRate);
    const double halfWidth = getKernelHalfWidth(fromRate, toRate);
    const auto lastIndex = static_cast<ptrdiff_t>(samples.size()) - 1;

    std::vector<float> output(count);
    for (size_t j = 0; j < count; j++) {
        const double position = j * step;
        const auto first = std::max<ptrdiff_t>(0, static_cast<ptrdiff_t>(std::ceil(position - halfWidth)));
        const auto last =
            std::min<ptrdiff_t>(lastIndex, static_cast<ptrdiff_t>(std::floor(position + halfWidth)));
        double sum = 0.0;
        for (ptrdiff_t i = first; i <= last; i++) {
            const double distance = i - position;
            sum += samples[i] * 2.0 * cutoff * sinc(2.0 * cutoff * distance) *
                   blackman(distance / halfWidth);
        }
        output[j] = static_cast<float>(sum);
    }
    return output;
}

std::vector<uint8_t> encodeBits(std::span<const float> targets, uint8_t startLevel,
                                DpcmEncodeMethod method)
{
    startLevel = std::min(startLevel, DPCM_LEVEL_MAX);
    switch (method) {
    case DpcmEncodeMethod::Greedy: return encodeGreedy(targets, startLevel);
    default:                       return encodeOptimal(targets, startLevel);
    }
}

std::vector<uint8_t> packBits(std::span<const uint8_t> bits)
{
    std::vector<uint8_t> data(getDpcmPlayableLength((bits.size() + 7) / 8), 0);
    for (size_t i = 0; i < data.size() * 8; i++) {
        // The padding alternates, starting with 1, so that the level stays
        const bool bit = (i < bits.size()) ? bits[i] != 0 : ((i - bits.size()) % 2 == 0);
        if (bit) {
            data[i / 8] |= static_cast<uint8_t>(1 << (i % 8));
        }
    }
    return data;
}

} // namespace detail

Result<DpcmEncodeResult, std::string> encodeDpcm(std::span<const float> samples, uint32_t sampleRate,
                                                 const DpcmEncodeOptions& options)
{
    if (sampleRate == 0) {
        return EncodeResult::Err("invalid sample rate 0");
    }
    if (options.rate && *options.rate > DPCM_MAX_RATE) {
        return EncodeResult::Err(std::format("invalid rate {} (0-{})", *options.rate, DPCM_MAX_RATE));
    }
    if (options.center > DPCM_LEVEL_MAX || options.restLevel > DPCM_LEVEL_MAX) {
        return EncodeResult::Err(std::format("invalid level (0-{})", DPCM_LEVEL_MAX));
    }
    if (!std::isfinite(options.gain) || options.gain < 0.0f) {
        return EncodeResult::Err(std::format("invalid gain {}", options.gain));
    }
    if (samples.empty()) {
        return EncodeResult::Err("the input is empty");
    }

    std::span<const float> input = samples;
    if (options.trimThresholdDb) {
        const auto [first, last] = detail::findNonSilentRange(samples, *options.trimThresholdDb);
        if (first >= last) {
            return EncodeResult::Err(
                std::format("the input is silent below {} dBFS", *options.trimThresholdDb));
        }
        input = samples.subspan(first, last - first);
    }

    float scale = options.gain;
    if (options.normalize) {
        float peak = 0.0f;
        for (float sample : input) {
            peak = std::max(peak, std::abs(sample));
        }
        if (peak > 0.0f) {
            scale /= peak;
        }
    }

    // The highest rate that fits, from the requested rate down
    const int startRate = options.rate.value_or(DPCM_MAX_RATE);
    for (int rate = startRate; rate >= 0; rate--) {
        const bool truncate = options.truncate && options.rate.has_value();
        std::optional<DpcmEncodeResult> result =
            encodeAtRate(input, sampleRate, static_cast<uint8_t>(rate), scale, options, truncate);
        if (result) {
            return EncodeResult::Ok(std::move(*result));
        }
        if (!options.rate) {
            continue;
        }

        // Too long at the requested rate: suggest a rate that fits
        for (int lower = rate - 1; lower >= 0; lower--) {
            if (encodeAtRate(input, sampleRate, static_cast<uint8_t>(lower), scale, options, false)) {
                return EncodeResult::Err(std::format(
                    "too long for rate {} (max {} bytes). Rate {} fits, or truncate it", rate,
                    DPCM_MAX_SAMPLE_BYTES, lower));
            }
        }
        break;
    }

    if (options.truncate) {
        return EncodeResult::Ok(*encodeAtRate(input, sampleRate, 0, scale, options, true));
    }
    return EncodeResult::Err(
        std::format("too long even for rate 0 (max {} bytes, {:.2f} seconds). Truncate it",
                    DPCM_MAX_SAMPLE_BYTES, MAX_BITS / getDpcmRateFrequency(0)));
}

} // namespace ocf::audio::tracker
