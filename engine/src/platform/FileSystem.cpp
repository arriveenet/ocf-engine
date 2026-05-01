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
