// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/Reference.h"
#include "ocf/core/Result.h"
#include "ocf/core/compiler.h"
#include "ocf/resource/SpriteSheet.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace ocf {

class Engine;
class TextureManager;

/**
 * @brief Reads sprite sheets in the JSON formats of TexturePacker (https://www.codeandweb.com).
 *
 * Both "JSON (Hash)" and "JSON (Array)" are read; they are also used by Phaser, PixiJS and
 * other tools. Frames may be rotated and trimmed. Polygon meshes ("vertices") are ignored.
 *
 * The image path in the result is an asset path, resolved against the file that contains it.
 */
class OCF_API SpriteSheetLoader {
public:
    enum class Format {
        /** Detected from the contents. */
        Auto,
        /** TexturePacker JSON (Hash) or JSON (Array). */
        Json,
    };

    SpriteSheetLoader() = delete;

    /**
     * @brief Read a sprite sheet and load its texture. The format is detected from the contents.
     *
     * @param assetPath Path of the file relative to the assets directory.
     */
    static Result<Ref<SpriteSheet>, std::string> load(Engine& engine, std::string_view assetPath);

    /**
     * @brief Read a sprite sheet without loading its texture.
     *
     * @param baseDirectory Asset directory that the image path is resolved against.
     */
    static Result<Ref<SpriteSheet>, std::string>
    loadFromString(std::string_view text, Format format = Format::Auto,
                   const std::filesystem::path& baseDirectory = std::filesystem::path());

    /** @brief The format of a sprite sheet from its contents, or std::nullopt if unknown. */
    static std::optional<Format> detectFormat(std::string_view text);

    /** @brief Load the image of the sprite sheet if it has no texture yet. */
    static Result<void, std::string> resolveTexture(SpriteSheet& sheet,
                                                    TextureManager& textureManager);
};

} // namespace ocf
