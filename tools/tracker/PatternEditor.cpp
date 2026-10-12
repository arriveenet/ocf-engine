// SPDX-License-Identifier: MIT
#include "PatternEditor.h"

#include "TrackerAudio.h"

#include <ocf/audio/tracker/SongNotation.h>

#include <format>
#include <optional>
#include <string>

namespace ocf::tracker_tool {

namespace {

constexpr int PAGE_ROWS = 16;
constexpr int MAX_OCTAVE = 7;
constexpr int MAX_EDIT_STEP = 16;

constexpr ImU32 NOTE_COLOR = IM_COL32(230, 230, 230, 255);
constexpr ImU32 INSTRUMENT_COLOR = IM_COL32(130, 220, 130, 255);
constexpr ImU32 VOLUME_COLOR = IM_COL32(130, 180, 255, 255);
constexpr ImU32 EFFECT_COLOR = IM_COL32(255, 180, 100, 255);
constexpr ImU32 EMPTY_COLOR = IM_COL32(110, 110, 110, 255);

constexpr ImU32 CURSOR_COLOR = IM_COL32(60, 110, 210, 200);
constexpr ImU32 CURSOR_EDIT_COLOR = IM_COL32(200, 60, 60, 200);
constexpr ImU32 CURSOR_ROW_COLOR = IM_COL32(50, 60, 90, 255);
constexpr ImU32 CURSOR_EDIT_ROW_COLOR = IM_COL32(80, 40, 40, 255);
constexpr ImU32 PLAYBACK_ROW_COLOR = IM_COL32(110, 50, 110, 255);
constexpr ImU32 BEAT_ROW_COLOR = IM_COL32(38, 38, 30, 255);    // Every 4 rows
constexpr ImU32 MEASURE_ROW_COLOR = IM_COL32(52, 52, 36, 255); // Every 16 rows

/** Note keys laid out like a piano, in semitones above the current octave. */
struct NoteKey {
    ImGuiKey key;
    int semitone;
};

constexpr NoteKey NOTE_KEYS[] = {
    // Lower row: the current octave
    {ImGuiKey_Z, 0}, {ImGuiKey_S, 1}, {ImGuiKey_X, 2}, {ImGuiKey_D, 3}, {ImGuiKey_C, 4},
    {ImGuiKey_V, 5}, {ImGuiKey_G, 6}, {ImGuiKey_B, 7}, {ImGuiKey_H, 8}, {ImGuiKey_N, 9},
    {ImGuiKey_J, 10}, {ImGuiKey_M, 11}, {ImGuiKey_Comma, 12}, {ImGuiKey_L, 13},
    {ImGuiKey_Period, 14}, {ImGuiKey_Semicolon, 15}, {ImGuiKey_Slash, 16},
    // Upper row: one octave up
    {ImGuiKey_Q, 12}, {ImGuiKey_2, 13}, {ImGuiKey_W, 14}, {ImGuiKey_3, 15}, {ImGuiKey_E, 16},
    {ImGuiKey_R, 17}, {ImGuiKey_5, 18}, {ImGuiKey_T, 19}, {ImGuiKey_6, 20}, {ImGuiKey_Y, 21},
    {ImGuiKey_7, 22}, {ImGuiKey_U, 23}, {ImGuiKey_I, 24}, {ImGuiKey_9, 25}, {ImGuiKey_O, 26},
    {ImGuiKey_0, 27}, {ImGuiKey_P, 28},
};

constexpr ImGuiKey NOTE_OFF_KEY = ImGuiKey_1;
constexpr ImGuiKey NOTE_RELEASE_KEY = ImGuiKey_Minus;

constexpr EffectType EFFECT_TYPES[] = {
    EffectType::Speed,
    EffectType::Jump,
    EffectType::Skip,
    EffectType::Halt,
};

/*
 * Characters of a cell: "C-4 00 F F06 F06 ..."
 *   note 0-2, instrument 4-5, volume 7, effect column e at 9 + 4e (type) and 10 + 4e (parameter)
 */
int getCellChars(int effectColumns)
{
    return 8 + 4 * effectColumns;
}

struct CharRange {
    int start;
    int count;
};

CharRange getColumnChars(int column)
{
    switch (column) {
    case 0: return {0, 3};
    case 1: return {4, 1};
    case 2: return {5, 1};
    case 3: return {7, 1};
    default: {
        const int effectColumn = column - FIXED_COLUMN_COUNT;
        return {9 + 4 * (effectColumn / COLUMNS_PER_EFFECT) + effectColumn % COLUMNS_PER_EFFECT, 1};
    }
    }
}

int getColumnAtChar(int charIndex, int columnCount)
{
    for (int column = 0; column < columnCount; column++) {
        const CharRange range = getColumnChars(column);
        if (charIndex < range.start + range.count) {
            return column;
        }
    }
    return columnCount - 1;
}

std::optional<int> getPressedHexDigit()
{
    for (int i = 0; i < 10; i++) {
        if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_0 + i), false) ||
            ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_Keypad0 + i), false)) {
            return i;
        }
    }
    for (int i = 0; i < 6; i++) {
        if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_A + i), false)) {
            return 10 + i;
        }
    }
    return std::nullopt;
}

