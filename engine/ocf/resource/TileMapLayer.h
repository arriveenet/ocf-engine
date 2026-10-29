// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/compiler.h"
#include "ocf/math/vec2.h"
#include "ocf/resource/TileMapTypes.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ocf {

class TileMap;

/**
 * @brief A layer of a TileMap. Layers are created by and belong to the map.
 *
 * Changes that affect drawing advance the revision of the map.
 */
class OCF_API TileMapLayer {
public:
    enum class Type : uint8_t {
        Tile,
        Object,
    };

    virtual ~TileMapLayer();

    TileMapLayer(const TileMapLayer&) = delete;
    TileMapLayer& operator=(const TileMapLayer&) = delete;

    Type getType() const noexcept { return m_type; }

    /** @brief Id assigned by Tiled Map Editor, or 0. */
    void setId(uint32_t id) noexcept { m_id = id; }
    uint32_t getId() const noexcept { return m_id; }

    const std::string& getName() const noexcept { return m_name; }

    void setVisible(bool visible);
    bool isVisible() const noexcept { return m_visible; }

    void setOpacity(float opacity);
    float getOpacity() const noexcept { return m_opacity; }

    /** @brief Offset of the layer in pixels, y down. */
    void setOffset(const math::vec2& offset);
    const math::vec2& getOffset() const noexcept { return m_offset; }

    TileProperties& getProperties() noexcept { return m_properties; }
    const TileProperties& getProperties() const noexcept { return m_properties; }

protected:
    TileMapLayer(TileMap& map, Type type, std::string name);

    void markChanged() noexcept;

private:
    TileMap& m_map;
    Type m_type;
    uint32_t m_id = 0;
    std::string m_name;
    bool m_visible = true;
    float m_opacity = 1.0f;
    math::vec2 m_offset = math::vec2(0.0f, 0.0f);
    TileProperties m_properties;
};

/**
 * @brief A layer with one TileGid per cell, stored row by row from the top left.
 */
class OCF_API TileLayer : public TileMapLayer {
public:
    TileLayer(TileMap& map, std::string name, uint32_t width, uint32_t height);
    ~TileLayer() override;

    uint32_t getWidth() const noexcept { return m_width; }
    uint32_t getHeight() const noexcept { return m_height; }

    /** @brief The tile with its flip flags, or 0 outside the layer. */
    TileGid getTile(uint32_t x, uint32_t y) const;

    /** @brief Set a tile with its flip flags. Ignored outside the layer. */
    void setTile(uint32_t x, uint32_t y, TileGid tile);

    void fill(TileGid tile);

    /** @brief Replace all tiles. Returns false if the count is not width * height. */
    bool setTiles(std::vector<TileGid> tiles);
    const std::vector<TileGid>& getTiles() const noexcept { return m_tiles; }

private:
    uint32_t m_width;
    uint32_t m_height;
    std::vector<TileGid> m_tiles;
};

/**
 * @brief A shape or point placed on the map, e.g. a spawn point or a collision area.
 *
 * Positions and sizes are in pixels with y down. As in Tiled Map Editor, the position of a
 * tile object is its bottom-left corner and that of other objects the top-left corner.
 */
struct MapObject {
    enum class Shape : uint8_t {
        Rectangle,
        Ellipse,
        Point,
        Polygon,
        Polyline,
        Text,
    };

    uint32_t id = 0;
    std::string name;
    std::string className; // "type" or "class" in Tiled Map Editor
    Shape shape = Shape::Rectangle;
    math::vec2 position = math::vec2(0.0f, 0.0f);
    math::vec2 size = math::vec2(0.0f, 0.0f);
    float rotation = 0.0f; // Degrees, clockwise
    TileGid gid = 0;       // Tile objects only, with flip flags
    bool visible = true;
    std::vector<math::vec2> points; // Polygon and polyline, relative to the position
    std::string text;               // Text objects only
    TileProperties properties;
};

/**
 * @brief A layer of MapObjects. The objects are kept as data and are not drawn.
 */
class OCF_API ObjectLayer : public TileMapLayer {
public:
    ObjectLayer(TileMap& map, std::string name);
    ~ObjectLayer() override;

    MapObject& addObject(MapObject object);

    std::vector<MapObject>& getObjects() noexcept { return m_objects; }
    const std::vector<MapObject>& getObjects() const noexcept { return m_objects; }

    /** @brief The first object with the name, or nullptr. */
    const MapObject* findObject(std::string_view name) const;

private:
    std::vector<MapObject> m_objects;
};

} // namespace ocf
