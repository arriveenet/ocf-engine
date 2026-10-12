// SPDX-License-Identifier: MIT
#include "FrameEditor.h"

#include <imgui.h>

#include <algorithm>
#include <cstdio>

namespace ocf::tracker_tool {

namespace {

constexpr const char* SHORT_CHANNEL_NAMES[CHANNEL_COUNT] = {"P1", "P2", "Tri", "Noi", "DPCM"};

constexpr ImU32 CURRENT_FRAME_COLOR = IM_COL32(50, 60, 90, 255);

/** The lowest pattern that no frame uses and that is empty. */
uint8_t findUnusedPattern(const Song& song, int channel)
{
    const auto type = static_cast<ChannelType>(channel);
    for (size_t index = 0; index < MAX_PATTERNS; index++) {
        const bool used = std::any_of(song.frames.begin(), song.frames.end(),
                                      [&](const Frame& frame) { return frame[channel] == index; });
        if (used) {
            continue;
        }
        const Pattern* pattern = song.findPattern(type, static_cast<uint8_t>(index));
        if (pattern == nullptr || pattern->isEmpty()) {
            return static_cast<uint8_t>(index);
        }
    }
    return 0;
}

void insertFrame(EditorState& state, const Frame& frame)
{
    if (state.song.frames.size() >= MAX_FRAMES) {
        return;
    }
    state.song.frames.insert(state.song.frames.begin() + state.cursor.frame + 1, frame);
    state.cursor.frame++;
    state.markModified();
}

/** A frame of new, empty patterns. */
void addFrame(EditorState& state)
{
    Frame frame{};
    for (int channel = 0; channel < static_cast<int>(CHANNEL_COUNT); channel++) {
        frame[channel] = findUnusedPattern(state.song, channel);
    }
    insertFrame(state, frame);
}

/** A frame that plays the same patterns. */
void duplicateFrame(EditorState& state)
{
    insertFrame(state, state.song.frames[state.cursor.frame]);
}

/** A frame that plays copies of the patterns. */
void cloneFrame(EditorState& state)
{
    Song& song = state.song;
    Frame frame{};
    for (int channel = 0; channel < static_cast<int>(CHANNEL_COUNT); channel++) {
        const auto type = static_cast<ChannelType>(channel);
        frame[channel] = findUnusedPattern(song, channel);
        // Copy first: adding the new pattern may move the existing ones
        Pattern copy = song.getPattern(type, song.frames[state.cursor.frame][channel]);
        song.getPattern(type, frame[channel]) = std::move(copy);
    }
    insertFrame(state, frame);
}

void removeFrame(EditorState& state)
{
    if (state.song.frames.size() <= 1) {
        return;
    }
    state.song.frames.erase(state.song.frames.begin() + state.cursor.frame);
    state.clampCursor();
    state.markModified();
}

void moveFrame(EditorState& state, int delta)
{
    const int target = state.cursor.frame + delta;
    if (target < 0 || target >= static_cast<int>(state.song.frames.size())) {
        return;
    }
    std::swap(state.song.frames[state.cursor.frame], state.song.frames[target]);
    state.cursor.frame = target;
    state.markModified();
}

void changePattern(EditorState& state, int delta)
{
    uint8_t& index = state.song.frames[state.cursor.frame][state.cursor.channel];
    const int newIndex = std::clamp(index + delta, 0, static_cast<int>(MAX_PATTERNS) - 1);
    index = static_cast<uint8_t>(newIndex);
    state.getPattern(state.cursor.frame, state.cursor.channel);
    state.markModified();
}

} // namespace

void FrameEditor::draw(EditorState& state)
{
    state.clampCursor();
    if (ImGui::Begin("フレーム")) {
        drawButtons(state);
        drawTable(state);
    }
    ImGui::End();
}

void FrameEditor::drawButtons(EditorState& state)
{
    if (ImGui::Button("追加")) {
        addFrame(state);
    }
    ImGui::SetItemTooltip("新しい空のパターンを使うフレームを、選択中のフレームの後に追加します");
    ImGui::SameLine();
    if (ImGui::Button("複製")) {
        duplicateFrame(state);
    }
    ImGui::SetItemTooltip("同じパターンを使うフレームを追加します");
    ImGui::SameLine();
    if (ImGui::Button("クローン")) {
        cloneFrame(state);
    }
    ImGui::SetItemTooltip("パターンをコピーして、新しいパターンを使うフレームを追加します");
    ImGui::SameLine();
    ImGui::BeginDisabled(state.song.frames.size() <= 1);
    if (ImGui::Button("削除")) {
        removeFrame(state);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::ArrowButton("##up", ImGuiDir_Up)) {
        moveFrame(state, -1);
    }
    ImGui::SameLine();
    if (ImGui::ArrowButton("##down", ImGuiDir_Down)) {
        moveFrame(state, 1);
    }

    ImGui::Text("パターン番号 (%s):", SHORT_CHANNEL_NAMES[state.cursor.channel]);
    ImGui::SameLine();
    if (ImGui::SmallButton("-")) {
        changePattern(state, -1);
    }
    ImGui::SameLine();
    ImGui::Text("%02X", state.song.frames[state.cursor.frame][state.cursor.channel]);
    ImGui::SameLine();
    if (ImGui::SmallButton("+")) {
        changePattern(state, 1);
    }
}

void FrameEditor::drawTable(EditorState& state)
{
    const ImGuiTableFlags flags =
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg;
    if (!ImGui::BeginTable("##frames", 1 + static_cast<int>(CHANNEL_COUNT), flags)) {
        return;
    }

    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("#");
    for (const char* name : SHORT_CHANNEL_NAMES) {
        ImGui::TableSetupColumn(name);
    }
    ImGui::TableHeadersRow();

    const bool scrollToCursor = (state.cursor.frame != m_lastCursorFrame);
    m_lastCursorFrame = state.cursor.frame;

    for (int frame = 0; frame < static_cast<int>(state.song.frames.size()); frame++) {
        ImGui::TableNextRow();
        ImGui::PushID(frame);
        if (frame == state.cursor.frame) {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, CURRENT_FRAME_COLOR);
        }

        ImGui::TableSetColumnIndex(0);
        const bool playing = state.playback.playing && state.playback.frame == frame;
        ImGui::Text("%s%02X", playing ? ">" : " ", frame);

        for (int channel = 0; channel < static_cast<int>(CHANNEL_COUNT); channel++) {
            ImGui::TableSetColumnIndex(channel + 1);
            ImGui::PushID(channel);
            char label[8];
            snprintf(label, sizeof(label), "%02X", state.song.frames[frame][channel]);
            const bool selected = (frame == state.cursor.frame && channel == state.cursor.channel);
            if (ImGui::Selectable(label, selected)) {
                state.cursor.frame = frame;
                if (state.cursor.channel != channel) {
                    state.cursor.channel = channel;
                    state.cursor.column = 0;
                }
            }
            ImGui::PopID();
        }

        if (scrollToCursor && frame == state.cursor.frame) {
            ImGui::SetScrollHereY(0.5f);
        }
        ImGui::PopID();
    }

    ImGui::EndTable();
}

} // namespace ocf::tracker_tool