uint8_t setDigit(uint8_t value, int digit, int hex)
{
    return static_cast<uint8_t>(digit == 0 ? (hex << 4) | (value & 0x0F) : (value & 0xF0) | hex);
}

ImU32 dim(ImU32 color)
{
    return (color & ~IM_COL32_A_MASK) | (static_cast<ImU32>(80) << IM_COL32_A_SHIFT);
}

void toggleMute(EditorState& state, TrackerAudio& audio, int channel)
{
    state.muted[channel] = !state.muted[channel];
    audio.setChannelMuted(static_cast<ChannelType>(channel), state.muted[channel]);
}

} // namespace

void PatternEditor::draw(EditorState& state, TrackerAudio& audio, ImFont* font)
{
    state.clampCursor();
    handleKeyboard(state, audio);
    if (state.playback.playing && state.followPlayback) {
        m_scrollToCursor = true;
    }

    if (ImGui::Begin("Pattern")) {
        drawToolbar(state, audio);
        ImGui::Separator();

        ImGui::PushFont(font, 0.0f);
        drawGrid(state, audio);
        ImGui::PopFont();
    }
    ImGui::End();
}

void PatternEditor::drawToolbar(EditorState& state, TrackerAudio& audio)
{
    if (ImGui::Button("Play")) {
        audio.playSong(0, 0);
    }
    ImGui::SetItemTooltip("Play from the start (F5)");
    ImGui::SameLine();
    if (ImGui::Button("Play Frame")) {
        audio.playSong(state.cursor.frame, 0);
    }
    ImGui::SetItemTooltip("Play from the start of the cursor's frame (F6 / Enter)");
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
        audio.stopSong();
    }
    ImGui::SetItemTooltip("Stop (F8)");

    ImGui::SameLine(0.0f, 24.0f);
    ImGui::SetNextItemWidth(90.0f);
    ImGui::SliderInt("Octave", &state.octave, 0, MAX_OCTAVE);
    ImGui::SetItemTooltip("Also changed with / and * on the keypad");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90.0f);
    ImGui::SliderInt("Step", &state.editStep, 0, MAX_EDIT_STEP);
    ImGui::SetItemTooltip("Rows the cursor moves after an entry");
    ImGui::SameLine();
    ImGui::Checkbox("Edit Mode", &state.editMode);
    ImGui::SetItemTooltip("When off, note keys only preview (Space)");
    ImGui::SameLine();
    ImGui::Checkbox("Follow", &state.followPlayback);
    ImGui::SetItemTooltip("Moves the cursor with the playback position");
}

