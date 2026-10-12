// SPDX-License-Identifier: MIT
#pragma once

#include "EditorState.h"
#include "FrameEditor.h"
#include "InstrumentEditor.h"
#include "PatternEditor.h"

#include <ocf/scene/Scene.h>

#include <memory>

struct ImFont;

namespace ocf::tracker_tool {

class TrackerAudio;

/** The whole tracker: owns the song being edited and draws the editor windows. */
class TrackerScene : public Scene {
public:
    explicit TrackerScene(ImFont* monospaceFont);
    ~TrackerScene() override;

    void onEnter() override;

    void onExit() override;

    void onUpdate(float deltaTime) override;

    void onImGui() override;

private:
    enum class PendingAction : uint8_t { None, NewSong, DemoSong };

    void handleShortcuts();
    void drawMenuBar();
    void drawSongWindow();
    void drawConfirmPopup();
    void drawHelpWindow();

    /** Asks for confirmation first when the song has unsaved changes. */
    void requestAction(PendingAction action);
    void runAction(PendingAction action);

    void playFromStart();
    void playFromFrame();
    void playFromRow();
    void stop();

    ImFont* m_monospaceFont = nullptr;
    EditorState m_state;
    std::unique_ptr<TrackerAudio> m_audio;
    PatternEditor m_patternEditor;
    FrameEditor m_frameEditor;
    InstrumentEditor m_instrumentEditor;

    PendingAction m_pendingAction = PendingAction::None;
    bool m_openConfirmPopup = false;
    bool m_showHelp = false;
    int m_rowsInput = DEFAULT_ROWS_PER_PATTERN;
    bool m_editingRows = false;
};

} // namespace ocf::tracker_tool
