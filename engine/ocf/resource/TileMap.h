// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/Reference.h"
#include "ocf/math/vec4.h"
#include "ocf/resource/Resource.h"
#include "ocf/resource/TileMapLayer.h"
#include "ocf/resource/TileMapTypes.h"
#include "ocf/resource/TileSet.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ocf {

/**
 * @brief An orthogonal tile map: a grid of cells, tile sets and layers drawn in order.
 *
 * The map is plain data, built either by TiledMapLoader or in code, and drawn by
 * TileMapRenderer. Coordinates are in pixels with the origin at the top left and y down.
 */
class OCF_API TileMap : public Resource {
public:
    /** @brief A tile set and the first TileGid it covers in this map. */
    struct TileSetEntry {
        TileGid firstGid = 0;
        Ref<TileSet> tileSet;
    };

    /**
     * @param width Width of the map in cells.
     * @param height Height of the map in cells.
     * @param tileWidth Width of a cell in pixels.
     * @param tileHeight Height of a cell in pixels.
     */
    TileMap(uint32_t width, uint32_t height, uint32_t tileWidth, uint32_t tileHeight);
    ~TileMap() override;

    void setName(std::string name) { m_name = std::move(name); }

    uint32_t getWidth() const noexcept { return m_width; }
    uint32_t getHeight() const noexcept { return m_height; }
    uint32_t getTileWidth() const noexcept { return m_tileWidth; }
    uint32_t getTileHeight() const noexcept { return m_tileHeight; }

    void setBackgroundColor(const math::vec4& color) noexcept { m_backgroundColor = color; }
    const math::vec4& getBackgroundColor() const noexcept { return m_backgroundColor; }

    TileProperties& getProperties() noexcept { return m_properties; }
    const TileProperties& getProperties() const noexcept { return m_properties; }

    /**
     * @brief Add a tile set after the ones already added.
     *
     * The tile count of the tile set must be final, since it decides where the next tile set
     * starts.
     *
     * @return The first TileGid of the tile set.
     */
    TileGid addTileSet(const Ref<TileSet>& tileSet);

    /**
     * @brief Add a tile set at a given first TileGid.
     *
     * @return false if firstGid is 0 or overlaps the tile sets already added.
     */
    bool addTileSet(const Ref<TileSet>& tileSet, TileGid firstGid);

    const std::vector<TileSetEntry>& getTileSets() const noexcept { return m_tileSets; }

    /** @brief The tile set containing a tile (flip flags are ignored), or nullptr. */
    const TileSetEntry* findTileSet(TileGid tile) const;

    /** @brief Add a tile layer as large as the map, drawn above the layers already added. */
    TileLayer* addTileLayer(std::string name);

    ObjectLayer* addObjectLayer(std::string name);

    const std::vector<std::unique_ptr<TileMapLayer>>& getLayers() const noexcept
    {
        return m_layers;
    }

    /** @brief The first layer with the name, or nullptr. */
    TileMapLayer* findLayer(std::string_view name) const;
    TileLayer* findTileLayer(std::string_view name) const;
    ObjectLayer* findObjectLayer(std::string_view name) const;

    /** @brief Advanced by every change that affects drawing. */
    uint32_t getRevision() const noexcept { return m_revision; }

private:
    friend class TileMapLayer;

    void markChanged() noexcept { m_revision++; }

    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_tileWidth;
    uint32_t m_tileHeight;
    math::vec4 m_backgroundColor = math::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    TileProperties m_properties;

    std::vector<TileSetEntry> m_tileSets; // Sorted by firstGid
    std::vector<std::unique_ptr<TileMapLayer>> m_layers;
    uint32_t m_revision = 0;
};

} // namespace ocf
