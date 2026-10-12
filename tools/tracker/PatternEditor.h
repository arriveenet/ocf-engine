// SPDX-License-Identifier: MIT
#pragma once

#include "EditorState.h"

#include <imgui.h>

namespace ocf::tracker_tool {

class TrackerAudio;

/** Shows the patterns of the cursor's frame and edits them with the keyboard. */
class PatternEditor {
public:
    void draw(EditorState& state, TrackerAudio& audio, ImFont* font);

private:
    void drawToolbar(EditorState& state, TrackerAudio& audio);
    void drawGrid(EditorState& state, TrackerAudio& audio);
    void drawChannelHeader(EditorState& state, TrackerAudio& audio, int channel);
    void drawCell(EditorState& state, int row, int channel, float charWidth);

    void handleKeyboard(EditorState& state, TrackerAudio& audio);
    void handleNavigation(EditorState& state);
    void handleNoteInput(EditorState& state, TrackerAudio& audio);
    void handleValueInput(EditorState& state);
    void enterNote(EditorState& state, TrackerAudio& audio, uint8_t note, ImGuiKey key);
    void clearField(EditorState& state);
    void moveRows(EditorState& state, int delta, bool acrossFrames);
    void moveColumn(EditorState& state, int delta);
    void advanceAfterEdit(EditorState& state);
    void stopPreviewOnKeyRelease(TrackerAudio& audio);

    ImGuiKey m_previewKey = ImGuiKey_None;
    ChannelType m_previewChannel = ChannelType::Pulse1;
    bool m_scrollToCursor = true;
};

} // namespace ocf::tracker_tool
