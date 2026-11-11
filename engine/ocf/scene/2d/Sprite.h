// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/math/Rect.h"
#include "ocf/resource/SpriteSheet.h"
#include "ocf/scene/2d/Drawable2D.h"

#include <string_view>

namespace ocf {

class Engine;
class MaterialInstance;
class Texture;

/**
 * @brief Component that draws a region of a texture as a quad.
 *
 * The quad is as large as the texture region in pixels and placed in the node's local space
 * as described in Drawable2D. The texture is not owned by the sprite.
 *
 * A frame of a SpriteSheet can be drawn instead of a plain region. Its content size is the
 * size of the original image, so trimmed frames of an animation keep their place relative to
 * the anchor.
 */
class OCF_API Sprite : public Drawable2D {
public:
    explicit Sprite(Engine& engine, Texture* texture = nullptr);
    Sprite(Engine& engine, const SpriteFrame& frame, Texture* texture);
    ~Sprite() override;

    /** @brief Set the texture to draw. The texture rect is reset to the whole texture. */
    void setTexture(Texture* texture);
    Texture* getTexture() const noexcept { return m_texture; }

    /** @brief Set the region of the texture to draw, in texels with the origin at the top left. */
    void setTextureRect(const math::Rect& rect);
    const math::Rect& getTextureRect() const noexcept { return m_frame.textureRect; }

    /**
     * @brief Draw a frame of a sprite sheet. The frame is copied.
     *
     * The anchor is not changed; pass frame.pivot to setAnchor() to use the packer's pivot.
     */
    void setSpriteFrame(const SpriteFrame& frame, Texture* texture);

    /**
     * @brief Draw the named frame of a sprite sheet with its texture.
     *
     * @return false if the sheet has no such frame; the sprite is not changed then.
     */
    bool setSpriteFrame(const SpriteSheet& sheet, std::string_view frameName);

    /** @brief The frame drawn; a plain texture region is a frame without trimming. */
    const SpriteFrame& getSpriteFrame() const noexcept { return m_frame; }

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
    SpriteFrame m_frame;
    bool m_flipX = false;
    bool m_flipY = false;
    TextureFilter m_filter = TextureFilter::Linear;

    MaterialInstance* m_materialInstance = nullptr;
    Texture* m_boundTexture = nullptr; // Texture and filter bound to m_materialInstance
    TextureFilter m_boundFilter = TextureFilter::Linear;
};

} // namespace ocf
