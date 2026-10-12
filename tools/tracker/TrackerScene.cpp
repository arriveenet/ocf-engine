// SPDX-License-Identifier: MIT
#include "TrackerScene.h"

#include "DemoSong.h"
#include "TrackerAudio.h"

#include <ocf/core/Engine.h>

#include <imgui.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <format>
#include <string>

namespace ocf::tracker_tool {

namespace {

constexpr float LEFT_PANEL_WIDTH = 340.0f;
constexpr float SONG_WINDOW_HEIGHT = 215.0f;
constexpr float FRAME_WINDOW_HEIGHT = 210.0f;

constexpr const char* CONFIRM_POPUP = "Confirm";

struct KeyHelp {
    const char* keys;
    const char* description;
};

constexpr KeyHelp KEY_HELP[] = {
    {"Z S X D C V G B H N J M , L . ; /", "Note (current octave)"},
    {"Q 2 W 3 E R 5 T 6 Y 7 U I 9 O 0 P", "Note (one octave up)"},
    {"1", "Note off (---)"},
    {"-", "Note release (===)"},
    {"0-9 A-F", "Instrument, volume and effect parameter (hexadecimal)"},
    {"F B D C", "Effect (effect column)"},
    {"Delete", "Clear the field under the cursor"},
    {"Arrow keys", "Move the cursor"},
    {"PageUp / PageDown", "Move 16 rows"},
    {"Home / End", "First / last row"},
    {"Tab / Shift+Tab", "Next / previous channel"},
    {"Space", "Toggle edit mode (when off, note keys only preview)"},
    {"Keypad / *", "Octave down / up"},
    {"Enter", "Play from the cursor's frame / stop"},
    {"F5", "Play from the start"},
    {"F6", "Play from the start of the cursor's frame"},
    {"F7", "Play from the cursor"},
    {"F8", "Stop"},
};

} // namespace

TrackerScene::TrackerScene(ImFont* monospaceFont)
    : m_monospaceFont(monospaceFont)
{
}

TrackerScene::~TrackerScene() = default;

void TrackerScene::onEnter()
{
    m_audio = std::make_unique<TrackerAudio>(getEngine().getAudioSystem());
    m_state.replaceSong(createDemoSong());
}

void TrackerScene::onExit()
{
    m_audio.reset();
}

void TrackerScene::onUpdate(float /*deltaTime*/)
{
    m_state.playback = m_audio->getStatus();
    if (m_state.playback.playing && m_state.followPlayback) {
        m_state.cursor.frame = m_state.playback.frame;
        m_state.cursor.row = m_state.playback.row;
        m_state.clampCursor();
    }
}

void TrackerScene::onImGui()
{
    handleShortcuts();
    drawMenuBar();

    // Initial layout: song, frames and instruments on the left, the pattern on the right
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float menuHeight = ImGui::GetFrameHeight();
    const ImVec2 origin(viewport->Pos.x, viewport->Pos.y + menuHeight);
    const float height = viewport->Size.y - menuHeight;

    ImGui::SetNextWindowPos(origin, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(LEFT_PANEL_WIDTH, SONG_WINDOW_HEIGHT), ImGuiCond_FirstUseEver);
    drawSongWindow();

    ImGui::SetNextWindowPos(ImVec2(origin.x, origin.y + SONG_WINDOW_HEIGHT), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(LEFT_PANEL_WIDTH, FRAME_WINDOW_HEIGHT), ImGuiCond_FirstUseEver);
    m_frameEditor.draw(m_state);

    const float instrumentTop = SONG_WINDOW_HEIGHT + FRAME_WINDOW_HEIGHT;
    ImGui::SetNextWindowPos(ImVec2(origin.x, origin.y + instrumentTop), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(LEFT_PANEL_WIDTH, height - instrumentTop), ImGuiCond_FirstUseEver);
    m_instrumentEditor.draw(m_state, *m_audio);

    ImGui::SetNextWindowPos(ImVec2(origin.x + LEFT_PANEL_WIDTH, origin.y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x - LEFT_PANEL_WIDTH, height),
                             ImGuiCond_FirstUseEver);
    m_patternEditor.draw(m_state, *m_audio, m_monospaceFont);

    drawConfirmPopup();
    drawHelpWindow();

    // Send the changes of this frame to the player at once
    if (m_state.songChanged) {
        m_audio->setSong(m_state.song);
        m_state.songChanged = false;
    }
}

void TrackerScene::handleShortcuts()
{
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput ||
        ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel)) {
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_F5, false)) {
        playFromStart();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F6, false)) {
        playFromFrame();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F7, false)) {
        playFromRow();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F8, false)) {
        stop();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
        ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false)) {
        if (m_state.playback.playing) {
            stop();
        }
        else {
            playFromFrame();
        }
    }
}

