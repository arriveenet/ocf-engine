// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/Variant.h"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace ocf {

/**
 * @brief Global tile id as stored in a tile layer.
 *
 * The upper bits hold the flip flags of Tiled Map Editor and the rest the id. 0 is an empty
 * cell; otherwise the id refers to a tile of the tile set whose range contains it.
 */
using TileGid = uint32_t;

inline constexpr TileGid TILE_FLIPPED_HORIZONTALLY = 0x80000000u;
inline constexpr TileGid TILE_FLIPPED_VERTICALLY = 0x40000000u;
inline constexpr TileGid TILE_FLIPPED_DIAGONALLY = 0x20000000u;
inline constexpr TileGid TILE_ROTATED_HEXAGONAL_120 = 0x10000000u; // Hexagonal maps only
inline constexpr TileGid TILE_FLAG_MASK = TILE_FLIPPED_HORIZONTALLY | TILE_FLIPPED_VERTICALLY |
                                          TILE_FLIPPED_DIAGONALLY | TILE_ROTATED_HEXAGONAL_120;
inline constexpr TileGid TILE_GID_MASK = ~TILE_FLAG_MASK;

/** @brief The id without the flip flags. */
constexpr TileGid getTileGid(TileGid tile) noexcept
{
    return tile & TILE_GID_MASK;
}

constexpr bool isTileFlippedX(TileGid tile) noexcept
{
    return (tile & TILE_FLIPPED_HORIZONTALLY) != 0;
}

constexpr bool isTileFlippedY(TileGid tile) noexcept
{
    return (tile & TILE_FLIPPED_VERTICALLY) != 0;
}

/** @brief Whether the tile is mirrored along its top-left to bottom-right diagonal. */
constexpr bool isTileFlippedDiagonally(TileGid tile) noexcept
{
    return (tile & TILE_FLIPPED_DIAGONALLY) != 0;
}

/**
 * @brief Combine an id with flip flags.
 *
 * The diagonal flip is applied first, then the horizontal and vertical ones, so a tile rotated
 * by 90 degrees clockwise is (diagonal, flipX).
 */
constexpr TileGid makeTileGid(TileGid gid, bool flipX = false, bool flipY = false,
                              bool flipDiagonally = false) noexcept
{
    return (gid & TILE_GID_MASK) | (flipX ? TILE_FLIPPED_HORIZONTALLY : 0u) |
           (flipY ? TILE_FLIPPED_VERTICALLY : 0u) | (flipDiagonally ? TILE_FLIPPED_DIAGONALLY : 0u);
}

/**
 * @brief Custom properties of a map, layer, tile set, tile or object.
 *
 * Tiled types are stored as bool, int (int and object), float, or std::string (string, color
 * as "#AARRGGBB", and file as an asset path).
 */
using TileProperties = std::unordered_map<std::string, Variant>;

} // namespace ocf
