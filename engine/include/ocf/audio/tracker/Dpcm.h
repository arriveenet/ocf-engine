// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

/**
 * Samples of the APU's DMC (delta modulation) channel. A sample is a stream of bits, read from the
 * lowest bit of each byte. Each bit moves the 7-bit output level up (1) or down (0) by 2.
 * See https://www.nesdev.org/wiki/APU_DMC and docs/dmc-converter-design.md.
 */
namespace ocf::audio::tracker {

constexpr size_t DPCM_RATE_COUNT = 16;

/** CPU cycles per bit of each rate ($4010 bits 0-3), NTSC. */
constexpr std::array<uint16_t, DPCM_RATE_COUNT> DPCM_RATE_PERIODS = {
    428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54,
};

constexpr uint8_t DPCM_MAX_RATE = DPCM_RATE_COUNT - 1;
constexpr size_t DPCM_MAX_SAMPLE_BYTES = 4081; // $4013 = FF: FF * 16 + 1
constexpr size_t DPCM_MEMORY_SIZE = 0x4000;    // $C000-$FFFF
constexpr size_t DPCM_SAMPLE_ALIGNMENT = 64;   // $4012 = $C000 + A * 64
constexpr uint8_t DPCM_LEVEL_MAX = 127;
constexpr uint8_t DPCM_DEFAULT_LEVEL = 64;

/** Bits per second of the rate (0-15). */
double getDpcmRateFrequency(uint8_t rate);

/** Rounds up to a length that $4013 can play: 16n + 1 bytes. */
size_t getDpcmPlayableLength(size_t bytes);

/** The memory a sample takes: its length rounded up to the alignment. */
constexpr size_t getDpcmAlignedSize(size_t bytes)
{
    return (bytes + DPCM_SAMPLE_ALIGNMENT - 1) / DPCM_SAMPLE_ALIGNMENT * DPCM_SAMPLE_ALIGNMENT;
}

/** One bit of the output unit, as the APU does it: +2 for 1, -2 for 0, unchanged at the limits. */
constexpr uint8_t stepDpcmLevel(uint8_t level, bool bit)
{
    if (bit) {
        return (level <= DPCM_LEVEL_MAX - 2) ? static_cast<uint8_t>(level + 2) : level;
    }
    return (level >= 2) ? static_cast<uint8_t>(level - 2) : level;
}

/** The output level after each bit, lowest bit of each byte first. */
std::vector<uint8_t> decodeDpcm(std::span<const uint8_t> data, uint8_t startLevel);

} // namespace ocf::audio::tracker
