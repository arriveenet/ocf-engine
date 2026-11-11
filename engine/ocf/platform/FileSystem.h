// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/compiler.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace ocf {

constexpr std::string_view ASSET_DIRECTORY_NAME = "assets";

/** @brief The path with '/' separators, encoded as UTF-8. */
OCF_API std::string pathToUtf8(const std::filesystem::path& path);

/**
 * @brief Resolve a UTF-8 path relative to a directory in the assets into a normalized asset path.
 *
 * Asset files refer to each other relative to their own directory; this joins such a reference
 * to that directory so that the result is relative to the assets directory.
 */
OCF_API std::filesystem::path resolveAssetPath(const std::filesystem::path& baseDirectory,
                                               std::string_view relativePath);

class OCF_API FileSystem {
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
     * @brief Read a whole file in the assets directory as bytes.
     * @param assetPath Path of the file relative to the assets directory.
     * @return The contents, or std::nullopt if the file cannot be read.
     */
    std::optional<std::string> readAssetFile(const std::filesystem::path& assetPath);

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
