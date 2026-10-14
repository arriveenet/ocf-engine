// SPDX-License-Identifier: MIT
#include "InstrumentEditor.h"

#include "TrackerAudio.h"

#include <ocf/audio/tracker/SongNotation.h>

#include <string>
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

std::string getSampleLabel(const Song& song, uint8_t sample)
{
    if (sample >= song.dpcmSamples.size()) {
        return std::format("{:02X} (missing)", sample);
    }
    return std::format("{:02X} {}", sample, song.dpcmSamples[sample].name);
}

std::string getKeyLabel(const Song& song, const Instrument& instrument, uint8_t note)
{
    const std::string name = noteToString(ChannelType::Dpcm, note);
    const DpcmKey* key = instrument.findDpcmKey(note);
    if (key == nullptr || !key->isAssigned()) {
        return name;
    }
    return std::format("{}  {}  rate {}{}", name, getSampleLabel(song, key->sample), key->rate,
                       key->loop ? "  loop" : "");
}

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

            drawDpcmKeys(state, instrument, audio);
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

void InstrumentEditor::drawDpcmKeys(EditorState& state, Instrument& instrument, TrackerAudio& audio)
{
    ImGui::Spacing();
    if (!ImGui::CollapsingHeader("DPCM Keys", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    const Song& song = state.song;
    if (song.dpcmSamples.empty()) {
        ImGui::TextDisabled("No samples");
        ImGui::SameLine();
        if (ImGui::SmallButton("DPCM Samples...")) {
            state.showDpcmSamples = true;
        }
        return;
    }

    // The note played on the DPCM channel, also chosen by playing a note there
    uint8_t& note = state.dpcmKeyNote;
    note = std::clamp(note, NOTE_MIN, NOTE_MAX);
    if (ImGui::BeginCombo("Key", getKeyLabel(song, instrument, note).c_str())) {
        for (int n = NOTE_MIN; n <= NOTE_MAX; n++) {
            const auto value = static_cast<uint8_t>(n);
            if (ImGui::Selectable(getKeyLabel(song, instrument, value).c_str(), value == note)) {
                note = value;
            }
            if (value == note) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("Also chosen by entering a note on the DPCM channel");

    DpcmKey& key = *instrument.findDpcmKey(note);
    const std::string sampleLabel = key.isAssigned() ? getSampleLabel(song, key.sample) : "(none)";
    if (ImGui::BeginCombo("Sample", sampleLabel.c_str())) {
        if (ImGui::Selectable("(none)", !key.isAssigned())) {
            key = DpcmKey{};
            state.markModified();
        }
        for (int i = 0; i < static_cast<int>(song.dpcmSamples.size()); i++) {
            if (ImGui::Selectable(getSampleLabel(song, static_cast<uint8_t>(i)).c_str(), key.sample == i)) {
                // A new key starts at the rate the sample was made for
                if (!key.isAssigned()) {
                    key.rate = song.dpcmSamples[i].rate;
                }
                key.sample = static_cast<uint8_t>(i);
                state.markModified();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::BeginDisabled(!key.isAssigned());
    int rate = key.rate;
    if (ImGui::SliderInt("Rate", &rate, 0, DPCM_MAX_RATE)) {
        key.rate = static_cast<uint8_t>(rate);
        state.markModified();
    }
    ImGui::SetItemTooltip("%.1f kHz. 15 is the fastest", getDpcmRateFrequency(key.rate) / 1000.0);
    if (ImGui::Checkbox("Loop", &key.loop)) {
        state.markModified();
    }
    ImGui::SetItemTooltip("Plays the sample again from the start until the note is released (===)");
    ImGui::SameLine();
    ImGui::Button("Preview##dpcm");
    if (ImGui::IsItemActivated()) {
        audio.playNote(ChannelType::Dpcm, note, state.getCurrentInstrument());
    }
    if (ImGui::IsItemDeactivated()) {
        audio.stopNote(ChannelType::Dpcm);
    }
    ImGui::SetItemTooltip("Plays the key on the DPCM channel while held");
    ImGui::EndDisabled();

    // The assigned keys, to choose one
    for (int n = NOTE_MIN; n <= NOTE_MAX; n++) {
        const auto value = static_cast<uint8_t>(n);
        if (!instrument.findDpcmKey(value)->isAssigned()) {
            continue;
        }
        const std::string label = getKeyLabel(song, instrument, value) + "##key" + std::to_string(n);
        if (ImGui::Selectable(label.c_str(), value == note)) {
            note = value;
        }
    }
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
