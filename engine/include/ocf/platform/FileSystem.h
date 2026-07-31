// SPDX-License-Identifier: MIT
#pragma once
#include <filesystem>
#include <string>

namespace ocf {

constexpr std::string_view ASSET_DIRECTORY_NAME = "assets";

class FileSystem {
public:
    static FileSystem* getInstance();
    static void destroyInstance();

    ~FileSystem();

    /**
     * @brief Get the path of the assets directory.
     * @return The path of the assets directory.
     */
    const std::string& getAssetPath() const { return m_assetsPath; }

    /**
     * @brief Get the full path of the given asset file.
     * @param fileName The name of the asset file.
     * @return The full path of the asset file.
     */
    std::string getAssetFullPath(std::string_view fileName);

    /**
     * @brief Get the full path of the parent directory of the given file path.
     * @param filePath The path of the file.
     * @return The full path of the parent directory.
     */
    std::string getParentFullPath(std::string_view filePath);

    /**
     * @brief Get the file extension of the given file name.
     * @param fileName The name of the file.
     * @return The file extension in lowercase without the leading dot.
     */
    std::string getExtension(std::string_view fileName) const;

protected:
    FileSystem();

    bool init();

    std::filesystem::path findAssetsDirectory();

private:
    static FileSystem* s_sharedFileSystem;

    std::string m_assetsPath;
};

} // namespace ocf
