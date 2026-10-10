#include "ocf/resource/FontAtlas.h"

#include "ocf/core/Engine.h"
#include "ocf/math/MaxRectsBinPack.h"
#include "ocf/renderer/Texture.h"

#include <cstdlib>
#include <cstring>

using namespace ocf::math;

namespace ocf {

FontAtlas::FontAtlas(Engine& engine)
    : FontAtlas(engine, DEFAULT_TEXTURE_WIDTH, DEFAULT_TEXTURE_HEIGHT)
{
}

FontAtlas::FontAtlas(Engine& engine, int width, int height)
    : m_engine(engine)
    , m_width(width)
    , m_height(height)
{
    m_binPack = std::make_unique<MaxRectsBinPack>();
}

FontAtlas::~FontAtlas()
{
    releaseTextures();
}

void FontAtlas::addNewPage()
{
    // Upload the pending glyphs of the previous page before its CPU data is cleared
    updateTexture();

    m_binPack->init(static_cast<float>(m_width), static_cast<float>(m_height));

    m_currentPageData.assign(static_cast<size_t>(m_width) * m_height, 0);

    Texture* texture = Texture::Builder()
                            .sampler(Texture::Sampler::Sampler2D)
                            .format(Texture::InternalFormat::R8)
                            .width(m_width)
                            .height(m_height)
                            .levels(1)
                            .build(m_engine);

    m_atlasTextures[++m_currentPage] = texture;
    m_ownedTextures.insert(texture);
}

bool FontAtlas::insert(math::Rect& outRect, const uint8_t* bitmap, int width, int height)
{
    if (m_currentPage < 0) {
        addNewPage();
    }

    // Reserve a transparent border around the glyph so linear filtering at its edges does not
    // pick up texels of the neighboring glyphs
    const int paddedWidth = width + GLYPH_PADDING * 2;
    const int paddedHeight = height + GLYPH_PADDING * 2;

    Rect result =
        m_binPack->insert(static_cast<float>(paddedWidth), static_cast<float>(paddedHeight));
    if (result.m_size.x == 0 || result.m_size.y == 0) {
        return false;
    }

    const int resultX = static_cast<int>(result.m_position.x) + GLYPH_PADDING;
    const int resultY = static_cast<int>(result.m_position.y) + GLYPH_PADDING;

    // Copy the bitmap data into the current page's texture
    for (int y = 0; y < height; y++) {
        const size_t destIndex = static_cast<size_t>(resultY + y) * m_width + resultX;
        const size_t srcIndex = static_cast<size_t>(width) * y;
        memcpy(&m_currentPageData[destIndex], &bitmap[srcIndex], width);
    }

    m_dirty = true;
    // Callers only see the glyph itself, not the padding
    outRect = Rect(static_cast<float>(resultX), static_cast<float>(resultY),
                   static_cast<float>(width), static_cast<float>(height));

    return true;
}

void FontAtlas::updateTexture()
{
    if (!m_dirty || m_currentPage < 0) {
        return;
    }

    // The device may upload asynchronously, so hand it a copy that it frees when done
    const size_t size = m_currentPageData.size();
    void* data = malloc(size);
    memcpy(data, m_currentPageData.data(), size);

    Texture::PixelBufferDescriptor buffer(
        data, size, Texture::Format::R, Texture::Type::Ubyte,
        [](void* buffer, size_t, void*) { free(buffer); });

    Texture* currentTexture = m_atlasTextures[m_currentPage];
    currentTexture->setImage(m_engine, 0, std::move(buffer));

    m_dirty = false;
}

void FontAtlas::releaseTextures()
{
    // Textures set from outside (e.g. via TextureManager) are not owned by the atlas
    for (Texture* texture : m_ownedTextures) {
        texture->terminate(m_engine);
        delete texture;
    }
    m_ownedTextures.clear();
    m_atlasTextures.clear();
    m_currentPage = -1;
}

void FontAtlas::setTexure(unsigned int slot, Texture* texture)
{
    m_atlasTextures[slot] = texture;
}

Texture* FontAtlas::getTexture(unsigned int slot) const
{
    return m_atlasTextures.at(slot);
}

} // namespace ocf
