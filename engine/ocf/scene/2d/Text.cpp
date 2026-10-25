// SPDX-License-Identifier: MIT
#include "ocf/scene/2d/Text.h"

#include "ocf/core/Logger.h"
#include "ocf/core/TextUtility.h"
#include "ocf/renderer/Texture.h"
#include "ocf/resource/Font.h"
#include "ocf/resource/FontAtlas.h"

#include <cstdint>

namespace ocf {

Text::Text(Engine& engine, const Ref<Font>& font)
    : Drawable2D(engine, "shaders/text.frag.spv")
    , m_font(font)
{
}

Text::~Text() = default;

void Text::update(float deltaTime)
{
    // The filter belongs to the shared Font, so it can change without going through Text
    if (m_font.ptr() != nullptr && m_font->getTextureFilter() != m_appliedFilter) {
        markDirty();
    }

    Drawable2D::update(deltaTime);
}

void Text::setFont(const Ref<Font>& font)
{
    if (m_font != font) {
        m_font = font;
        markDirty();
    }
}

void Text::setString(std::string_view utf8)
{
    if (m_string != utf8) {
        m_string = utf8;
        markDirty();
    }
}

void Text::setAlignment(TextAlignment alignment)
{
    if (m_alignment != alignment) {
        m_alignment = alignment;
        markDirty();
    }
}

void Text::rebuild()
{
    if (m_font.ptr() == nullptr || m_string.empty()) {
        return;
    }

    FontAtlas* atlas = m_font->getFontAtlas();
    if (atlas == nullptr) {
        OCF_LOG_WARN("Text: font '{}' has no atlas", m_font->getFontName());
        return;
    }

    m_appliedFilter = m_font->getTextureFilter();

    const std::u32string utf32Text = TextUtility::utf8ToUtf32(m_string);
    const TextLayoutResult layout = TextLayout::layout(*m_font.ptr(), utf32Text, m_alignment);
    setContentSize(layout.size);

    if (layout.quads.empty()) {
        return;
    }

    // Group the quads by atlas page, since each page is a separate texture and draw call
    const size_t pageCount = atlas->getPageCount();
    std::vector<std::vector<const GlyphQuad*>> quadsPerPage(pageCount);
    for (const GlyphQuad& quad : layout.quads) {
        if (quad.page < 0 || static_cast<size_t>(quad.page) >= pageCount) {
            OCF_LOG_WARN("Text: glyph refers to missing atlas page {}", quad.page);
            continue;
        }
        quadsPerPage[quad.page].push_back(&quad);
    }

    if (m_pages.size() < pageCount) {
        m_pages.resize(pageCount);
    }

    std::vector<VertexPositionTexCoord> vertices;
    std::vector<uint32_t> indices;

    for (size_t pageIndex = 0; pageIndex < pageCount; pageIndex++) {
        const auto& quads = quadsPerPage[pageIndex];
        if (quads.empty()) {
            continue;
        }

        Texture* texture = atlas->getTexture(static_cast<unsigned int>(pageIndex));
        const float invTexWidth = 1.0f / static_cast<float>(texture->getWidth());
        const float invTexHeight = 1.0f / static_cast<float>(texture->getHeight());

        vertices.clear();
        indices.clear();
        vertices.reserve(quads.size() * 4);
        indices.reserve(quads.size() * 6);

        for (const GlyphQuad* quad : quads) {
            const math::vec2 uv0(quad->texRect.getMinX() * invTexWidth,
                                 quad->texRect.getMinY() * invTexHeight);
            const math::vec2 uv1(quad->texRect.getMaxX() * invTexWidth,
                                 quad->texRect.getMaxY() * invTexHeight);
            appendQuad(vertices, indices, quad->position, quad->size, uv0, uv1);
        }

        MaterialInstance* materialInstance =
            preparePageMaterial(m_pages[pageIndex], texture, m_appliedFilter);
        addGeometry(vertices, indices, materialInstance);
    }
}

MaterialInstance* Text::preparePageMaterial(Page& page, Texture* texture, TextureFilter filter)
{
    const bool isNewInstance = page.materialInstance == nullptr;
    if (isNewInstance) {
        page.materialInstance = createMaterialInstance();
    }

    // Only touch the descriptor when the texture or filter changes (e.g. after setFont())
    if (isNewInstance || page.texture != texture || page.filter != filter) {
        bindTexture(page.materialInstance, texture, filter);
        page.texture = texture;
        page.filter = filter;
    }

    return page.materialInstance;
}

} // namespace ocf
