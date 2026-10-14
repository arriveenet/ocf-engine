// SPDX-License-Identifier: MIT
#include "TrackerScene.h"

#include "DemoSong.h"
#include "TrackerAudio.h"

#include <ocf/audio/tracker/SongSerializer.h>
#include <ocf/core/Engine.h>
#include <ocf/platform/FileSystem.h>

#include <imgui.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <format>
#include <string>
#include <utility>

namespace ocf::tracker_tool {

namespace {

constexpr float LEFT_PANEL_WIDTH = 340.0f;
constexpr float SONG_WINDOW_HEIGHT = 215.0f;
constexpr float FRAME_WINDOW_HEIGHT = 210.0f;

constexpr const char* CONFIRM_POPUP = "Confirm";
constexpr const char* ERROR_POPUP = "Error";

// Songs are kept with the other assets, so that games can load them
constexpr const char* SONG_DIRECTORY = "songs";
constexpr const char* DEFAULT_FILE_NAME = "untitled.json";

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
    {"Delete", "Clear the field under the cursor, or the selection"},
    {"Arrow keys", "Move the cursor"},
    {"Shift+Arrow keys / drag", "Select (within the frame)"},
    {"Escape", "Clear the selection"},
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
    {"Ctrl+Z", "Undo"},
    {"Ctrl+Y / Ctrl+Shift+Z", "Redo"},
    {"Ctrl+C / Ctrl+X", "Copy / cut the selection (or the field under the cursor)"},
    {"Ctrl+V", "Paste at the cursor"},
    {"Ctrl+A", "Select the channel, again for the whole frame"},
    {"Ctrl+N", "New song"},
    {"Ctrl+O", "Open a song"},
    {"Ctrl+S / Ctrl+Shift+S", "Save / save as"},
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
    m_state.beginFrame();
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
    drawFileDialog();
    drawErrorPopup();
    drawHelpWindow();

    // Typing in a text field or dragging a slider is one undo step, recorded when it ends
    if (!ImGui::IsAnyItemActive()) {
        m_state.commitEdit();
    }

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

    if (io.KeyCtrl) {
        if (ImGui::IsKeyPressed(ImGuiKey_N, false)) {
            requestAction(PendingAction::NewSong);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_O, false)) {
            requestAction(PendingAction::OpenFile);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_S, false)) {
            if (io.KeyShift) {
                saveAs();
            }
            else {
                save();
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
            if (io.KeyShift) {
                redo();
            }
            else {
                undo();
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Y)) {
            redo();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_C, false)) {
            m_patternEditor.copy(m_state);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_X, false)) {
            m_patternEditor.cut(m_state);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_V, false)) {
            m_patternEditor.paste(m_state);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_A, false)) {
            m_patternEditor.selectAll(m_state);
        }
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
        if (ImGui::MenuItem("New", "Ctrl+N")) {
            requestAction(PendingAction::NewSong);
        }
        if (ImGui::MenuItem("Open...", "Ctrl+O")) {
            requestAction(PendingAction::OpenFile);
        }
        if (ImGui::MenuItem("Open Demo Song")) {
            requestAction(PendingAction::DemoSong);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Save", "Ctrl+S")) {
            save();
        }
        if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
            saveAs();
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("Undo", "Ctrl+Z", false, m_state.canUndo())) {
            undo();
        }
        if (ImGui::MenuItem("Redo", "Ctrl+Y", false, m_state.canRedo())) {
            redo();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Cut", "Ctrl+X")) {
            m_patternEditor.cut(m_state);
        }
        if (ImGui::MenuItem("Copy", "Ctrl+C")) {
            m_patternEditor.copy(m_state);
        }
        if (ImGui::MenuItem("Paste", "Ctrl+V", false, m_patternEditor.canPaste())) {
            m_patternEditor.paste(m_state);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Select All", "Ctrl+A")) {
            m_patternEditor.selectAll(m_state);
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

    const std::string fileName = m_state.filePath.empty()
                                     ? std::string("(untitled)")
                                     : pathToUtf8(m_state.filePath.filename());
    const std::string playback =
        m_state.playback.playing
            ? std::format("Playing  Frame {:02X}  Row {:02X}", m_state.playback.frame,
                          m_state.playback.row)
            : std::string("Stopped");
    const std::string status =
        std::format("{}{}    {}", fileName, m_state.modified ? "*" : "", playback);
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

void TrackerScene::drawErrorPopup()
{
    if (m_openErrorPopup) {
        ImGui::OpenPopup(ERROR_POPUP);
        m_openErrorPopup = false;
    }

    if (ImGui::BeginPopupModal(ERROR_POPUP, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(m_errorMessage.c_str());
        if (ImGui::Button("OK", ImVec2(120.0f, 0.0f))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void TrackerScene::drawFileDialog()
{
    if (!m_fileDialog.draw()) {
        return;
    }
    if (m_fileDialog.getMode() == FileDialog::Mode::Open) {
        openFrom(m_fileDialog.getPath());
    }
    else {
        saveTo(m_fileDialog.getPath());
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
        ImGui::TextDisabled("Click a channel name to mute it. Right-click it to change the "
                            "number of effect columns.");
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
    case PendingAction::OpenFile:
        m_fileDialog.open(FileDialog::Mode::Open, getSongDirectory());
        break;
    default: break;
    }
}

void TrackerScene::save()
{
    if (m_state.filePath.empty()) {
        saveAs();
    }
    else {
        saveTo(m_state.filePath);
    }
}

void TrackerScene::saveAs()
{
    const std::string fileName = m_state.filePath.empty() ? std::string(DEFAULT_FILE_NAME)
                                                          : pathToUtf8(m_state.filePath.filename());
    m_fileDialog.open(FileDialog::Mode::Save, getSongDirectory(), fileName);
}

void TrackerScene::saveTo(const std::filesystem::path& path)
{
    const auto result = audio::tracker::saveSong(m_state.song, path);
    if (result.isErr()) {
        showError(std::format("Could not save the song.\n\n{}", result.unwrapErr()));
        return;
    }
    m_state.filePath = path;
    m_state.modified = false;
}

void TrackerScene::openFrom(const std::filesystem::path& path)
{
    auto result = audio::tracker::loadSong(path);
    if (result.isErr()) {
        showError(std::format("Could not open {}.\n\n{}", pathToUtf8(path.filename()),
                              result.unwrapErr()));
        return;
    }
    stop();
    m_state.replaceSong(std::move(result).unwrap(), path);
}

std::filesystem::path TrackerScene::getSongDirectory() const
{
    if (!m_state.filePath.empty()) {
        return m_state.filePath.parent_path();
    }
    return std::filesystem::path(FileSystem::getInstance()->getAssetPath()) / SONG_DIRECTORY;
}

void TrackerScene::showError(std::string message)
{
    m_errorMessage = std::move(message);
    m_openErrorPopup = true;
}

void TrackerScene::undo()
{
    m_state.undo();
    m_patternEditor.scrollToCursor();
}

void TrackerScene::redo()
{
    m_state.redo();
    m_patternEditor.scrollToCursor();
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
