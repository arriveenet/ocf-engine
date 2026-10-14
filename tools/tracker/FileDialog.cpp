// SPDX-License-Identifier: MIT
#include "FileDialog.h"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <optional>
#include <system_error>
#include <utility>

namespace ocf::tracker_tool {

namespace {

constexpr ImVec4 MESSAGE_COLOR(1.0f, 0.4f, 0.4f, 1.0f);

} // namespace

std::string pathToUtf8(const std::filesystem::path& path)
{
    const std::u8string text = path.u8string();
    return std::string(text.begin(), text.end());
}

std::filesystem::path pathFromUtf8(const std::string& text)
{
    return std::filesystem::path(std::u8string(text.begin(), text.end()));
}

FileDialog::FileDialog(std::string id)
    : m_id(std::move(id))
{
}

void FileDialog::open(Mode mode, const std::string& title, const std::filesystem::path& directory,
                      std::vector<std::string> extensions, const std::string& fileName)
{
    m_title = title + "###" + m_id;
    m_extensions = std::move(extensions);
    m_mode = mode;
    m_openRequested = true;
    m_fileName = fileName;
    m_confirmReplace = false;
    setDirectory(directory);
}

bool FileDialog::draw()
{
    const char* title = m_title.c_str();
    if (m_openRequested) {
        ImGui::OpenPopup(title);
        m_openRequested = false;
    }

    ImGui::SetNextWindowSize(ImVec2(600.0f, 440.0f), ImGuiCond_Appearing);
    if (!ImGui::BeginPopupModal(title)) {
        return false;
    }

    if (ImGui::Button("Up")) {
        setDirectory(m_directory.parent_path());
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputText("##directory", &m_directoryText, ImGuiInputTextFlags_EnterReturnsTrue)) {
        setDirectory(pathFromUtf8(m_directoryText));
    }

    // Directories, then the files. Changing the directory is done after the loop.
    bool acceptRequested = false;
    std::optional<std::filesystem::path> nextDirectory;
    const float footerHeight = ImGui::GetFrameHeightWithSpacing() * 3.0f;
    if (ImGui::BeginListBox("##entries", ImVec2(-FLT_MIN, -footerHeight))) {
        for (const Entry& entry : m_entries) {
            const std::string label = entry.isDirectory ? entry.name + "/" : entry.name;
            const bool selected = !entry.isDirectory && entry.name == m_fileName;
            if (ImGui::Selectable(label.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                if (entry.isDirectory) {
                    nextDirectory = m_directory / pathFromUtf8(entry.name);
                }
                else {
                    m_fileName = entry.name;
                    m_confirmReplace = false;
                    acceptRequested = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
                }
            }
        }
        ImGui::EndListBox();
    }
    if (nextDirectory) {
        setDirectory(*nextDirectory);
    }

    ImGui::TextUnformatted("File name");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputText("##name", &m_fileName, ImGuiInputTextFlags_EnterReturnsTrue)) {
        acceptRequested = true;
    }
    if (ImGui::IsItemEdited()) {
        m_confirmReplace = false;
    }

    bool chosen = false;
    if (m_confirmReplace) {
        ImGui::TextColored(MESSAGE_COLOR, "%s already exists. Replace it?",
                           pathToUtf8(m_path.filename()).c_str());
        if (ImGui::Button("Replace", ImVec2(120.0f, 0.0f))) {
            chosen = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f))) {
            m_confirmReplace = false;
        }
    }
    else {
        if (!m_message.empty()) {
            ImGui::TextColored(MESSAGE_COLOR, "%s", m_message.c_str());
        }
        else {
            ImGui::NewLine();
        }
        const char* acceptLabel = (m_mode == Mode::Open) ? "Open" : "Save";
        if (ImGui::Button(acceptLabel, ImVec2(120.0f, 0.0f)) || acceptRequested) {
            chosen = accept();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f))) {
            ImGui::CloseCurrentPopup();
        }
    }

    if (chosen) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
    return chosen;
}

void FileDialog::setDirectory(const std::filesystem::path& directory)
{
    std::error_code error;
    std::filesystem::path absolute = std::filesystem::absolute(directory, error);
    m_directory = (error ? directory : absolute).lexically_normal();
    m_directoryText = pathToUtf8(m_directory);
    m_entries.clear();
    m_message.clear();

    // A directory that does not exist yet lists nothing. Saving creates it.
    if (!std::filesystem::is_directory(m_directory, error)) {
        return;
    }

    const auto options = std::filesystem::directory_options::skip_permission_denied;
    for (std::filesystem::directory_iterator it(m_directory, options, error), end;
         !error && it != end; it.increment(error)) {
        std::error_code entryError;
        const bool isDirectory = it->is_directory(entryError);
        if (!isDirectory && !hasExtension(it->path())) {
            continue;
        }
        m_entries.push_back({pathToUtf8(it->path().filename()), isDirectory});
    }
    if (error) {
        m_message = error.message();
    }

    std::sort(m_entries.begin(), m_entries.end(), [](const Entry& a, const Entry& b) {
        if (a.isDirectory != b.isDirectory) {
            return a.isDirectory;
        }
        return a.name < b.name;
    });
}

bool FileDialog::hasExtension(const std::filesystem::path& path) const
{
    std::string extension = pathToUtf8(path.extension());
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return std::find(m_extensions.begin(), m_extensions.end(), extension) != m_extensions.end();
}

bool FileDialog::accept()
{
    if (m_fileName.empty()) {
        m_message = "Enter a file name";
        return false;
    }

    std::filesystem::path path = pathFromUtf8(m_fileName);
    if (path.is_relative()) {
        path = m_directory / path;
    }

    std::error_code error;
    if (m_mode == Mode::Open) {
        if (!std::filesystem::is_regular_file(path, error)) {
            m_message = "The file does not exist";
            return false;
        }
    }
    else {
        if (!path.has_extension() && !m_extensions.empty()) {
            path += m_extensions.front();
        }
        if (std::filesystem::exists(path, error)) {
            m_path = path;
            m_confirmReplace = true;
            return false;
        }
    }

    m_path = path;
    return true;
}

} // namespace ocf::tracker_tool
