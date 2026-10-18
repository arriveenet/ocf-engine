// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/audio/tracker/Song.h"
#include "ocf/core/Result.h"

#include <filesystem>
#include <string>
#include <string_view>

namespace ocf::audio::tracker {

/**
 * Reads and writes songs in the JSON format described in docs/tracker-format.md.
 */
class SongSerializer {
public:
    static constexpr const char* FORMAT_NAME = "ocf-tracker-song";
    static constexpr int FORMAT_VERSION = 1;

    SongSerializer() = delete;

    /** Writes the song as JSON, with a stable key order and layout so that diffs stay small. */
    static std::string saveToString(const Song& song);

    /**
     * Reads a song. Invalid values are errors rather than being clamped. The error message
     * starts with the JSON pointer of the problem, e.g. "/patterns/3/rows/7/note: invalid note
     * "H-4"".
     */
    static Result<Song, std::string> loadFromString(std::string_view json);

    /** Writes to a temporary file first, so that a failed save leaves the existing file intact. */
    static Result<void, std::string> save(const Song& song, const std::filesystem::path& path);

    static Result<Song, std::string> load(const std::filesystem::path& path);
};

} // namespace ocf::audio::tracker
