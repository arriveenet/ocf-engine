// SPDX-License-Identifier: MIT
#include "PathUtility.h"

namespace ocf::tracker_tool {

std::string pathToUtf8(const std::filesystem::path& path)
{
    const std::u8string text = path.u8string();
    return std::string(text.begin(), text.end());
}

std::filesystem::path pathFromUtf8(const std::string& text)
{
    return std::filesystem::path(std::u8string(text.begin(), text.end()));
}

} // namespace ocf::tracker_tool
