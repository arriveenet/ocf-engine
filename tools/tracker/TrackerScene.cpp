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

constexpr const char* CONFIRM_POPUP = "確認";

struct KeyHelp {
    const char* keys;
    const char* description;
};

constexpr KeyHelp KEY_HELP[] = {
    {"Z S X D C V G B H N J M , L .", "ノート入力（現在のオクターブ）"},
    {"Q 2 W 3 E R 5 T 6 Y 7 U I 9 O 0 P", "ノート入力（1オクターブ上）"},
    {"1", "ノートオフ (---)"},
    {"-", "ノートリリース (===)"},
    {"0-9 A-F", "インストゥルメント・音量・エフェクト値の入力（16進）"},
    {"F B D C", "エフェクトの種類（エフェクト列）"},
    {"Delete", "カーソル位置を消去"},
    {"矢印キー", "カーソル移動"},
    {"PageUp / PageDown", "16行移動"},
    {"Home / End", "先頭 / 末尾の行"},
    {"Tab / Shift+Tab", "次 / 前のチャンネル"},
    {"Space", "編集モードの切り替え（オフのときは試聴のみ）"},
    {"テンキー / *", "オクターブを下げる / 上げる"},
    {"Enter", "再生 / 停止（カーソルのフレームから）"},
    {"F5", "最初から再生"},
    {"F6", "カーソルのフレームの先頭から再生"},
    {"F7", "カーソル位置から再生"},
    {"F8", "停止"},
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

    if (ImGui::BeginMenu("ファイル")) {
        if (ImGui::MenuItem("新規")) {
            requestAction(PendingAction::NewSong);
        }
        if (ImGui::MenuItem("デモ曲を開く")) {
            requestAction(PendingAction::DemoSong);
        }
        ImGui::Separator();
        // Waiting for the JSON serializer (docs/tracker-format.md)
        for (const char* label : {"開く...", "保存", "名前を付けて保存..."}) {
            ImGui::MenuItem(label, nullptr, false, false);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("JSONの読み書きはまだ実装されていません");
            }
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("再生")) {
        if (ImGui::MenuItem("最初から再生", "F5")) {
            playFromStart();
        }
        if (ImGui::MenuItem("フレームの先頭から再生", "F6")) {
            playFromFrame();
        }
        if (ImGui::MenuItem("カーソル位置から再生", "F7")) {
            playFromRow();
        }
        if (ImGui::MenuItem("停止", "F8")) {
            stop();
        }
        ImGui::Separator();
        ImGui::MenuItem("再生位置に追従", nullptr, &m_state.followPlayback);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("ヘルプ")) {
        ImGui::MenuItem("キー操作", nullptr, &m_showHelp);
        ImGui::EndMenu();
    }

    const std::string status =
        m_state.playback.playing
            ? std::format("再生中  フレーム {:02X}  行 {:02X}", m_state.playback.frame,
                          m_state.playback.row)
            : std::string("停止中");
    const float statusWidth = ImGui::CalcTextSize(status.c_str()).x;
    ImGui::SameLine(ImGui::GetWindowWidth() - statusWidth - ImGui::GetStyle().ItemSpacing.x * 2);
    ImGui::TextUnformatted(status.c_str());

    ImGui::EndMainMenuBar();
}

void TrackerScene::drawSongWindow()
{
    if (ImGui::Begin("曲")) {
        Song& song = m_state.song;
        if (ImGui::InputText("タイトル", &song.title)) {
            m_state.markModified();
        }
        if (ImGui::InputText("作者", &song.author)) {
            m_state.markModified();
        }

        int speed = song.speed;
        if (ImGui::InputInt("スピード", &speed)) {
            song.speed = static_cast<uint8_t>(std::clamp<int>(speed, MIN_SPEED, MAX_SPEED));
            m_state.markModified();
        }
        ImGui::SetItemTooltip("1行あたりのtick数（%d-%d）。Fxx の 01-1F でも変わります", MIN_SPEED,
                              MAX_SPEED);

        int tempo = song.tempo;
        if (ImGui::InputInt("テンポ", &tempo)) {
            song.tempo = static_cast<uint8_t>(std::clamp<int>(tempo, MIN_TEMPO, MAX_TEMPO));
            m_state.markModified();
        }
        ImGui::SetItemTooltip("%d-%d。Fxx の 20-FF でも変わります", MIN_TEMPO, MAX_TEMPO);

        // Applied on Enter, since fewer rows remove the rows beyond the end
        if (!m_editingRows) {
            m_rowsInput = song.rowsPerPattern;
        }
        if (ImGui::InputInt("行数", &m_rowsInput, 1, 16, ImGuiInputTextFlags_EnterReturnsTrue)) {
            song.setRowsPerPattern(
                static_cast<uint16_t>(std::clamp(m_rowsInput, 1, static_cast<int>(MAX_ROWS))));
            m_state.clampCursor();
            m_state.markModified();
        }
        m_editingRows = ImGui::IsItemActive();
        ImGui::SetItemTooltip("Enterで確定します。減らすと、範囲外の行は消えます");

        ImGui::Separator();
        ImGui::TextDisabled("フレーム: %zu  インストゥルメント: %zu", song.frames.size(),
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
        ImGui::TextUnformatted("現在の曲の変更は失われます。よろしいですか？");
        if (ImGui::Button("OK", ImVec2(120.0f, 0.0f))) {
            runAction(m_pendingAction);
            m_pendingAction = PendingAction::None;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("キャンセル", ImVec2(120.0f, 0.0f))) {
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
    if (ImGui::Begin("キー操作", &m_showHelp)) {
        if (ImGui::BeginTable("##keys", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV)) {
            ImGui::TableSetupColumn("キー", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("操作");
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
        ImGui::TextDisabled("チャンネル名のクリックでミュート、右クリックでエフェクト列の数を変更");
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