void TrackerScene::drawMenuBar()
{
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New")) {
            requestAction(PendingAction::NewSong);
        }
        if (ImGui::MenuItem("Open Demo Song")) {
            requestAction(PendingAction::DemoSong);
        }
        ImGui::Separator();
        // Waiting for the JSON serializer (docs/tracker-format.md)
        for (const char* label : {"Open...", "Save", "Save As..."}) {
            ImGui::MenuItem(label, nullptr, false, false);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("Reading and writing JSON is not implemented yet");
            }
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Playback")) {
        if (ImGui::MenuItem("Play from Start", "F5")) {
            playFromStart();
        }
        if (ImGui::MenuItem("Play from Frame", "F6")) {
            playFromFrame();
        }
        if (ImGui::MenuItem("Play from Cursor", "F7")) {
            playFromRow();
        }
        if (ImGui::MenuItem("Stop", "F8")) {
            stop();
        }
        ImGui::Separator();
        ImGui::MenuItem("Follow Playback", nullptr, &m_state.followPlayback);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
        ImGui::MenuItem("Keyboard Shortcuts", nullptr, &m_showHelp);
        ImGui::EndMenu();
    }

    const std::string status =
        m_state.playback.playing
            ? std::format("Playing  Frame {:02X}  Row {:02X}", m_state.playback.frame,
                          m_state.playback.row)
            : std::string("Stopped");
    const float statusWidth = ImGui::CalcTextSize(status.c_str()).x;
    ImGui::SameLine(ImGui::GetWindowWidth() - statusWidth - ImGui::GetStyle().ItemSpacing.x * 2);
    ImGui::TextUnformatted(status.c_str());

    ImGui::EndMainMenuBar();
}

void TrackerScene::drawSongWindow()
{
    if (ImGui::Begin("Song")) {
        Song& song = m_state.song;
        if (ImGui::InputText("Title", &song.title)) {
            m_state.markModified();
        }
        if (ImGui::InputText("Author", &song.author)) {
            m_state.markModified();
        }

        int speed = song.speed;
        if (ImGui::InputInt("Speed", &speed)) {
            song.speed = static_cast<uint8_t>(std::clamp<int>(speed, MIN_SPEED, MAX_SPEED));
            m_state.markModified();
        }
        ImGui::SetItemTooltip("Ticks per row (%d-%d). Also set by Fxx with 01-1F", MIN_SPEED,
                              MAX_SPEED);

        int tempo = song.tempo;
        if (ImGui::InputInt("Tempo", &tempo)) {
            song.tempo = static_cast<uint8_t>(std::clamp<int>(tempo, MIN_TEMPO, MAX_TEMPO));
            m_state.markModified();
        }
        ImGui::SetItemTooltip("%d-%d. Also set by Fxx with 20-FF", MIN_TEMPO, MAX_TEMPO);

        // Applied on Enter, since fewer rows remove the rows beyond the end
        if (!m_editingRows) {
            m_rowsInput = song.rowsPerPattern;
        }
        if (ImGui::InputInt("Rows", &m_rowsInput, 1, 16, ImGuiInputTextFlags_EnterReturnsTrue)) {
            song.setRowsPerPattern(
                static_cast<uint16_t>(std::clamp(m_rowsInput, 1, static_cast<int>(MAX_ROWS))));
            m_state.clampCursor();
            m_state.markModified();
        }
        m_editingRows = ImGui::IsItemActive();
        ImGui::SetItemTooltip("Applied with Enter. Fewer rows remove the rows beyond the end");

        ImGui::Separator();
        ImGui::TextDisabled("Frames: %zu  Instruments: %zu", song.frames.size(),
                            song.instruments.size());
    }
    ImGui::End();
}

void TrackerScene::drawConfirmPopup()
{
    if (m_openConfirmPopup) {
        ImGui::OpenPopup(CONFIRM_POPUP);
        m_openConfirmPopup = false;
    }

    if (ImGui::BeginPopupModal(CONFIRM_POPUP, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("The changes to the current song will be lost. Continue?");
        if (ImGui::Button("OK", ImVec2(120.0f, 0.0f))) {
            runAction(m_pendingAction);
            m_pendingAction = PendingAction::None;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f))) {
            m_pendingAction = PendingAction::None;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void TrackerScene::drawHelpWindow()
{
    if (!m_showHelp) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(560.0f, 0.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Keyboard Shortcuts", &m_showHelp)) {
        if (ImGui::BeginTable("##keys", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV)) {
            ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Action");
            ImGui::TableHeadersRow();
            for (const KeyHelp& help : KEY_HELP) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(help.keys);
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(help.description);
            }
            ImGui::EndTable();
        }
        ImGui::TextDisabled("Click a channel name to mute it. Right-click it to change the number of effect columns.");
    }
    ImGui::End();
}

void TrackerScene::requestAction(PendingAction action)
{
    if (m_state.modified) {
        m_pendingAction = action;
        m_openConfirmPopup = true;
    }
    else {
        runAction(action);
    }
}

void TrackerScene::runAction(PendingAction action)
{
    switch (action) {
    case PendingAction::NewSong:
        stop();
        m_state.replaceSong(Song::createDefault());
        break;
    case PendingAction::DemoSong:
        stop();
        m_state.replaceSong(createDemoSong());
        break;
    default: break;
    }
}

void TrackerScene::playFromStart()
{
    m_audio->playSong(0, 0);
}

void TrackerScene::playFromFrame()
{
    m_audio->playSong(m_state.cursor.frame, 0);
}

void TrackerScene::playFromRow()
{
    m_audio->playSong(m_state.cursor.frame, m_state.cursor.row);
}

void TrackerScene::stop()
{
    m_audio->stopSong();
}

} // namespace ocf::tracker_tool
