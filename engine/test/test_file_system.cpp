// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>
#include <ocf/platform/FileSystem.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>

using namespace ocf;

TEST(FileSystemTest, PathToUtf8UsesForwardSlashes)
{
    EXPECT_EQ(pathToUtf8(std::filesystem::path("tilemaps") / "props.png"), "tilemaps/props.png");
    EXPECT_EQ(pathToUtf8(std::filesystem::path()), "");
}

TEST(FileSystemTest, PathToUtf8KeepsNonAsciiCharacters)
{
    const std::u8string name = u8"マップ/地形.png";
    EXPECT_EQ(pathToUtf8(std::filesystem::path(name)), std::string(name.begin(), name.end()));
}

TEST(FileSystemTest, ResolveAssetPathJoinsAndNormalizes)
{
    EXPECT_EQ(pathToUtf8(resolveAssetPath("tilemaps", "props.png")), "tilemaps/props.png");
    EXPECT_EQ(pathToUtf8(resolveAssetPath("tilemaps/sub", "../props.png")), "tilemaps/props.png");
    EXPECT_EQ(pathToUtf8(resolveAssetPath("tilemaps", "./a/./b.png")), "tilemaps/a/b.png");
    EXPECT_EQ(pathToUtf8(resolveAssetPath(std::filesystem::path(), "sample.json")), "sample.json");
}

TEST(FileSystemTest, ResolveAssetPathReadsTheRelativePathAsUtf8)
{
    const std::u8string name = u8"地形.png";
    const std::string utf8(name.begin(), name.end());
    EXPECT_EQ(resolveAssetPath("tilemaps", utf8), std::filesystem::path(u8"tilemaps/地形.png"));
}

TEST(FileSystemTest, ReadAssetFileReturnsTheWholeFile)
{
    std::ifstream file(std::string(OCF_TEST_ASSETS_DIR) + "/tilemaps/props.tsj", std::ios::binary);
    ASSERT_TRUE(file);
    const std::string expected((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());

    const std::optional<std::string> text =
        FileSystem::getInstance()->readAssetFile(std::filesystem::path("tilemaps") / "props.tsj");
    ASSERT_TRUE(text.has_value());
    EXPECT_EQ(*text, expected);
}

TEST(FileSystemTest, ReadAssetFileReturnsNulloptForMissingFiles)
{
    EXPECT_FALSE(FileSystem::getInstance()->readAssetFile("no/such/file.json").has_value());
}
