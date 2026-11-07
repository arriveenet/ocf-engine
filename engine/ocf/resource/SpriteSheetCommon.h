// SPDX-License-Identifier: MIT
#pragma once

// Internal to SpriteSheetLoader: what the readers of each format share.
//
// Every reader converts its format into the conventions of SpriteFrame: the texture rect has
// the size of the image as drawn, rotated frames are stored clockwise, and the offset is the
// top-left corner of the trimmed image in the original one. Paths are asset paths resolved
// against the directory of the file.

#include "ocf/core/Reference.h"
#include "ocf/core/Result.h"
#include "ocf/resource/SpriteSheet.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace ocf::spritesheet {

using SheetResult = Result<Ref<SpriteSheet>, std::string>;

/** Reads the JSON (Hash) and JSON (Array) formats of TexturePacker. */
SheetResult readJson(std::string_view text, const std::filesystem::path& baseDirectory);

/** Checks a frame read from any format. Returns the problem, or std::nullopt if it is valid. */
std::optional<std::string> validateFrame(const SpriteFrame& frame);

} // namespace ocf::spritesheet
