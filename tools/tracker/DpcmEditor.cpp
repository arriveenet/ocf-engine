// SPDX-License-Identifier: MIT
#include "DpcmEditor.h"

#include "PathUtility.h"
#include "TrackerAudio.h"

#include <ocf/platform/FileSystem.h>

#include <imgui.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <format>
#include <fstream>
#include <iterator>
#include <utility>
#include <vector>

namespace ocf::tracker_tool {

using namespace audio::tracker;

namespace {

constexpr ImVec4 ERROR_COLOR(1.0f, 0.4f, 0.4f, 1.0f);
constexpr int PLOT_POINTS = 400;
constexpr float MIN_GAIN_DB = -24.0f;
constexpr float MAX_GAIN_DB = 12.0f;
constexpr const char* DMC_EXTENSION = ".dmc";

std::string getRateLabel(int rate)
{
    return std::format("{} ({:.1f} kHz)", rate,
                       getDpcmRateFrequency(static_cast<uint8_t>(rate)) / 1000.0);
}

bool isDmcFile(const std::filesystem::path& path)
{
    std::string extension = pathToUtf8(path.extension());
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return extension == DMC_EXTENSION;
}

/** .dmc files are only the bytes. A length that $4013 cannot play is padded with alternating bits. */
Result<std::vector<uint8_t>, std::string> readDmcFile(const std::filesystem::path& path)
{
    using DmcResult = Result<std::vector<uint8_t>, std::string>;
    std::ifstream file(path, std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (!file && !file.eof()) {
        return DmcResult::Err("cannot read the file");
    }
    if (data.empty()) {
        return DmcResult::Err("the file is empty");
    }
    if (data.size() > DPCM_MAX_SAMPLE_BYTES) {
        return DmcResult::Err(std::format("{} bytes is longer than {}", data.size(), DPCM_MAX_SAMPLE_BYTES));
    }
    data.resize(getDpcmPlayableLength(data.size()), 0x55);
    return DmcResult::Ok(std::move(data));
}

/** Whether the samples fit in the memory with a sample of `bytes`, replacing the sample `replacing`. */
bool fitsInMemory(const Song& song, size_t bytes, int replacing, size_t& needed)
{
    size_t used = song.getDpcmMemoryUsed();
    if (replacing >= 0) {
        used -= getDpcmAlignedSize(song.dpcmSamples[replacing].data.size());
    }
    needed = used + getDpcmAlignedSize(bytes);
    return needed <= DPCM_MEMORY_SIZE;
}

std::string describeResult(const DpcmEncodeResult& result)
{
    const std::string snr =
        std::isinf(result.snrDb) ? std::string("inf") : std::format("{:.1f}", result.snrDb);
    return std::format("rate {}, {} bytes, {:.2f} s, SNR {} dB, overload {:.1f}%{}", result.rate,
                       result.data.size(),
                       result.data.size() * 8 / getDpcmRateFrequency(result.rate), snr,
                       result.overloadRatio * 100.0, result.truncated ? " (truncated)" : "");
}

} // namespace

DpcmEditor::DpcmEditor()
    : m_directory(FileSystem::getInstance()->getAssetPath())
{
}

void DpcmEditor::draw(EditorState& state, TrackerAudio& audio)
{
    // Outside of the window, so that the dialog stays when the window is closed
    drawFileDialog(state);

    if (!state.showDpcmSamples) {
        return;
    }
    if (ImGui::Begin("DPCM Samples", &state.showDpcmSamples)) {
        state.currentDpcmSample = std::clamp(state.currentDpcmSample, 0,
                                             std::max(0, static_cast<int>(state.song.dpcmSamples.size()) - 1));
        drawList(state);
        ImGui::Separator();
        drawSample(state, audio);
        drawImportSettings(state);

        if (!m_message.empty()) {
            ImGui::Separator();
            if (m_messageIsError) {
                ImGui::PushStyleColor(ImGuiCol_Text, ERROR_COLOR);
            }
            ImGui::TextWrapped("%s", m_message.c_str());
            if (m_messageIsError) {
                ImGui::PopStyleColor();
            }
        }
    }
    ImGui::End();
}

void DpcmEditor::drawList(EditorState& state)
{
    std::vector<DpcmSample>& samples = state.song.dpcmSamples;
    const bool hasCurrent = !samples.empty();

    ImGui::BeginDisabled(samples.size() >= MAX_DPCM_SAMPLES);
    if (ImGui::Button("Import...")) {
        m_fileDialog.open(FileDialog::Mode::Open, "Import Sample", m_directory,
                          {".wav", ".flac", ".mp3", DMC_EXTENSION});
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("WAV, FLAC and MP3 are converted with the import settings below");
    ImGui::SameLine();
    ImGui::BeginDisabled(!hasCurrent);
    if (ImGui::Button("Export...")) {
        m_fileDialog.open(FileDialog::Mode::Save, "Export Sample", m_directory, {DMC_EXTENSION},
                          samples[state.currentDpcmSample].name + DMC_EXTENSION);
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove")) {
        removeSample(state);
    }
    ImGui::SetItemTooltip("Also removes the keys that play it");
    ImGui::EndDisabled();

    const size_t used = state.song.getDpcmMemoryUsed();
    const std::string memory = std::format("{} / {} bytes", used, DPCM_MEMORY_SIZE);
    ImGui::ProgressBar(static_cast<float>(used) / DPCM_MEMORY_SIZE, ImVec2(-FLT_MIN, 0.0f),
                       memory.c_str());
    ImGui::SetItemTooltip("The samples share 16KB, each taking its length rounded up to 64 bytes");

    const ImVec2 size(-FLT_MIN, 6.0f * ImGui::GetTextLineHeightWithSpacing());
    if (ImGui::BeginListBox("##samples", size)) {
        for (int i = 0; i < static_cast<int>(samples.size()); i++) {
            const std::string label =
                std::format("{:02X} {} ({} bytes)##{}", i, samples[i].name, samples[i].data.size(), i);
            if (ImGui::Selectable(label.c_str(), i == state.currentDpcmSample)) {
                state.currentDpcmSample = i;
            }
        }
        ImGui::EndListBox();
    }
}

void DpcmEditor::drawSample(EditorState& state, TrackerAudio& audio)
{
    std::vector<DpcmSample>& samples = state.song.dpcmSamples;
    if (samples.empty()) {
        ImGui::TextDisabled("No samples. Import a WAV, FLAC, MP3 or .dmc file");
        return;
    }
    const int index = state.currentDpcmSample;
    DpcmSample& sample = samples[index];

    if (ImGui::InputText("Name", &sample.name)) {
        state.markModified();
    }

    if (ImGui::BeginCombo("Rate", getRateLabel(sample.rate).c_str())) {
        for (int rate = DPCM_MAX_RATE; rate >= 0; rate--) {
            if (ImGui::Selectable(getRateLabel(rate).c_str(), rate == sample.rate)) {
                sample.rate = static_cast<uint8_t>(rate);
                state.markModified();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("The rate it was made for. New keys start with it");

    int restLevel = sample.restLevel;
    if (ImGui::SliderInt("Rest Level", &restLevel, 0, DPCM_LEVEL_MAX)) {
        sample.restLevel = static_cast<uint8_t>(restLevel);
        state.markModified();
    }
    ImGui::SetItemTooltip("The level it starts from: $4011 is set to it when the sample starts");

    // The levels as the APU plays them
    const std::vector<uint8_t> levels = decodeDpcm(sample.data, sample.restLevel);
    if (!levels.empty()) {
        std::vector<float> points(std::min<size_t>(PLOT_POINTS, levels.size()));
        for (size_t i = 0; i < points.size(); i++) {
            points[i] = levels[i * levels.size() / points.size()];
        }
        ImGui::PlotLines("##waveform", points.data(), static_cast<int>(points.size()), 0, nullptr,
                         0.0f, static_cast<float>(DPCM_LEVEL_MAX), ImVec2(-FLT_MIN, 64.0f));
    }
    ImGui::TextDisabled("%zu bytes, %.2f s at rate %d", sample.data.size(),
                        sample.data.size() * 8 / getDpcmRateFrequency(sample.rate), sample.rate);

    ImGui::Button("Preview");
    if (ImGui::IsItemActivated()) {
        audio.playDpcmSample(DpcmKey{static_cast<uint8_t>(index), sample.rate, false});
    }
    if (ImGui::IsItemDeactivated()) {
        audio.stopNote(ChannelType::Dpcm);
    }
    ImGui::SetItemTooltip("Plays at its rate while held");
}

void DpcmEditor::drawImportSettings(EditorState& state)
{
    if (!ImGui::CollapsingHeader("Import Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    const std::string rateLabel = (m_rate < 0) ? std::string("Auto") : getRateLabel(m_rate);
    if (ImGui::BeginCombo("Rate##import", rateLabel.c_str())) {
        if (ImGui::Selectable("Auto", m_rate < 0)) {
            m_rate = -1;
        }
        for (int rate = DPCM_MAX_RATE; rate >= 0; rate--) {
            if (ImGui::Selectable(getRateLabel(rate).c_str(), rate == m_rate)) {
                m_rate = rate;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("Auto: the highest rate that fits in 4081 bytes");

    ImGui::Checkbox("Normalize", &m_normalize);
    ImGui::SetItemTooltip("Scales the peak to full scale, before the gain");
    ImGui::SliderFloat("Gain", &m_gainDb, MIN_GAIN_DB, MAX_GAIN_DB, "%.1f dB");
    ImGui::SetItemTooltip("Loud high sounds cannot be followed: lower it when the overload is high");
    ImGui::Checkbox("Trim", &m_trim);
    ImGui::SetItemTooltip("Removes the quiet start and end, so that the sample starts at once");
    if (m_trim) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120.0f);
        ImGui::SliderFloat("##trim", &m_trimThresholdDb, -80.0f, -20.0f, "%.0f dBFS");
    }
    ImGui::SliderInt("Level##import", &m_restLevel, 0, DPCM_LEVEL_MAX);
    ImGui::SetItemTooltip("The center of the waveform, where it starts and ends");
    ImGui::Checkbox("Greedy", &m_greedy);
    ImGui::SetItemTooltip("Off: the bits with the least error. On: as FamiTracker converts");
    ImGui::SameLine();
    ImGui::Checkbox("Truncate", &m_truncate);
    ImGui::SetItemTooltip("Cuts samples that are too long, instead of failing");

    ImGui::BeginDisabled(!hasSource(state));
    if (ImGui::Button("Convert Again")) {
        reconvert(state);
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("Converts the last imported file again with these settings, "
                          "while it is the selected sample");
}

void DpcmEditor::drawFileDialog(EditorState& state)
{
    if (!m_fileDialog.draw()) {
        return;
    }
    if (m_fileDialog.getMode() == FileDialog::Mode::Open) {
        importFile(state, m_fileDialog.getPath());
    }
    else {
        exportFile(state, m_fileDialog.getPath());
    }
}

void DpcmEditor::importFile(EditorState& state, const std::filesystem::path& path)
{
    m_directory = path.parent_path();
    Song& song = state.song;
    const std::string fileName = pathToUtf8(path.filename());
    if (song.dpcmSamples.size() >= MAX_DPCM_SAMPLES) {
        setMessage(std::format("A song has at most {} samples", MAX_DPCM_SAMPLES), true);
        return;
    }

    DpcmSample sample;
    sample.name = pathToUtf8(path.stem());
    std::optional<audio::MonoAudio> source;
    std::string message;
    if (isDmcFile(path)) {
        auto data = readDmcFile(path);
        if (data.isErr()) {
            setMessage(std::format("Could not import {}: {}", fileName, data.unwrapErr()), true);
            return;
        }
        sample.data = std::move(data).unwrap();
        message = std::format("Imported {}: {} bytes. Set its rate and rest level", fileName,
                              sample.data.size());
    }
    else {
        auto audio = audio::AudioFile::readMono(path);
        if (audio.isErr()) {
            setMessage(std::format("Could not import {}: {}", fileName, audio.unwrapErr()), true);
            return;
        }
        auto result = encode(audio.unwrap());
        if (result.isErr()) {
            setMessage(std::format("Could not convert {}: {}", fileName, result.unwrapErr()), true);
            return;
        }
        sample.data = std::move(result.unwrap().data);
        sample.rate = result.unwrap().rate;
        sample.restLevel = static_cast<uint8_t>(m_restLevel);
        message = std::format("Imported {}: {}", fileName, describeResult(result.unwrap()));
        source = std::move(audio).unwrap();
    }

    size_t needed = 0;
    if (!fitsInMemory(song, sample.data.size(), -1, needed)) {
        setMessage(std::format("Not enough DPCM memory for {}: the samples would take {} of {} bytes",
                               fileName, needed, DPCM_MEMORY_SIZE),
                   true);
        return;
    }

    song.dpcmSamples.push_back(std::move(sample));
    state.currentDpcmSample = static_cast<int>(song.dpcmSamples.size()) - 1;
    state.markModified();
    setMessage(std::move(message));

    m_source = std::move(source);
    m_sourceSample = state.currentDpcmSample;
    m_sourceSampleCount = song.dpcmSamples.size();
    m_sourceRevision = state.songRevision;
}

void DpcmEditor::exportFile(const EditorState& state, const std::filesystem::path& path)
{
    m_directory = path.parent_path();
    if (state.currentDpcmSample >= static_cast<int>(state.song.dpcmSamples.size())) {
        return;
    }
    const DpcmSample& sample = state.song.dpcmSamples[state.currentDpcmSample];
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(sample.data.data()),
               static_cast<std::streamsize>(sample.data.size()));
    if (!file) {
        setMessage(std::format("Could not write {}", pathToUtf8(path.filename())), true);
        return;
    }
    setMessage(std::format("Exported {} (rate {}, rest level {})", pathToUtf8(path.filename()),
                           sample.rate, sample.restLevel));
}

void DpcmEditor::reconvert(EditorState& state)
{
    auto result = encode(*m_source);
    if (result.isErr()) {
        setMessage(std::format("Could not convert: {}", result.unwrapErr()), true);
        return;
    }
    DpcmEncodeResult& encoded = result.unwrap();

    Song& song = state.song;
    size_t needed = 0;
    if (!fitsInMemory(song, encoded.data.size(), m_sourceSample, needed)) {
        setMessage(std::format("Not enough DPCM memory: the samples would take {} of {} bytes",
                               needed, DPCM_MEMORY_SIZE),
                   true);
        return;
    }

    const std::string description = describeResult(encoded);

    // The keys at the old rate of the sample follow it
    DpcmSample& sample = song.dpcmSamples[m_sourceSample];
    for (Instrument& instrument : song.instruments) {
        for (DpcmKey& key : instrument.dpcmKeys) {
            if (key.sample == m_sourceSample && key.rate == sample.rate) {
                key.rate = encoded.rate;
            }
        }
    }
    sample.data = std::move(encoded.data);
    sample.rate = encoded.rate;
    sample.restLevel = static_cast<uint8_t>(m_restLevel);
    state.markModified();
    setMessage(std::format("Converted again: {}", description));
}

void DpcmEditor::removeSample(EditorState& state)
{
    Song& song = state.song;
    const int index = state.currentDpcmSample;
    song.dpcmSamples.erase(song.dpcmSamples.begin() + index);

    // The keys of the removed sample are cleared, and those of the later ones renumbered
    for (Instrument& instrument : song.instruments) {
        for (DpcmKey& key : instrument.dpcmKeys) {
            if (!key.isAssigned() || key.sample < index) {
                continue;
            }
            if (key.sample == index) {
                key = DpcmKey{};
            }
            else {
                key.sample--;
            }
        }
    }

    if (m_sourceSample == index) {
        m_source.reset();
    }
    else if (m_sourceSample > index) {
        m_sourceSample--;
    }
    m_sourceSampleCount = song.dpcmSamples.size();

    state.currentDpcmSample = std::min(index, static_cast<int>(song.dpcmSamples.size()) - 1);
    state.markModified();
    m_message.clear();
}

Result<DpcmEncodeResult, std::string> DpcmEditor::encode(const audio::MonoAudio& audio) const
{
    DpcmEncodeOptions options;
    options.rate = (m_rate < 0) ? std::nullopt : std::optional<uint8_t>(static_cast<uint8_t>(m_rate));
    options.normalize = m_normalize;
    options.gain = std::pow(10.0f, m_gainDb / 20.0f);
    if (m_trim) {
        options.trimThresholdDb = m_trimThresholdDb;
    }
    options.center = static_cast<uint8_t>(m_restLevel);
    options.restLevel = static_cast<uint8_t>(m_restLevel);
    options.method = m_greedy ? DpcmEncodeMethod::Greedy : DpcmEncodeMethod::Optimal;
    options.truncate = m_truncate;
    return DpcmEncoder::encode(audio.samples, audio.sampleRate, options);
}

bool DpcmEditor::hasSource(const EditorState& state) const
{
    // Replaced or undone songs, and removed samples, lose the link
    return m_source && m_sourceSample == state.currentDpcmSample &&
           m_sourceSampleCount == state.song.dpcmSamples.size() &&
           m_sourceRevision == state.songRevision;
}

void DpcmEditor::setMessage(std::string message, bool error)
{
    m_message = std::move(message);
    m_messageIsError = error;
}

} // namespace ocf::tracker_tool
