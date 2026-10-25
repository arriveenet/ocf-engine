// SPDX-License-Identifier: MIT
#include "ocf/scene/2d/Sprite.h"

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

Sprite::~Sprite() = default;

void Sprite::setTexture(Texture* texture)
{
    m_texture = texture;
    if (m_texture != nullptr) {
        m_textureRect = math::Rect(0.0f, 0.0f, static_cast<float>(m_texture->getWidth()),
                                   static_cast<float>(m_texture->getHeight()));
    } else {
        m_textureRect = math::Rect();
    }
    markDirty();
}

void Sprite::setTextureRect(const math::Rect& rect)
{
    if (!m_textureRect.equals(rect)) {
        m_textureRect = rect;
        markDirty();
    }
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
    if (m_texture == nullptr || m_textureRect.m_size.x <= 0.0f ||
        m_textureRect.m_size.y <= 0.0f) {
        return;
    }

    setContentSize(m_textureRect.m_size);

    const float invTexWidth = 1.0f / static_cast<float>(m_texture->getWidth());
    const float invTexHeight = 1.0f / static_cast<float>(m_texture->getHeight());
    math::vec2 uv0(m_textureRect.getMinX() * invTexWidth, m_textureRect.getMinY() * invTexHeight);
    math::vec2 uv1(m_textureRect.getMaxX() * invTexWidth, m_textureRect.getMaxY() * invTexHeight);
    if (m_flipX) {
        std::swap(uv0.x, uv1.x);
    }
    if (m_flipY) {
        std::swap(uv0.y, uv1.y);
    }

    std::vector<VertexPositionTexCoord> vertices;
    std::vector<uint32_t> indices;
    appendQuad(vertices, indices, math::vec2(0.0f, 0.0f), m_textureRect.m_size, uv0, uv1);

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
