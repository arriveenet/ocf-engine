// SPDX-License-Identifier: MIT
#pragma once

#include <filesystem>
#include <string>

/**
 * ImGui works with UTF-8 strings, which std::filesystem::path does not use on Windows.
 */
namespace ocf::tracker_tool {

std::string pathToUtf8(const std::filesystem::path& path);
std::filesystem::path pathFromUtf8(const std::string& text);

} // namespace ocf::tracker_tool
