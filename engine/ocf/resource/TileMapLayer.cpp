// SPDX-License-Identifier: MIT
#include "ocf/resource/TileMapLayer.h"

#include "ocf/resource/TileMap.h"

#include <algorithm>

namespace ocf {

TileMapLayer::TileMapLayer(TileMap& map, Type type, std::string name)
    : m_map(map)
    , m_type(type)
    , m_name(std::move(name))
{
}

TileMapLayer::~TileMapLayer() = default;

void TileMapLayer::setVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;
        markChanged();
    }
}

void TileMapLayer::setOpacity(float opacity)
{
    opacity = std::clamp(opacity, 0.0f, 1.0f);
    if (m_opacity != opacity) {
        m_opacity = opacity;
        markChanged();
    }
}

void TileMapLayer::setOffset(const math::vec2& offset)
{
    if (m_offset != offset) {
        m_offset = offset;
        markChanged();
    }
}

void TileMapLayer::markChanged() noexcept
{
    m_map.markChanged();
}

TileLayer::TileLayer(TileMap& map, std::string name, uint32_t width, uint32_t height)
    : TileMapLayer(map, Type::Tile, std::move(name))
    , m_width(width)
    , m_height(height)
    , m_tiles(static_cast<size_t>(width) * height, 0)
{
}

TileLayer::~TileLayer() = default;

TileGid TileLayer::getTile(uint32_t x, uint32_t y) const
{
    if (x >= m_width || y >= m_height) {
        return 0;
    }
    return m_tiles[static_cast<size_t>(y) * m_width + x];
}

void TileLayer::setTile(uint32_t x, uint32_t y, TileGid tile)
{
    if (x >= m_width || y >= m_height) {
        return;
    }
    TileGid& cell = m_tiles[static_cast<size_t>(y) * m_width + x];
    if (cell != tile) {
        cell = tile;
        markChanged();
    }
}

void TileLayer::fill(TileGid tile)
{
    std::fill(m_tiles.begin(), m_tiles.end(), tile);
    markChanged();
}

bool TileLayer::setTiles(std::vector<TileGid> tiles)
{
    if (tiles.size() != m_tiles.size()) {
        return false;
    }
    m_tiles = std::move(tiles);
    markChanged();
    return true;
}

ObjectLayer::ObjectLayer(TileMap& map, std::string name)
    : TileMapLayer(map, Type::Object, std::move(name))
{
}

ObjectLayer::~ObjectLayer() = default;

MapObject& ObjectLayer::addObject(MapObject object)
{
    return m_objects.emplace_back(std::move(object));
}

const MapObject* ObjectLayer::findObject(std::string_view name) const
{
    auto iter = std::find_if(m_objects.begin(), m_objects.end(),
                             [name](const MapObject& object) { return object.name == name; });
    return iter != m_objects.end() ? &*iter : nullptr;
}

} // namespace ocf
