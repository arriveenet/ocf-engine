// SPDX-License-Identifier: MIT
#include "ocf/resource/TileSet.h"

#include "ocf/renderer/Texture.h"

namespace ocf {

TileSet::TileSet(uint32_t tileWidth, uint32_t tileHeight, uint32_t margin, uint32_t spacing)
    : m_tileWidth(tileWidth)
    , m_tileHeight(tileHeight)
    , m_margin(margin)
    , m_spacing(spacing)
{
}

TileSet::TileSet(Texture* texture, uint32_t tileWidth, uint32_t tileHeight, uint32_t margin,
                 uint32_t spacing)
    : TileSet(tileWidth, tileHeight, margin, spacing)
{
    setTexture(texture);
}

TileSet::~TileSet() = default;

void TileSet::setImage(std::string path, uint32_t width, uint32_t height)
{
    m_imagePath = std::move(path);
    fitGrid(width, height);
}

void TileSet::setTexture(Texture* texture)
{
    m_texture = texture;
    if (m_texture != nullptr && m_imageWidth == 0 && m_imageHeight == 0) {
        fitGrid(static_cast<uint32_t>(m_texture->getWidth()),
                static_cast<uint32_t>(m_texture->getHeight()));
    }
}

math::Rect TileSet::getTextureRect(uint32_t localId) const
{
    if (m_columns == 0 || localId >= m_tileCount) {
        return math::Rect();
    }

    const uint32_t column = localId % m_columns;
    const uint32_t row = localId / m_columns;
    return math::Rect(static_cast<float>(m_margin + column * (m_tileWidth + m_spacing)),
                      static_cast<float>(m_margin + row * (m_tileHeight + m_spacing)),
                      static_cast<float>(m_tileWidth), static_cast<float>(m_tileHeight));
}

const TileProperties* TileSet::getTileProperties(uint32_t localId) const
{
    auto iter = m_tileProperties.find(localId);
    return iter != m_tileProperties.end() ? &iter->second : nullptr;
}

void TileSet::setTileProperties(uint32_t localId, TileProperties properties)
{
    m_tileProperties[localId] = std::move(properties);
}

void TileSet::fitGrid(uint32_t imageWidth, uint32_t imageHeight)
{
    m_imageWidth = imageWidth;
    m_imageHeight = imageHeight;

    // A tile fits if its right edge does not pass the margin on the far side
    const auto countTiles = [this](uint32_t imageSize, uint32_t tileSize) -> uint32_t {
        if (tileSize == 0 || imageSize < 2 * m_margin + tileSize) {
            return 0;
        }
        return (imageSize - 2 * m_margin + m_spacing) / (tileSize + m_spacing);
    };

    m_columns = countTiles(imageWidth, m_tileWidth);
    m_tileCount = m_columns * countTiles(imageHeight, m_tileHeight);
}

} // namespace ocf
