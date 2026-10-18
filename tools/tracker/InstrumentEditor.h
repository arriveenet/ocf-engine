// SPDX-License-Identifier: MIT
#pragma once

#include "EditorState.h"

#include <array>
#include <string>

namespace ocf::tracker_tool {

class TrackerAudio;

/** Lists the instruments, and edits their sequences as MML text and their DPCM keys. */
class InstrumentEditor {
public:
    void draw(EditorState& state, TrackerAudio& audio);

private:
    void drawList(EditorState& state);
    void drawSequence(EditorState& state, Instrument& instrument, SequenceType type);
    void drawDpcmKeys(EditorState& state, Instrument& instrument, TrackerAudio& audio);
    void syncText(const EditorState& state);

    // The text being edited, which may be invalid while typing
    std::array<std::string, SEQUENCE_TYPE_COUNT> m_mmlText;
    std::array<bool, SEQUENCE_TYPE_COUNT> m_mmlValid{};
    int m_textInstrument = -1;
    int m_textRevision = -1;
};

} // namespace ocf::tracker_tool
