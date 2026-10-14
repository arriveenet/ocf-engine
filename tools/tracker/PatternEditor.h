// SPDX-License-Identifier: MIT
#pragma once

#include "EditorState.h"

#include <imgui.h>

#include <vector>

namespace ocf::tracker_tool {

class TrackerAudio;

/** Shows the patterns of the cursor's frame and edits them with the keyboard. */
class PatternEditor {
public:
    void draw(EditorState& state, TrackerAudio& audio, ImFont* font);

    /** Copies the selection, or the field under the cursor when nothing is selected. */
    void copy(EditorState& state);
    void cut(EditorState& state);
    /** Pastes at the cursor's row and channel. */
    void paste(EditorState& state);
    /** Selects the cursor's channel, then the whole frame. */
    void selectAll(EditorState& state);
    bool canPaste() const { return !m_clipboard.empty(); }

    void scrollToCursor() { m_scrollToCursor = true; }

private:
    /** The fields copied from one channel. */
    struct ClipboardChannel {
        ChannelType type = ChannelType::Pulse1;
        int firstField = 0;
        int lastField = 0;
        std::vector<Cell> rows;
    };

    void drawToolbar(EditorState& state, TrackerAudio& audio);
    void drawGrid(EditorState& state, TrackerAudio& audio);
    void drawChannelHeader(EditorState& state, TrackerAudio& audio, int channel);
    void drawCell(EditorState& state, int row, int channel, float charWidth,
                  const SelectionBlock* block);

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
    bool m_dragging = false; // Selecting with the mouse
    std::vector<ClipboardChannel> m_clipboard;
};

} // namespace ocf::tracker_tool
