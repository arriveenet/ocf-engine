// SPDX-License-Identifier: MIT
#pragma once

#include "EditorState.h"
#include "FileDialog.h"

#include <ocf/audio/AudioFile.h>
#include <ocf/audio/tracker/DpcmEncoder.h>

#include <filesystem>
#include <optional>
#include <string>

namespace ocf::tracker_tool {

class TrackerAudio;

/**
 * Lists the DPCM samples of the song. Imports them from WAV, FLAC and MP3 files (converted with
 * the import settings) or from .dmc files, and exports them as .dmc files.
 */
class DpcmEditor {
public:
    DpcmEditor();

    /** Draws the window while state.showDpcmSamples is set. */
    void draw(EditorState& state, TrackerAudio& audio);

private:
    void drawList(EditorState& state);
    void drawSample(EditorState& state, TrackerAudio& audio);
    void drawImportSettings(EditorState& state);
    void drawFileDialog(EditorState& state);

    void importFile(EditorState& state, const std::filesystem::path& path);
    void exportFile(const EditorState& state, const std::filesystem::path& path);
    void reconvert(EditorState& state);
    void removeSample(EditorState& state);

    Result<audio::tracker::DpcmEncodeResult, std::string> encode(const audio::MonoAudio& audio) const;
    /** The audio of the last imported file belongs to the current sample. */
    bool hasSource(const EditorState& state) const;

    void setMessage(std::string message, bool error = false);

    FileDialog m_fileDialog{"SampleDialog"};
    std::filesystem::path m_directory;

    // Import settings, as in ocf_dmcconv
    int m_rate = -1; // -1: the highest rate that fits
    bool m_normalize = false;
    float m_gainDb = 0.0f;
    bool m_trim = true;
    float m_trimThresholdDb = -50.0f;
    int m_restLevel = audio::tracker::DPCM_DEFAULT_LEVEL; // Also the center of the waveform
    bool m_greedy = false;
    bool m_truncate = false;

    // The audio of the last imported file, to convert it again with other settings
    std::optional<audio::MonoAudio> m_source;
    int m_sourceSample = -1;
    size_t m_sourceSampleCount = 0;
    int m_sourceRevision = -1;

    std::string m_message;
    bool m_messageIsError = false;
};

} // namespace ocf::tracker_tool