void PatternEditor::drawGrid(EditorState& state, TrackerAudio& audio)
{
    const float charWidth = ImGui::CalcTextSize("0").x;
    const ImGuiTableFlags flags = ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
                                  ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit;
    if (!ImGui::BeginTable("##pattern", 1 + static_cast<int>(CHANNEL_COUNT), flags)) {
        return;
    }

    ImGui::TableSetupScrollFreeze(1, 1);
    ImGui::TableSetupColumn("##row", ImGuiTableColumnFlags_WidthFixed, charWidth * 2);
    for (size_t channel = 0; channel < CHANNEL_COUNT; channel++) {
        ImGui::TableSetupColumn(getChannelName(static_cast<ChannelType>(channel)),
                                ImGuiTableColumnFlags_WidthFixed,
                                charWidth * getCellChars(state.song.effectColumns[channel]));
    }

    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
    ImGui::TableSetColumnIndex(0);
    ImGui::TableHeader("##row");
    for (int channel = 0; channel < static_cast<int>(CHANNEL_COUNT); channel++) {
        ImGui::TableSetColumnIndex(channel + 1);
        drawChannelHeader(state, audio, channel);
    }

    const bool showPlayback = state.playback.playing && state.playback.frame == state.cursor.frame;
    for (int row = 0; row < state.song.rowsPerPattern; row++) {
        ImGui::TableNextRow();
        ImGui::PushID(row);

        if (row % 16 == 0) {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, MEASURE_ROW_COLOR);
        }
        else if (row % 4 == 0) {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, BEAT_ROW_COLOR);
        }
        if (showPlayback && row == state.playback.row) {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, PLAYBACK_ROW_COLOR);
        }
        else if (row == state.cursor.row) {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1,
                                   state.editMode ? CURSOR_EDIT_ROW_COLOR : CURSOR_ROW_COLOR);
        }

        ImGui::TableSetColumnIndex(0);
        ImGui::PushStyleColor(ImGuiCol_Text, row % 4 == 0 ? NOTE_COLOR : EMPTY_COLOR);
        ImGui::Text("%02X", row);
        ImGui::PopStyleColor();

        for (int channel = 0; channel < static_cast<int>(CHANNEL_COUNT); channel++) {
            ImGui::TableSetColumnIndex(channel + 1);
            drawCell(state, row, channel, charWidth);
        }

        if (m_scrollToCursor && row == state.cursor.row) {
            ImGui::SetScrollHereY(0.5f);
        }
        ImGui::PopID();
    }
    m_scrollToCursor = false;

    ImGui::EndTable();
}

void PatternEditor::drawChannelHeader(EditorState& state, TrackerAudio& audio, int channel)
{
    const auto type = static_cast<ChannelType>(channel);
    std::string label = getChannelName(type);
    if (state.muted[channel]) {
        label += " [M]";
    }
    label += "###header";

    ImGui::PushID(channel);
    ImGui::TableHeader(label.c_str());
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        toggleMute(state, audio, channel);
    }
    if (type == ChannelType::Dpcm) {
        ImGui::SetItemTooltip("DPCM is not played yet\nClick: mute / Right-click: menu");
    }
    else {
        ImGui::SetItemTooltip("Click: mute / Right-click: menu");
    }

    if (ImGui::BeginPopupContextItem("##header_menu")) {
        if (ImGui::MenuItem("Mute", nullptr, state.muted[channel])) {
            toggleMute(state, audio, channel);
        }
        int columns = state.song.effectColumns[channel];
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::SliderInt("Effect Columns", &columns, 1, static_cast<int>(MAX_EFFECT_COLUMNS))) {
            state.song.effectColumns[channel] = static_cast<uint8_t>(columns);
            state.clampCursor();
            state.markModified();
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
}

void PatternEditor::drawCell(EditorState& state, int row, int channel, float charWidth)
{
    const auto type = static_cast<ChannelType>(channel);
    const int effectColumns = state.song.effectColumns[channel];
    const float lineHeight = ImGui::GetTextLineHeight();
    const ImVec2 origin = ImGui::GetCursorScreenPos();

    // Drawing must not add patterns, so missing ones are shown as empty
    static const Cell EMPTY_CELL;
    const Pattern* pattern =
        state.song.findPattern(type, state.song.frames[state.cursor.frame][channel]);
    const Cell& cell =
        (pattern != nullptr && row < static_cast<int>(pattern->rows.size())) ? pattern->rows[row]
                                                                             : EMPTY_CELL;

    ImGui::PushID(channel);
    ImGui::InvisibleButton("##cell", ImVec2(charWidth * getCellChars(effectColumns), lineHeight));
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        const int charIndex = static_cast<int>((ImGui::GetIO().MousePos.x - origin.x) / charWidth);
        state.cursor.row = row;
        state.cursor.channel = channel;
        state.cursor.column = getColumnAtChar(charIndex, state.getColumnCount(channel));
    }
    ImGui::PopID();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (row == state.cursor.row && channel == state.cursor.channel) {
        const CharRange range = getColumnChars(state.cursor.column);
        const ImVec2 min(origin.x + range.start * charWidth, origin.y);
        const ImVec2 max(min.x + range.count * charWidth, origin.y + lineHeight);
        drawList->AddRectFilled(min, max, state.editMode ? CURSOR_EDIT_COLOR : CURSOR_COLOR);
    }

    const bool muted = state.muted[channel];
    auto drawText = [&](int charIndex, const std::string& text, ImU32 color, bool empty) {
        ImU32 textColor = empty ? EMPTY_COLOR : color;
        if (muted) {
            textColor = dim(textColor);
        }
        drawList->AddText(ImVec2(origin.x + charIndex * charWidth, origin.y), textColor,
                          text.c_str());
    };

    drawText(0, noteToString(type, cell.note), NOTE_COLOR, cell.note == NOTE_NONE);
    drawText(4, cell.instrument == VALUE_NONE ? ".." : std::format("{:02X}", cell.instrument),
             INSTRUMENT_COLOR, cell.instrument == VALUE_NONE);
    drawText(7, cell.volume == VALUE_NONE ? "." : std::format("{:X}", cell.volume), VOLUME_COLOR,
             cell.volume == VALUE_NONE);
    for (int effect = 0; effect < effectColumns; effect++) {
        drawText(9 + 4 * effect, effectToString(cell.effects[effect]), EFFECT_COLOR,
                 cell.effects[effect].type == EffectType::None);
    }
}

