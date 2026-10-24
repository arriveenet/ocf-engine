// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/compiler.h"
#include "ocf/audio/tracker/Song.h"
#include "ocf/core/Result.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

namespace ocf::audio::tracker {

struct WavExportOptions {
    uint32_t sampleRate = 48000;

    // A song that loops (it has no Cxx) plays this many times, then fades out
    int playCount = 2;
    float fadeOutSeconds = 5.0f; // 0: cut at the end of the last play, e.g. for a game loop
};

/** The length an export would have. */
struct WavExportLength {
    double seconds = 0.0;
    bool loops = false; // The song loops, so the play count and the fade out apply
};

/**
 * Renders songs to 16-bit mono WAV files, with the same APU emulation as the real-time
 * playback. A song ends at Cxx, or, when it loops, after playing WavExportOptions::playCount
 * times. A jump back to an earlier position (the wrap after the last frame, or Bxx) counts as
 * the start of the next play.
 */
class OCF_API SongExporter {
public:
    static constexpr int MAX_PLAY_COUNT = 16;
    static constexpr float MAX_FADE_OUT_SECONDS = 30.0f;
    static constexpr double MAX_SECONDS = 60.0 * 60.0; // Longer exports are cut

    /** Called with the progress (0.0 to 1.0). Returning false cancels the export. */
    using ProgressCallback = std::function<bool(double progress)>;

    SongExporter() = delete;

    /** Plays the song through without the APU, which is quick. */
    static WavExportLength measure(const Song& song, const WavExportOptions& options);

    /**
     * Writes to a temporary file first, so that a failed or cancelled export leaves an existing
     * file intact. Rendering takes a while (the APU is clocked at the CPU rate), so call it from
     * a worker thread in an interactive tool.
     */
    static Result<void, std::string> exportWav(const Song& song, const std::filesystem::path& path,
                                               const WavExportOptions& options,
                                               const ProgressCallback& onProgress = {});
};

} // namespace ocf::audio::tracker
