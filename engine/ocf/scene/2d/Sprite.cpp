// SPDX-License-Identifier: MIT
#include "ocf/scene/2d/Sprite.h"

#include "ocf/core/Logger.h"
#include "ocf/renderer/Texture.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace ocf {

Sprite::Sprite(Engine& engine, Texture* texture)
    : Drawable2D(engine, "shaders/sprite.frag.spv")
{
    setTexture(texture);
}

Sprite::Sprite(Engine& engine, const SpriteFrame& frame, Texture* texture)
    : Drawable2D(engine, "shaders/sprite.frag.spv")
{
    setSpriteFrame(frame, texture);
}

Sprite::~Sprite() = default;

void Sprite::setTexture(Texture* texture)
{
    m_texture = texture;
    if (m_texture != nullptr) {
        m_frame = SpriteFrame::fromRect(
            math::Rect(0.0f, 0.0f, static_cast<float>(m_texture->getWidth()),
                       static_cast<float>(m_texture->getHeight())));
    } else {
        m_frame = SpriteFrame();
    }
    markDirty();
}

void Sprite::setTextureRect(const math::Rect& rect)
{
    // A plain region replaces a sprite sheet frame along with its trimming and rotation
    SpriteFrame frame = SpriteFrame::fromRect(rect);
    if (m_frame.rotated || !m_frame.textureRect.equals(rect) || m_frame.offset != frame.offset ||
        m_frame.sourceSize != frame.sourceSize) {
        m_frame = std::move(frame);
        markDirty();
    }
}

void Sprite::setSpriteFrame(const SpriteFrame& frame, Texture* texture)
{
    m_texture = texture;
    m_frame = frame;
    markDirty();
}

bool Sprite::setSpriteFrame(const SpriteSheet& sheet, std::string_view frameName)
{
    const SpriteFrame* frame = sheet.getFrame(frameName);
    if (frame == nullptr) {
        OCF_LOG_WARN("Sprite: sprite sheet \"{}\" has no frame \"{}\"", sheet.getName(),
                     frameName);
        return false;
    }
    setSpriteFrame(*frame, sheet.getTexture());
    return true;
}

void Sprite::setFlipX(bool flipX)
{
    if (m_flipX != flipX) {
        m_flipX = flipX;
        markDirty();
    }
}

void Sprite::setFlipY(bool flipY)
{
    if (m_flipY != flipY) {
        m_flipY = flipY;
        markDirty();
    }
}

void Sprite::setTextureFilter(TextureFilter filter)
{
    if (m_filter == filter) {
        return;
    }
    m_filter = filter;

    // The geometry does not depend on the filter, so only the bound sampler is replaced. If
    // nothing is bound yet, the next rebuild binds the texture with the new filter.
    if (m_materialInstance != nullptr && m_boundTexture != nullptr) {
        bindTexture(m_materialInstance, m_boundTexture, m_filter);
        m_boundFilter = m_filter;
    }
}

void Sprite::rebuild()
{
    if (m_texture == nullptr || m_texture->getWidth() == 0 || m_texture->getHeight() == 0 ||
        m_frame.textureRect.m_size.x <= 0.0f || m_frame.textureRect.m_size.y <= 0.0f) {
        return;
    }

    // The content is the original image, of which the packed part may cover only a region
    setContentSize(m_frame.sourceSize);

    const math::vec2 textureSize(static_cast<float>(m_texture->getWidth()),
                                 static_cast<float>(m_texture->getHeight()));
    const SpriteQuad quad = computeSpriteQuad(m_frame, textureSize, m_flipX, m_flipY);

    std::vector<VertexPositionTexCoord> vertices;
    std::vector<uint32_t> indices;
    appendQuad(vertices, indices, quad.position, quad.size, quad.uvs);

    if (m_materialInstance == nullptr) {
        m_materialInstance = createMaterialInstance();
    }

    // Only touch the descriptor when the texture or filter changes
    if (m_boundTexture != m_texture || m_boundFilter != m_filter) {
        bindTexture(m_materialInstance, m_texture, m_filter);
        m_boundTexture = m_texture;
        m_boundFilter = m_filter;
    }

    addGeometry(vertices, indices, m_materialInstance);
}

} // namespace ocf
