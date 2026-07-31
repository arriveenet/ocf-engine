// SPDX-License-Identifier: MIT
#include "ocf/platform/FileSystem.h"
#include <filesystem>

namespace fs = std::filesystem;

namespace ocf {

FileSystem* FileSystem::s_sharedFileSystem = nullptr;

FileSystem* FileSystem::getInstance()
{
    if (s_sharedFileSystem == nullptr) {
        s_sharedFileSystem = new FileSystem();
        s_sharedFileSystem->init();
    }
    return s_sharedFileSystem;
}

void FileSystem::destroyInstance()
{
    delete s_sharedFileSystem;
    s_sharedFileSystem = nullptr;
}

std::string FileSystem::getAssetFullPath(std::string_view fileName)
{
    if (m_assetsPath.empty()) {
        return "";
    }

    fs::path filePath(m_assetsPath);
    filePath.append(fileName);

    if (fs::exists(filePath)) {
        return fs::absolute(filePath).generic_string();
    }

    return "";
}

std::string FileSystem::getParentFullPath(std::string_view filePath)
{
    fs::path path(filePath);
    return path.parent_path().generic_string();
}

std::string FileSystem::getExtension(std::string_view fileName) const
{
    std::string ext = fs::path(fileName).extension().string();

    if (!ext.empty() && ext[0] == '.') {
        ext.erase(0, 1);
    }

    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    return ext;
}

FileSystem::FileSystem()
{
}

FileSystem::~FileSystem()
{
}

bool FileSystem::init()
{
    auto assetPath = findAssetsDirectory();
    if (assetPath.empty()) {
        return false;
    }

    m_assetsPath = assetPath.generic_string();

    return true;
}

std::filesystem::path FileSystem::findAssetsDirectory()
{
    fs::path searchPath = fs::current_path();

    while (true) {
        fs::path assetsPath = searchPath / ASSET_DIRECTORY_NAME;

        if (fs::exists(assetsPath)) {
            return assetsPath;
        }

        if (!searchPath.has_parent_path()) {
            break;
        }

        searchPath = searchPath.parent_path();
    }

    return std::filesystem::path();
}

} // namespace ocf
