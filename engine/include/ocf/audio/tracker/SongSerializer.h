// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/audio/tracker/Song.h"
#include "ocf/core/Result.h"

#include <filesystem>
#include <string>
#include <string_view>

/**
 * Reads and writes songs in the JSON format described in docs/tracker-format.md.
 */
namespace ocf::audio::tracker {

constexpr const char* SONG_FORMAT_NAME = "ocf-tracker-song";
constexpr int SONG_FORMAT_VERSION = 1;

/** Writes the song as JSON, with a stable key order and layout so that diffs stay small. */
std::string saveSongToString(const Song& song);

/**
 * Reads a song. Invalid values are errors rather than being clamped. The error message starts
 * with the JSON pointer of the problem, e.g. "/patterns/3/rows/7/note: invalid note "H-4"".
 */
Result<Song, std::string> loadSongFromString(std::string_view json);

/** Writes to a temporary file first, so that a failed save leaves the existing file intact. */
Result<void, std::string> saveSong(const Song& song, const std::filesystem::path& path);

Result<Song, std::string> loadSong(const std::filesystem::path& path);

} // namespace ocf::audio::tracker