void PatternEditor::handleKeyboard(EditorState& state, TrackerAudio& audio)
{
    stopPreviewOnKeyRelease(audio);

    // Keys go to the pattern unless a text field or a popup takes them
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput || io.KeyCtrl || io.KeyAlt ||
        ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel)) {
        return;
    }

    handleNavigation(state);
    if (getField(state.cursor.column).type == FieldType::Note) {
        handleNoteInput(state, audio);
    }
    else if (state.editMode) {
        handleValueInput(state);
    }
}

void PatternEditor::handleNavigation(EditorState& state)
{
    const int lastRow = state.song.rowsPerPattern - 1;

    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        moveRows(state, -1, true);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        moveRows(state, 1, true);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) {
        moveRows(state, -PAGE_ROWS, false);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_PageDown)) {
        moveRows(state, PAGE_ROWS, false);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Home)) {
        moveRows(state, -lastRow, false);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_End)) {
        moveRows(state, lastRow, false);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        moveColumn(state, -1);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        moveColumn(state, 1);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
        const int channelCount = static_cast<int>(CHANNEL_COUNT);
        const int delta = ImGui::GetIO().KeyShift ? channelCount - 1 : 1;
        state.cursor.channel = (state.cursor.channel + delta) % channelCount;
        state.cursor.column = 0;
        m_scrollToCursor = true;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
        state.editMode = !state.editMode;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_KeypadDivide)) {
        state.octave = std::max(state.octave - 1, 0);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_KeypadMultiply)) {
        state.octave = std::min(state.octave + 1, MAX_OCTAVE);
    }

    if (state.editMode && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        clearField(state);
        state.markModified();
        advanceAfterEdit(state);
    }
}

void PatternEditor::handleNoteInput(EditorState& state, TrackerAudio& audio)
{
    for (const NoteKey& noteKey : NOTE_KEYS) {
        if (!ImGui::IsKeyPressed(noteKey.key, false)) {
            continue;
        }
        const int semitone = state.octave * 12 + noteKey.semitone;
        if (semitone >= NOTE_MAX) {
            return;
        }
        // The noise channel has 16 pitches, which repeat over the keyboard as in FamiTracker
        const uint8_t note = (state.getCursorChannel() == ChannelType::Noise)
                                 ? static_cast<uint8_t>(semitone % NOISE_NOTE_MAX + 1)
                                 : static_cast<uint8_t>(semitone + 1);
        enterNote(state, audio, note, noteKey.key);
        return;
    }

    if (ImGui::IsKeyPressed(NOTE_OFF_KEY, false)) {
        enterNote(state, audio, NOTE_OFF, NOTE_OFF_KEY);
    }
    else if (ImGui::IsKeyPressed(NOTE_RELEASE_KEY, false)) {
        enterNote(state, audio, NOTE_RELEASE, NOTE_RELEASE_KEY);
    }
}

