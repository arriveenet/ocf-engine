// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/audio/tracker/Dpcm.h"
#include "ocf/core/Result.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

/**
 * Converts a waveform to a DMC sample. See docs/dmc-converter-design.md.
 */
namespace ocf::audio::tracker {

enum class DpcmEncodeMethod : uint8_t {
    Greedy,  // Each bit moves toward the target, as FamiTracker does
    Optimal, // The bits with the least squared error over the whole sample (Viterbi search)
};

struct DpcmEncodeOptions {
    std::optional<uint8_t> rate = DPCM_MAX_RATE; // nullopt: the highest rate that fits
    bool normalize = false;                      // Scales the peak to 1.0, before the gain
    float gain = 1.0f;                           // 1.0 is an amplitude of 63 levels
    std::optional<float> trimThresholdDb;        // Trims the quieter ends (dBFS)
    uint8_t center = DPCM_DEFAULT_LEVEL;         // The level of 0.0
    uint8_t restLevel = DPCM_DEFAULT_LEVEL;      // The level at the start and the end
    bool returnToRest = true;                    // Adds bits that bring the level back at the end
    DpcmEncodeMethod method = DpcmEncodeMethod::Optimal;
    bool truncate = false; // Otherwise a sample longer than DPCM_MAX_SAMPLE_BYTES is an error
};

struct DpcmEncodeResult {
    std::vector<uint8_t> data; // 16n + 1 bytes
    uint8_t rate = 0;
    bool truncated = false;
    double snrDb = 0.0;         // Of the played levels against the targets. Infinite when exact
    double overloadRatio = 0.0; // Bits that ended more than one step away from the target
};

/** Converts mono samples (-1.0 to 1.0) at the sample rate. */
Result<DpcmEncodeResult, std::string> encodeDpcm(std::span<const float> samples, uint32_t sampleRate,
                                                 const DpcmEncodeOptions& options);

/** The steps of encodeDpcm(), for tests. */
namespace detail {

/** The range [first, last) between the first and the last samples above the threshold. */
std::pair<size_t, size_t> findNonSilentRange(std::span<const float> samples, float thresholdDb);

/** Windowed sinc, low-passed below both Nyquist frequencies. */
std::vector<float> resample(std::span<const float> samples, double fromRate, double toRate);

/** One bit (0 or 1) per target level, starting from the level. */
std::vector<uint8_t> encodeBits(std::span<const float> targets, uint8_t startLevel,
                                DpcmEncodeMethod method);

/** Packs the bits, lowest bit first, and pads them to a playable length. */
std::vector<uint8_t> packBits(std::span<const uint8_t> bits);

} // namespace detail

} // namespace ocf::audio::tracker
