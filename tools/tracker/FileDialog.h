// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace ocf::tracker_tool {

/** ImGui works with UTF-8 strings. */
std::string pathToUtf8(const std::filesystem::path& path);
std::filesystem::path pathFromUtf8(const std::string& text);

/** A small modal dialog that picks a file to open or save. */
class FileDialog {
public:
    enum class Mode : uint8_t {
        Open,
        Save,
    };

    /** The id tells the dialogs apart, so that there can be several. */
    explicit FileDialog(std::string id);

    /**
     * Opens the dialog at the next draw(). It lists the files with the extensions (lower case,
     * with the dot), and saving adds the first one to a name without an extension.
     */
    void open(Mode mode, const std::string& title, const std::filesystem::path& directory,
              std::vector<std::string> extensions, const std::string& fileName = {});

    /** Returns true on the frame a file is chosen. getPath() then returns it. */
    bool draw();

    Mode getMode() const { return m_mode; }

    const std::filesystem::path& getPath() const { return m_path; }

private:
    struct Entry {
        std::string name;
        bool isDirectory = false;
    };

    void setDirectory(const std::filesystem::path& directory);
    bool hasExtension(const std::filesystem::path& path) const;

    /** Checks the file name. Returns true when m_path is the final choice. */
    bool accept();

    std::string m_id;
    std::string m_title; // With the id, as the ImGui name
    std::vector<std::string> m_extensions;
    Mode m_mode = Mode::Open;
    bool m_openRequested = false;
    std::filesystem::path m_directory;
    std::string m_directoryText;
    std::string m_fileName;
    std::vector<Entry> m_entries;
    std::string m_message;
    bool m_confirmReplace = false;
    std::filesystem::path m_path;
};

} // namespace ocf::tracker_tool
