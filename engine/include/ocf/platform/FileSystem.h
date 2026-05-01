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

    const std::string& getAssetPath() const { return m_assetsPath; }

    std::string getAssetFullPath(std::string_view fileName);

protected:
    FileSystem();

    bool init();

    std::filesystem::path findAssetsDirectory();

private:
    static FileSystem* s_sharedFileSystem;

    std::string m_assetsPath;
};

} // namespace ocf
