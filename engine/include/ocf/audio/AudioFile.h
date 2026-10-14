// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/Result.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace ocf::audio {

struct MonoAudio {
    std::vector<float> samples; // -1.0 to 1.0
    uint32_t sampleRate = 0;
};

/** Reads a whole WAV, FLAC or MP3 file, mixed down to mono, at its own sample rate. */
Result<MonoAudio, std::string> readMonoAudio(const std::filesystem::path& path);

} // namespace ocf::audio
