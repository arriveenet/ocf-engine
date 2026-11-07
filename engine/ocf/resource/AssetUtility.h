// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/compiler.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

/** Helpers for asset paths, shared by the resource loaders. */
namespace ocf::asset {

/** @brief The path with '/' separators, encoded as UTF-8. */
OCF_API std::string toUtf8(const std::filesystem::path& path);

/** @brief Resolve a UTF-8 path relative to an asset directory into a normalized asset path. */
OCF_API std::filesystem::path resolvePath(const std::filesystem::path& baseDirectory,
                                          std::string_view relativePath);

/** @brief Read a file from the assets directory, or std::nullopt if it cannot be read. */
OCF_API std::optional<std::string> readFile(const std::filesystem::path& assetPath);

} // namespace ocf::asset
