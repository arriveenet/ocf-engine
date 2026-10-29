// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/Reference.h"
#include "ocf/math/Rect.h"
#include "ocf/math/vec2.h"
#include "ocf/resource/TileMapTypes.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>

namespace ocf {

class Texture;

/**
 * @brief A grid of equally sized tiles cut out of one image.
 *
 * Tiles are numbered from 0 at the top left, row by row. The grid starts `margin` texels from
 * the image border and has `spacing` texels between tiles. The texture is not owned by the
 * tile set.
 */
class OCF_API TileSet : public RefCounted {
public:
    TileSet(uint32_t tileWidth, uint32_t tileHeight, uint32_t margin = 0, uint32_t spacing = 0);

    /** @brief Make a tile set covering the whole texture. */
    TileSet(Texture* texture, uint32_t tileWidth, uint32_t tileHeight, uint32_t margin = 0,
            uint32_t spacing = 0);

    ~TileSet();

    void setName(std::string name) { m_name = std::move(name); }
    const std::string& getName() const noexcept { return m_name; }

    uint32_t getTileWidth() const noexcept { return m_tileWidth; }
    uint32_t getTileHeight() const noexcept { return m_tileHeight; }
    uint32_t getMargin() const noexcept { return m_margin; }
    uint32_t getSpacing() const noexcept { return m_spacing; }

    /**
     * @brief Set the image the tiles are cut from and fit the grid to it.
     *
     * @param path Asset path of the image, resolved to a texture by the loader.
     */
    void setImage(std::string path, uint32_t width, uint32_t height);
    const std::string& getImagePath() const noexcept { return m_imagePath; }
    uint32_t getImageWidth() const noexcept { return m_imageWidth; }
    uint32_t getImageHeight() const noexcept { return m_imageHeight; }

    /** @brief Set the texture to draw with. Fits the grid to it if no image size is set yet. */
    void setTexture(Texture* texture);
    Texture* getTexture() const noexcept { return m_texture; }

    /** @brief Override the grid size computed from the image. */
    void setColumns(uint32_t columns) noexcept { m_columns = columns; }
    uint32_t getColumns() const noexcept { return m_columns; }

    void setTileCount(uint32_t tileCount) noexcept { m_tileCount = tileCount; }
    uint32_t getTileCount() const noexcept { return m_tileCount; }

    /** @brief Offset in pixels applied when drawing the tiles. */
    void setTileOffset(const math::vec2& offset) noexcept { m_tileOffset = offset; }
    const math::vec2& getTileOffset() const noexcept { return m_tileOffset; }

    /** @brief Region of a tile in texels with the origin at the top left. Empty if out of range. */
    math::Rect getTextureRect(uint32_t localId) const;

    TileProperties& getProperties() noexcept { return m_properties; }
    const TileProperties& getProperties() const noexcept { return m_properties; }

    /** @brief Properties of one tile, or nullptr if it has none. */
    const TileProperties* getTileProperties(uint32_t localId) const;
    void setTileProperties(uint32_t localId, TileProperties properties);

private:
    void fitGrid(uint32_t imageWidth, uint32_t imageHeight);

    std::string m_name;
    uint32_t m_tileWidth = 0;
    uint32_t m_tileHeight = 0;
    uint32_t m_margin = 0;
    uint32_t m_spacing = 0;
    uint32_t m_columns = 0;
    uint32_t m_tileCount = 0;
    math::vec2 m_tileOffset = math::vec2(0.0f, 0.0f);

    std::string m_imagePath;
    uint32_t m_imageWidth = 0;
    uint32_t m_imageHeight = 0;
    Texture* m_texture = nullptr;

    TileProperties m_properties;
    std::unordered_map<uint32_t, TileProperties> m_tileProperties;
};

} // namespace ocf
