// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/Reference.h"
#include "ocf/core/Result.h"
#include "ocf/core/compiler.h"
#include "ocf/resource/TileMap.h"

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace ocf {

class Engine;
class TextureManager;

/**
 * @brief Reads maps of Tiled Map Editor (https://www.mapeditor.org) into a TileMap.
 *
 * Both the JSON (.tmj, .tsj) and the XML (.tmx, .tsx) formats are read, and a map can refer
 * to external tile sets in either format. Supported are finite orthogonal maps with tile
 * layers (CSV, base64, base64 with zlib or gzip), object layers, group layers (flattened into
 * their children) and custom properties. Image layers are skipped with a warning.
 *
 * Paths in the result are asset paths, resolved against the file that contains them.
 */
class OCF_API TiledMapLoader {
public:
    enum class Format {
        Json,
        Xml,
    };

    /** Returns the contents of a file given by its asset path, or std::nullopt. */
    using FileReader = std::function<std::optional<std::string>(const std::filesystem::path&)>;

    TiledMapLoader() = delete;

    /**
     * @brief Read a map and load the textures of its tile sets.
     *
     * The format is chosen by the extension: .tmx is XML and anything else JSON.
     *
     * @param assetPath Path of the map relative to the assets directory.
     */
    static Result<Ref<TileMap>, std::string> load(Engine& engine, std::string_view assetPath);

    /**
     * @brief Read a map without loading textures.
     *
     * @param baseDirectory Asset directory that relative paths in the map are resolved against.
     * @param reader Reads external tile sets. If empty, they are read from the assets directory.
     */
    static Result<Ref<TileMap>, std::string>
    loadFromString(std::string_view text, Format format,
                   const std::filesystem::path& baseDirectory = std::filesystem::path(),
                   const FileReader& reader = FileReader());

    /** @brief Load the image of every tile set that has no texture yet. */
    static Result<void, std::string> resolveTextures(TileMap& map, TextureManager& textureManager);

    /** @brief Read a file from the assets directory. */
    static std::optional<std::string> readAssetFile(const std::filesystem::path& assetPath);
};

} // namespace ocf
