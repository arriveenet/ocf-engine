#include "ocf/FileUtils.h"
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace ocf {

namespace fs = std::filesystem;

constexpr const char* OCF_ASSETS_DIR = "assets";

FileUtils* FileUtils::s_sharedFileUtils = nullptr;
std::string FileUtils::s_exeDirectory;

namespace {
    std::string checkExePath()
    {
        return fs::current_path().generic_string();
    }

    std::string checkAssetsPath()
    {
        std::string result;

        fs::path currentPath = fs::current_path();
        fs::path rootPath = currentPath.root_path();

        while (currentPath != rootPath) {
            fs::path assetsPath = currentPath;
            assetsPath.append(OCF_ASSETS_DIR);
            if (fs::exists(assetsPath)) {
                result = assetsPath.string();
                break;
            }

            currentPath = currentPath.parent_path();
        }

        return result;
    }
}

FileUtils* FileUtils::getInstance()
{
    if (s_sharedFileUtils == nullptr) {
        s_sharedFileUtils = new FileUtils;
        if (!s_sharedFileUtils->init()) {
            delete s_sharedFileUtils;
            s_sharedFileUtils = nullptr;
        }
    }

    return s_sharedFileUtils;
}

void FileUtils::destroyInstance()
{
    delete s_sharedFileUtils;
    s_sharedFileUtils = nullptr;
}

std::string FileUtils::getExtension(std::string_view filename)
{
    std::string fileExtension;
    size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos)
    {
        fileExtension = filename.substr(pos, filename.length());
        std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(),
                      [](char c) {return static_cast<char>(std::tolower(c)); });
    }

    return fileExtension;
}

FileUtils::~FileUtils()
{
}

std::string FileUtils::getFullPath(const std::string& filename) const
{
    std::filesystem::path fullPaht = std::filesystem::absolute(filename);

    return fullPaht.string();
}

std::string FileUtils::getParentFullPath(const std::string& filename) const
{
    std::filesystem::path parentPath = std::filesystem::path(filename).parent_path();

    return std::filesystem::absolute(parentPath).string();
}

void FileUtils::addSearchPath(const std::string& path, bool front)
{
    auto iter = std::find(m_searchPathArray.begin(), m_searchPathArray.end(), path);
    if (iter == m_searchPathArray.end()) {
        if (front) {
            m_searchPathArray.insert(m_searchPathArray.begin(), path);
        }
        else {
            m_searchPathArray.emplace_back(path);
        }
    }
}

const std::vector<std::string>& FileUtils::getSearchPath() const
{
    return m_searchPathArray;
}

std::string FileUtils::getAssetsPath() const
{
    return m_defaultAssetsRootPath;
}

std::string FileUtils::fullPathForFilename(const std::string& filename) const
{
    if (filename.empty()) {
        return "";
    }

    const fs::path p = fs::u8path(filename);
    if (p.is_absolute()) {
        return filename;
    }

    std::string fullpath;
    for (const auto& searchIter : m_searchPathArray) {
        fullpath = this->getPathForFilename(filename, searchIter);

        if (!fullpath.empty()) {
            return fullpath;
        }
    }

    return std::string();
}

std::string FileUtils::getStringFromFile(std::string_view filename) const
{
    std::string fullPath = fullPathForFilename(filename.data());

    std::string buffer;

    std::ifstream fs(fullPath, std::ios::binary);
    if (fs) {
        fs.seekg(0, std::ifstream::end);
        size_t fileSize = static_cast<size_t>(fs.tellg());
        fs.seekg(0, std::ifstream::beg);

        buffer.resize(fileSize);
        fs.read(buffer.data(), fileSize);

        fs.close();
    }

    return buffer;
}

bool FileUtils::isFileExist(const std::string& filename) const
{
    return fs::exists(filename);
}

FileUtils::FileUtils()
{
}

bool FileUtils::init()
{
    if (s_exeDirectory.empty()) {
        s_exeDirectory = checkExePath();
    }

    m_searchPathArray.emplace_back(s_exeDirectory);

    m_defaultAssetsRootPath = checkAssetsPath();
    if (!m_defaultAssetsRootPath.empty()) {
        m_searchPathArray.emplace_back(m_defaultAssetsRootPath);
        m_searchPathArray.emplace_back(m_defaultAssetsRootPath + "/fonts");
        m_searchPathArray.emplace_back(m_defaultAssetsRootPath + "/shaders");
        m_searchPathArray.emplace_back(m_defaultAssetsRootPath + "/textures");
    }

    return true;
}

std::string FileUtils::getPathForFilename(const std::string& filename, const std::string& searchPath) const
{
    const std::string appendPath = searchPath + "/" + filename;
    fs::path path = fs::u8path(appendPath);

    std::string result = fs::exists(path) ? path.generic_string() : "";

    return result;
}

} // namespace ocf
