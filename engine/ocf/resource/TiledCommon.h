// SPDX-License-Identifier: MIT
#pragma once

// Internal to TiledMapLoader: code shared by the JSON and XML readers.

#include "ocf/core/Reference.h"
#include "ocf/core/Result.h"
#include "ocf/math/vec2.h"
#include "ocf/math/vec4.h"
#include "ocf/resource/TileMap.h"
#include "ocf/resource/TiledMapLoader.h"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ocf::tiled {

struct ParseContext {
    std::filesystem::path baseDirectory; // Asset directory of the file being read
    TiledMapLoader::FileReader reader;
};

/** Opacity, offset and visibility inherited from the group layers around a layer. */
struct LayerGroup {
    math::vec2 offset = math::vec2(0.0f, 0.0f);
    float opacity = 1.0f;
    bool visible = true;
};

Result<Ref<TileMap>, std::string> readMapJson(std::string_view text, const ParseContext& context);
Result<Ref<TileMap>, std::string> readMapXml(std::string_view text, const ParseContext& context);

/** Reads a tile set file. Its image path is resolved against `baseDirectory`. */
Result<Ref<TileSet>, std::string> readTileSetJson(std::string_view text,
                                                  const std::filesystem::path& baseDirectory);
Result<Ref<TileSet>, std::string> readTileSetXml(std::string_view text,
                                                 const std::filesystem::path& baseDirectory);

/** Reads an external tile set in the format given by its extension (.tsx is XML). */
Result<Ref<TileSet>, std::string> readExternalTileSet(std::string_view source,
                                                      const ParseContext& context);

/** The path with '/' separators, encoded as UTF-8. */
std::string toUtf8(const std::filesystem::path& path);

/** Resolves a path relative to an asset directory into a normalized asset path. */
std::filesystem::path resolvePath(const std::filesystem::path& baseDirectory,
                                  std::string_view relativePath);

/** Parses "#RRGGBB" or "#AARRGGBB" (the '#' is optional). */
std::optional<math::vec4> parseColor(std::string_view text);

/** Parses comma separated tiles, as in the "csv" encoding. Whitespace is ignored. */
Result<std::vector<TileGid>, std::string> decodeCsvTiles(std::string_view text,
                                                         size_t expectedCount);

/**
 * Decodes the "base64" encoding with a compression of "" (none), "zlib" or "gzip". The tiles
 * are 32-bit little endian. Whitespace around and inside the text is ignored.
 */
Result<std::vector<TileGid>, std::string>
decodeBase64Tiles(std::string_view text, std::string_view compression, size_t expectedCount);

/** Stores the tiles in a layer of the map, checking that their tile sets exist. */
Result<void, std::string> setLayerTiles(const TileMap& map, TileLayer& layer,
                                        std::vector<TileGid> tiles);

/** Applies the attributes of a layer and of the groups around it. */
void applyLayerAttributes(TileMapLayer& layer, const LayerGroup& group, uint32_t id, bool visible,
                          float opacity, const math::vec2& offset);

} // namespace ocf::tiled
