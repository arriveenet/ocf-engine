// SPDX-License-Identifier: MIT
#include "InstrumentEditor.h"

#include "TrackerAudio.h"

#include <ocf/audio/tracker/SongNotation.h>

#include <imgui.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <format>
#include <optional>
#include <vector>

namespace ocf::tracker_tool {

namespace {

constexpr uint8_t PREVIEW_NOTE = 49; // C-4
constexpr ChannelType PREVIEW_CHANNEL = ChannelType::Pulse1;

struct SequenceInfo {
    const char* label;
    const char* emptyText;
    int maxValue;
};

SequenceInfo getSequenceInfo(SequenceType type)
{
    switch (type) {
    case SequenceType::Volume: return {"Volume (0-15)", "Empty: always 15", MAX_VOLUME};
    default:                   return {"Duty (0-3)", "Empty: always 0 (12.5%)", 3};
    }
}

/** Removes an instrument and renumbers the notes that use the later ones. */
void removeInstrument(Song& song, int index)
{
    song.instruments.erase(song.instruments.begin() + index);
    for (std::vector<Pattern>& pool : song.patterns) {
        for (Pattern& pattern : pool) {
            for (Cell& cell : pattern.rows) {
                if (cell.instrument == VALUE_NONE || cell.instrument < index) {
                    continue;
                }
                cell.instrument =
                    (cell.instrument == index) ? VALUE_NONE : static_cast<uint8_t>(cell.instrument - 1);
            }
        }
    }
}

} // namespace

void InstrumentEditor::draw(EditorState& state, TrackerAudio& audio)
{
    if (ImGui::Begin("Instruments")) {
        drawList(state);
        ImGui::Separator();

        const uint8_t current = state.getCurrentInstrument();
        if (current == VALUE_NONE) {
            ImGui::TextDisabled("No instruments");
        }
        else {
            syncText(state);
            Instrument& instrument = state.song.instruments[current];
            if (ImGui::InputText("Name", &instrument.name)) {
                state.markModified();
            }
            for (size_t type = 0; type < SEQUENCE_TYPE_COUNT; type++) {
                drawSequence(state, instrument, static_cast<SequenceType>(type));
            }

            ImGui::Spacing();
            ImGui::Button("Preview (C-4)");
            if (ImGui::IsItemActivated()) {
                audio.playNote(PREVIEW_CHANNEL, PREVIEW_NOTE, current);
            }
            if (ImGui::IsItemDeactivated()) {
                audio.stopNote(PREVIEW_CHANNEL);
            }
            ImGui::SetItemTooltip("Plays on Pulse 1 while held");
        }
    }
    ImGui::End();
}

void InstrumentEditor::drawList(EditorState& state)
{
    std::vector<Instrument>& instruments = state.song.instruments;
    const uint8_t current = state.getCurrentInstrument();

    ImGui::BeginDisabled(instruments.size() >= MAX_INSTRUMENTS);
    if (ImGui::Button("Add")) {
        Instrument instrument;
        instrument.name = "Instrument";
        instruments.push_back(instrument);
        state.currentInstrument = static_cast<int>(instruments.size()) - 1;
        state.markModified();
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(current == VALUE_NONE);
    if (ImGui::Button("Duplicate")) {
        Instrument copy = instruments[current];
        instruments.push_back(std::move(copy));
        state.currentInstrument = static_cast<int>(instruments.size()) - 1;
        state.markModified();
    }
    ImGui::EndDisabled();
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(current == VALUE_NONE);
    if (ImGui::Button("Remove")) {
        removeInstrument(state.song, current);
        state.currentInstrument =
            std::min(state.currentInstrument, static_cast<int>(instruments.size()) - 1);
        m_textInstrument = -1; // Another instrument may now have this number
        state.markModified();
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("Also renumbers the instruments used in the patterns");

    const ImVec2 size(-FLT_MIN, 6.0f * ImGui::GetTextLineHeightWithSpacing());
    if (ImGui::BeginListBox("##instruments", size)) {
        for (int i = 0; i < static_cast<int>(instruments.size()); i++) {
            const std::string label = std::format("{:02X} {}##{}", i, instruments[i].name, i);
            if (ImGui::Selectable(label.c_str(), i == state.currentInstrument)) {
                state.currentInstrument = i;
            }
        }
        ImGui::EndListBox();
    }
}

void InstrumentEditor::drawSequence(EditorState& state, Instrument& instrument, SequenceType type)
{
    const size_t index = toIndex(type);
    const SequenceInfo info = getSequenceInfo(type);

    ImGui::PushID(static_cast<int>(index));
    ImGui::SeparatorText(info.label);

    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputTextWithHint("##mml", "e.g. 15 12 | 10 8 / 4 0", &m_mmlText[index])) {
        std::optional<Sequence> sequence = sequenceFromMml(m_mmlText[index]);
        m_mmlValid[index] = sequence.has_value();
        if (sequence) {
            for (int8_t& value : sequence->values) {
                value = static_cast<int8_t>(std::clamp<int>(value, 0, info.maxValue));
            }
            instrument.getSequence(type) = std::move(*sequence);
            state.markModified();
        }
    }
    ImGui::SetItemTooltip("One value per tick. The loop point follows |, the release point follows /");
    if (!m_mmlValid[index]) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Invalid sequence");
    }

    const Sequence& sequence = instrument.getSequence(type);
    if (sequence.isEmpty()) {
        ImGui::TextDisabled("%s", info.emptyText);
    }
    else {
        const std::vector<float> values(sequence.values.begin(), sequence.values.end());
        ImGui::PlotHistogram("##graph", values.data(), static_cast<int>(values.size()), 0, nullptr,
                             0.0f, static_cast<float>(info.maxValue), ImVec2(-FLT_MIN, 48.0f));
        const std::string loop =
            sequence.loopPoint >= 0 ? std::to_string(sequence.loopPoint) : std::string("-");
        const std::string release =
            sequence.releasePoint >= 0 ? std::to_string(sequence.releasePoint) : std::string("-");
        ImGui::TextDisabled("Length: %zu  Loop: %s  Release: %s", sequence.values.size(),
                            loop.c_str(), release.c_str());
    }
    ImGui::PopID();
}

void InstrumentEditor::syncText(const EditorState& state)
{
    if (m_textInstrument == state.currentInstrument && m_textRevision == state.songRevision) {
        return;
    }
    m_textInstrument = state.currentInstrument;
    m_textRevision = state.songRevision;

    const Instrument& instrument = state.song.instruments[state.currentInstrument];
    for (size_t type = 0; type < SEQUENCE_TYPE_COUNT; type++) {
        m_mmlText[type] = sequenceToMml(instrument.sequences[type]);
        m_mmlValid[type] = true;
    }
}

} // namespace ocf::tracker_tool
