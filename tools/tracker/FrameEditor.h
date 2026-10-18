// SPDX-License-Identifier: MIT
#pragma once

#include "EditorState.h"

namespace ocf::tracker_tool {

/** Edits the order of frames and the pattern each channel plays in them. */
class FrameEditor {
public:
    void draw(EditorState& state);

private:
    void drawButtons(EditorState& state);
    void drawTable(EditorState& state);

    int m_lastCursorFrame = -1;
};

} // namespace ocf::tracker_tool