void PatternEditor::handleValueInput(EditorState& state)
{
    const Field field = getField(state.cursor.column);
    Cell& cell = state.getCursorCell();

    if (field.type == FieldType::EffectType) {
        for (EffectType type : EFFECT_TYPES) {
            const char letter = effectTypeToLetter(type);
            if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_A + (letter - 'A')), false)) {
                cell.effects[field.effect].type = type;
                state.markModified();
                advanceAfterEdit(state);
                return;
            }
        }
        return;
    }

    const std::optional<int> digit = getPressedHexDigit();
    if (!digit) {
        return;
    }

    switch (field.type) {
    case FieldType::Instrument: {
        const uint8_t value = (cell.instrument == VALUE_NONE) ? 0 : cell.instrument;
        cell.instrument =
            std::min(setDigit(value, field.digit, *digit), static_cast<uint8_t>(MAX_INSTRUMENTS - 1));
        break;
    }
    case FieldType::Volume: cell.volume = static_cast<uint8_t>(*digit); break;
    case FieldType::EffectParam: {
        Effect& effect = cell.effects[field.effect];
        if (effect.type == EffectType::None) {
            return;
        }
        effect.param = setDigit(effect.param, field.digit, *digit);
        break;
    }
    default: return;
    }
    state.markModified();
    advanceAfterEdit(state);
}

void PatternEditor::enterNote(EditorState& state, TrackerAudio& audio, uint8_t note, ImGuiKey key)
{
    const ChannelType channel = state.getCursorChannel();
    if (m_previewKey != ImGuiKey_None) {
        audio.stopNote(m_previewChannel);
        m_previewKey = ImGuiKey_None;
    }

    // Preview, also when not in edit mode
    if (note == NOTE_OFF) {
        audio.stopNote(channel);
    }
    else if (note == NOTE_RELEASE) {
        audio.playNote(channel, NOTE_RELEASE, VALUE_NONE);
    }
    else {
        audio.playNote(channel, note, state.getCurrentInstrument());
        m_previewKey = key;
        m_previewChannel = channel;
    }

    if (!state.editMode) {
        return;
    }
    Cell& cell = state.getCursorCell();
    cell.note = note;
    if (note != NOTE_OFF && note != NOTE_RELEASE) {
        cell.instrument = state.getCurrentInstrument();
    }
    state.markModified();
    advanceAfterEdit(state);
}

void PatternEditor::clearField(EditorState& state)
{
    Cell& cell = state.getCursorCell();
    const Field field = getField(state.cursor.column);
    switch (field.type) {
    case FieldType::Note:
        cell.note = NOTE_NONE;
        cell.instrument = VALUE_NONE;
        cell.volume = VALUE_NONE;
        break;
    case FieldType::Instrument: cell.instrument = VALUE_NONE; break;
    case FieldType::Volume:     cell.volume = VALUE_NONE; break;
    default:                    cell.effects[field.effect] = Effect{}; break;
    }
}

void PatternEditor::moveRows(EditorState& state, int delta, bool acrossFrames)
{
    const int rowCount = state.song.rowsPerPattern;
    const int frameCount = static_cast<int>(state.song.frames.size());
    int row = state.cursor.row + delta;
    int frame = state.cursor.frame;

    if (acrossFrames) {
        while (row < 0) {
            row += rowCount;
            frame--;
        }
        while (row >= rowCount) {
            row -= rowCount;
            frame++;
        }
        frame = (frame % frameCount + frameCount) % frameCount;
    }
    else {
        row = std::clamp(row, 0, rowCount - 1);
    }

    state.cursor.row = row;
    state.cursor.frame = frame;
    m_scrollToCursor = true;
}

void PatternEditor::moveColumn(EditorState& state, int delta)
{
    const int channelCount = static_cast<int>(CHANNEL_COUNT);
    Cursor& cursor = state.cursor;
    cursor.column += delta;
    if (cursor.column < 0) {
        cursor.channel = (cursor.channel + channelCount - 1) % channelCount;
        cursor.column = state.getColumnCount(cursor.channel) - 1;
    }
    else if (cursor.column >= state.getColumnCount(cursor.channel)) {
        cursor.channel = (cursor.channel + 1) % channelCount;
        cursor.column = 0;
    }
}

void PatternEditor::advanceAfterEdit(EditorState& state)
{
    state.cursor.row = std::min(state.cursor.row + state.editStep, state.song.rowsPerPattern - 1);
    m_scrollToCursor = true;
}

void PatternEditor::stopPreviewOnKeyRelease(TrackerAudio& audio)
{
    if (m_previewKey != ImGuiKey_None && !ImGui::IsKeyDown(m_previewKey)) {
        audio.stopNote(m_previewChannel);
        m_previewKey = ImGuiKey_None;
    }
}

} // namespace ocf::tracker_tool
