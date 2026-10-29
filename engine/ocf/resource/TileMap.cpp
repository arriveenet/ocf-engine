// SPDX-License-Identifier: MIT
#include "ocf/resource/TileMap.h"

#include <algorithm>

namespace ocf {

TileMap::TileMap(uint32_t width, uint32_t height, uint32_t tileWidth, uint32_t tileHeight)
    : m_width(width)
    , m_height(height)
    , m_tileWidth(tileWidth)
    , m_tileHeight(tileHeight)
{
}

TileMap::~TileMap() = default;

TileGid TileMap::addTileSet(const Ref<TileSet>& tileSet)
{
    TileGid firstGid = 1;
    if (!m_tileSets.empty()) {
        const TileSetEntry& last = m_tileSets.back();
        firstGid = last.firstGid + last.tileSet->getTileCount();
    }
    addTileSet(tileSet, firstGid);
    return firstGid;
}

bool TileMap::addTileSet(const Ref<TileSet>& tileSet, TileGid firstGid)
{
    if (tileSet.ptr() == nullptr || firstGid == 0 || (firstGid & TILE_FLAG_MASK) != 0) {
        return false;
    }

    // Keep the entries sorted so that findTileSet() can search them
    auto iter = std::upper_bound(
        m_tileSets.begin(), m_tileSets.end(), firstGid,
        [](TileGid gid, const TileSetEntry& entry) { return gid < entry.firstGid; });

    if (iter != m_tileSets.begin()) {
        const TileSetEntry& previous = *(iter - 1);
        if (firstGid < previous.firstGid + previous.tileSet->getTileCount() ||
            firstGid == previous.firstGid) {
            return false;
        }
    }
    if (iter != m_tileSets.end() && firstGid + tileSet->getTileCount() > iter->firstGid) {
        return false;
    }

    m_tileSets.insert(iter, TileSetEntry{firstGid, tileSet});
    markChanged();
    return true;
}

const TileMap::TileSetEntry* TileMap::findTileSet(TileGid tile) const
{
    const TileGid gid = getTileGid(tile);
    if (gid == 0) {
        return nullptr;
    }

    auto iter = std::upper_bound(
        m_tileSets.begin(), m_tileSets.end(), gid,
        [](TileGid value, const TileSetEntry& entry) { return value < entry.firstGid; });
    if (iter == m_tileSets.begin()) {
        return nullptr;
    }

    const TileSetEntry& entry = *(iter - 1);
    if (gid - entry.firstGid >= entry.tileSet->getTileCount()) {
        return nullptr;
    }
    return &entry;
}

TileLayer* TileMap::addTileLayer(std::string name)
{
    auto layer = std::make_unique<TileLayer>(*this, std::move(name), m_width, m_height);
    TileLayer* result = layer.get();
    m_layers.push_back(std::move(layer));
    markChanged();
    return result;
}

ObjectLayer* TileMap::addObjectLayer(std::string name)
{
    auto layer = std::make_unique<ObjectLayer>(*this, std::move(name));
    ObjectLayer* result = layer.get();
    m_layers.push_back(std::move(layer));
    return result;
}

TileMapLayer* TileMap::findLayer(std::string_view name) const
{
    for (const auto& layer : m_layers) {
        if (layer->getName() == name) {
            return layer.get();
        }
    }
    return nullptr;
}

TileLayer* TileMap::findTileLayer(std::string_view name) const
{
    for (const auto& layer : m_layers) {
        if (layer->getType() == TileMapLayer::Type::Tile && layer->getName() == name) {
            return static_cast<TileLayer*>(layer.get());
        }
    }
    return nullptr;
}

ObjectLayer* TileMap::findObjectLayer(std::string_view name) const
{
    for (const auto& layer : m_layers) {
        if (layer->getType() == TileMapLayer::Type::Object && layer->getName() == name) {
            return static_cast<ObjectLayer*>(layer.get());
        }
    }
    return nullptr;
}

} // namespace ocf
