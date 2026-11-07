// SPDX-License-Identifier: MIT
#include "ocf/resource/AssetUtility.h"

#include "ocf/platform/FileSystem.h"

#include <fstream>
#include <iterator>

namespace ocf::asset {

std::string toUtf8(const std::filesystem::path& path)
{
    const std::u8string text = path.generic_u8string();
    return std::string(text.begin(), text.end());
}

std::filesystem::path resolvePath(const std::filesystem::path& baseDirectory,
                                  std::string_view relativePath)
{
    const std::filesystem::path path(
        std::u8string(reinterpret_cast<const char8_t*>(relativePath.data()), relativePath.size()));
    return (baseDirectory / path).lexically_normal();
}

std::optional<std::string> readFile(const std::filesystem::path& assetPath)
{
    const std::string fullPath = FileSystem::getInstance()->getAssetFullPath(toUtf8(assetPath));
    if (fullPath.empty()) {
        return std::nullopt;
    }

    std::ifstream file(fullPath, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (file.bad()) {
        return std::nullopt;
    }
    return text;
}

} // namespace ocf::asset
