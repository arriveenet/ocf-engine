// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/Reference.h"
#include "ocf/resource/TileMap.h"
#include "ocf/scene/2d/Drawable2D.h"

#include <cstdint>
#include <vector>

namespace ocf {

class Engine;
class MaterialInstance;
class Texture;

/**
 * @brief Component that draws the tile layers of a TileMap.
 *
 * The map is laid out in pixels as in Tiled Map Editor and placed in the node's local space as
 * described in Drawable2D; the content size is the size of the map. Visible tile layers are
 * drawn in order with one draw call per layer and tile set, so the view should use
 * View::SortMode::SubmitOrder. Object layers are not drawn.
 *
 * The map is shared and can be edited after it is set: changes are picked up in update().
 */
class OCF_API TileMapRenderer : public Drawable2D {
public:
    explicit TileMapRenderer(Engine& engine, const Ref<TileMap>& tileMap = Ref<TileMap>());
    ~TileMapRenderer() override;

    void update(float deltaTime) override;

    void setTileMap(const Ref<TileMap>& tileMap);
    const Ref<TileMap>& getTileMap() const noexcept { return m_tileMap; }

    /** @brief Set the texture filter. Nearest by default, so that pixel art stays sharp. */
    void setTextureFilter(TextureFilter filter);
    TextureFilter getTextureFilter() const noexcept { return m_filter; }

protected:
    void rebuild() override;

private:
    // Material binding for one draw call, reused across rebuilds
    struct Batch {
        MaterialInstance* materialInstance = nullptr;
        Texture* texture = nullptr;
        TextureFilter filter = TextureFilter::Nearest;
    };

    bool haveTexturesChanged() const;
    MaterialInstance* prepareBatchMaterial(size_t batchIndex, Texture* texture, float opacity);

    Ref<TileMap> m_tileMap;
    TextureFilter m_filter = TextureFilter::Nearest;

    // State of the map used by the last rebuild
    uint32_t m_appliedRevision = 0;
    std::vector<Texture*> m_appliedTextures; // Indexed by tile set

    std::vector<Batch> m_batches; // Indexed by draw call
};

} // namespace ocf
