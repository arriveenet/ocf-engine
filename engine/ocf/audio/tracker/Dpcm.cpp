// SPDX-License-Identifier: MIT
#include "ocf/audio/tracker/Dpcm.h"

#include "ocf/audio/AudioNesApu.h"

#include <algorithm>

namespace ocf::audio::tracker {

double getDpcmRateFrequency(uint8_t rate)
{
    return AudioNesApu::CPU_FREQUENCY / DPCM_RATE_PERIODS[std::min(rate, DPCM_MAX_RATE)];
}

size_t getDpcmPlayableLength(size_t bytes)
{
    if (bytes <= 1) {
        return 1;
    }
    return (bytes - 1 + 15) / 16 * 16 + 1;
}

std::vector<uint8_t> decodeDpcm(std::span<const uint8_t> data, uint8_t startLevel)
{
    std::vector<uint8_t> levels;
    levels.reserve(data.size() * 8);
    uint8_t level = std::min(startLevel, DPCM_LEVEL_MAX);
    for (uint8_t byte : data) {
        for (int bit = 0; bit < 8; bit++) {
            level = stepDpcmLevel(level, (byte >> bit) & 1);
            levels.push_back(level);
        }
    }
    return levels;
}

} // namespace ocf::audio::tracker
