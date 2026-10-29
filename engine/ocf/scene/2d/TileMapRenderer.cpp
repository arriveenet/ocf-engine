// SPDX-License-Identifier: MIT
#include "ocf/scene/2d/TileMapRenderer.h"

#include "ocf/core/Logger.h"
#include "ocf/renderer/Texture.h"

#include <array>
#include <utility>

namespace ocf {

namespace {

/**
 * Texture coordinates of the corners of a tile (top left, bottom left, bottom right, top
 * right) with its flip flags applied: the diagonal flip first, then the horizontal and
 * vertical ones, as Tiled Map Editor does.
 */
std::array<math::vec2, 4> getTileTexCoords(TileGid tile, const math::vec2& uv0,
                                           const math::vec2& uv1)
{
    std::array<math::vec2, 4> uvs = {math::vec2(uv0.x, uv0.y), math::vec2(uv0.x, uv1.y),
                                     math::vec2(uv1.x, uv1.y), math::vec2(uv1.x, uv0.y)};
    if (isTileFlippedDiagonally(tile)) {
        std::swap(uvs[1], uvs[3]);
    }
    if (isTileFlippedX(tile)) {
        std::swap(uvs[0], uvs[3]);
        std::swap(uvs[1], uvs[2]);
    }
    if (isTileFlippedY(tile)) {
        std::swap(uvs[0], uvs[1]);
        std::swap(uvs[3], uvs[2]);
    }
    return uvs;
}

} // namespace

TileMapRenderer::TileMapRenderer(Engine& engine, const Ref<TileMap>& tileMap)
    : Drawable2D(engine, "shaders/sprite.frag.spv")
    , m_tileMap(tileMap)
{
}

TileMapRenderer::~TileMapRenderer() = default;

void TileMapRenderer::update(float deltaTime)
{
    // The map is shared, so it can change without going through this component
    if (m_tileMap.ptr() != nullptr &&
        (m_tileMap->getRevision() != m_appliedRevision || haveTexturesChanged())) {
        markDirty();
    }

    Drawable2D::update(deltaTime);
}

void TileMapRenderer::setTileMap(const Ref<TileMap>& tileMap)
{
    if (m_tileMap != tileMap) {
        m_tileMap = tileMap;
        markDirty();
    }
}

void TileMapRenderer::setTextureFilter(TextureFilter filter)
{
    if (m_filter != filter) {
        m_filter = filter;
        markDirty();
    }
}

void TileMapRenderer::rebuild()
{
    if (m_tileMap.ptr() == nullptr) {
        return;
    }

    const TileMap& map = *m_tileMap.ptr();
    const std::vector<TileMap::TileSetEntry>& tileSets = map.getTileSets();

    m_appliedRevision = map.getRevision();
    m_appliedTextures.clear();
    for (const TileMap::TileSetEntry& entry : tileSets) {
        m_appliedTextures.push_back(entry.tileSet->getTexture());
    }

    const float tileWidth = static_cast<float>(map.getTileWidth());
    const float tileHeight = static_cast<float>(map.getTileHeight());
    setContentSize(math::vec2(tileWidth * static_cast<float>(map.getWidth()),
                              tileHeight * static_cast<float>(map.getHeight())));

    // Geometry of the current layer, indexed by tile set
    std::vector<std::vector<VertexPositionTexCoord>> vertices(tileSets.size());
    std::vector<std::vector<uint32_t>> indices(tileSets.size());
    size_t batchCount = 0;
    bool isTextureMissing = false;

    for (const auto& layerPointer : map.getLayers()) {
        if (layerPointer->getType() != TileMapLayer::Type::Tile || !layerPointer->isVisible() ||
            layerPointer->getOpacity() <= 0.0f) {
            continue;
        }
        const TileLayer& layer = static_cast<const TileLayer&>(*layerPointer);

        for (size_t i = 0; i < tileSets.size(); i++) {
            vertices[i].clear();
            indices[i].clear();
        }

        for (uint32_t y = 0; y < layer.getHeight(); y++) {
            for (uint32_t x = 0; x < layer.getWidth(); x++) {
                const TileGid tile = layer.getTile(x, y);
                const TileMap::TileSetEntry* entry = map.findTileSet(tile);
                if (entry == nullptr) {
                    continue;
                }

                const TileSet& tileSet = *entry->tileSet.ptr();
                const Texture* texture = tileSet.getTexture();
                if (texture == nullptr) {
                    isTextureMissing = true;
                    continue;
                }

                const math::Rect rect = tileSet.getTextureRect(getTileGid(tile) - entry->firstGid);
                const float invTexWidth = 1.0f / static_cast<float>(texture->getWidth());
                const float invTexHeight = 1.0f / static_cast<float>(texture->getHeight());
                const math::vec2 uv0(rect.getMinX() * invTexWidth, rect.getMinY() * invTexHeight);
                const math::vec2 uv1(rect.getMaxX() * invTexWidth, rect.getMaxY() * invTexHeight);

                // Tiles are aligned to the bottom-left corner of their cell, so that tiles
                // taller than the cell extend upwards
                const math::vec2 position =
                    math::vec2(static_cast<float>(x) * tileWidth,
                               static_cast<float>(y + 1) * tileHeight - rect.m_size.y) +
                    tileSet.getTileOffset() + layer.getOffset();

                const size_t tileSetIndex = static_cast<size_t>(entry - tileSets.data());
                appendQuad(vertices[tileSetIndex], indices[tileSetIndex], position, rect.m_size,
                           getTileTexCoords(tile, uv0, uv1));
            }
        }

        for (size_t i = 0; i < tileSets.size(); i++) {
            if (indices[i].empty()) {
                continue;
            }
            MaterialInstance* materialInstance = prepareBatchMaterial(
                batchCount++, tileSets[i].tileSet->getTexture(), layer.getOpacity());
            addGeometry(vertices[i], indices[i], materialInstance);
        }
    }

    if (isTextureMissing) {
        OCF_LOG_WARN("TileMapRenderer: map '{}' uses a tile set without a texture", map.getName());
    }
}

bool TileMapRenderer::haveTexturesChanged() const
{
    const std::vector<TileMap::TileSetEntry>& tileSets = m_tileMap->getTileSets();
    if (tileSets.size() != m_appliedTextures.size()) {
        return true;
    }
    for (size_t i = 0; i < tileSets.size(); i++) {
        if (tileSets[i].tileSet->getTexture() != m_appliedTextures[i]) {
            return true;
        }
    }
    return false;
}

MaterialInstance* TileMapRenderer::prepareBatchMaterial(size_t batchIndex, Texture* texture,
                                                        float opacity)
{
    if (m_batches.size() <= batchIndex) {
        m_batches.resize(batchIndex + 1);
    }
    Batch& batch = m_batches[batchIndex];

    const bool isNewInstance = batch.materialInstance == nullptr;
    if (isNewInstance) {
        batch.materialInstance = createMaterialInstance(opacity);
    }
    else {
        setMaterialOpacity(batch.materialInstance, opacity);
    }

    // Only touch the descriptor when the texture or filter changes
    if (isNewInstance || batch.texture != texture || batch.filter != m_filter) {
        bindTexture(batch.materialInstance, texture, m_filter);
        batch.texture = texture;
        batch.filter = m_filter;
    }

    return batch.materialInstance;
}

} // namespace ocf
