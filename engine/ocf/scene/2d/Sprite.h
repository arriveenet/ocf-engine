// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/math/Rect.h"
#include "ocf/scene/2d/Drawable2D.h"

namespace ocf {

class Engine;
class MaterialInstance;
class Texture;

/**
 * @brief Component that draws a region of a texture as a quad.
 *
 * The quad is as large as the texture region in pixels and placed in the node's local space
 * as described in Drawable2D. The texture is not owned by the sprite.
 */
class OCF_API Sprite : public Drawable2D {
public:
    explicit Sprite(Engine& engine, Texture* texture = nullptr);
    ~Sprite() override;

    /** @brief Set the texture to draw. The texture rect is reset to the whole texture. */
    void setTexture(Texture* texture);
    Texture* getTexture() const noexcept { return m_texture; }

    /** @brief Set the region of the texture to draw, in texels with the origin at the top left. */
    void setTextureRect(const math::Rect& rect);
    const math::Rect& getTextureRect() const noexcept { return m_textureRect; }

    void setFlipX(bool flipX);
    bool isFlipX() const noexcept { return m_flipX; }

    void setFlipY(bool flipY);
    bool isFlipY() const noexcept { return m_flipY; }

    void setTextureFilter(TextureFilter filter);
    TextureFilter getTextureFilter() const noexcept { return m_filter; }

protected:
    void rebuild() override;

private:
    Texture* m_texture = nullptr;
    math::Rect m_textureRect;
    bool m_flipX = false;
    bool m_flipY = false;
    TextureFilter m_filter = TextureFilter::Linear;

    MaterialInstance* m_materialInstance = nullptr;
    Texture* m_boundTexture = nullptr; // Texture and filter bound to m_materialInstance
    TextureFilter m_boundFilter = TextureFilter::Linear;
};

} // namespace ocf
