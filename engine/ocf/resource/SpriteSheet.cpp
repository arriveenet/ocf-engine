// SPDX-License-Identifier: MIT
#include "ocf/resource/SpriteSheet.h"

#include <algorithm>
#include <utility>

namespace ocf {

SpriteFrame SpriteFrame::fromRect(const math::Rect& rect)
{
    SpriteFrame frame;
    frame.textureRect = rect;
    frame.sourceSize = rect.m_size;
    return frame;
}

math::Rect SpriteFrame::getPackedRect() const
{
    if (!rotated) {
        return textureRect;
    }
    return math::Rect(textureRect.m_position,
                      math::vec2(textureRect.m_size.y, textureRect.m_size.x));
}

SpriteQuad computeSpriteQuad(const SpriteFrame& frame, const math::vec2& textureSize, bool flipX,
                             bool flipY)
{
    SpriteQuad quad;
    quad.size = frame.textureRect.m_size;
    quad.position = frame.offset;

    const math::Rect packed = frame.getPackedRect();
    const float u0 = packed.getMinX() / textureSize.x;
    const float v0 = packed.getMinY() / textureSize.y;
    const float u1 = packed.getMaxX() / textureSize.x;
    const float v1 = packed.getMaxY() / textureSize.y;

    // Corners are top left, bottom left, bottom right, top right of the image as drawn
    if (frame.rotated) {
        // Stored rotated clockwise: the left edge of the image is the top edge in the texture
        quad.uvs = {math::vec2(u1, v0), math::vec2(u0, v0), math::vec2(u0, v1),
                    math::vec2(u1, v1)};
    } else {
        quad.uvs = {math::vec2(u0, v0), math::vec2(u0, v1), math::vec2(u1, v1),
                    math::vec2(u1, v0)};
    }

    if (flipX) {
        std::swap(quad.uvs[0], quad.uvs[3]);
        std::swap(quad.uvs[1], quad.uvs[2]);
        quad.position.x = frame.sourceSize.x - frame.offset.x - quad.size.x;
    }
    if (flipY) {
        std::swap(quad.uvs[0], quad.uvs[1]);
        std::swap(quad.uvs[3], quad.uvs[2]);
        quad.position.y = frame.sourceSize.y - frame.offset.y - quad.size.y;
    }
    return quad;
}

SpriteSheet::SpriteSheet() = default;

SpriteSheet::~SpriteSheet() = default;

void SpriteSheet::setImage(std::string path, uint32_t width, uint32_t height)
{
    m_imagePath = std::move(path);
    m_imageWidth = width;
    m_imageHeight = height;
}

void SpriteSheet::addFrame(SpriteFrame frame)
{
    const auto it = m_frameIndices.find(frame.name);
    if (it != m_frameIndices.end()) {
        m_frames[it->second] = std::move(frame);
        return;
    }
    m_frameIndices.emplace(frame.name, m_frames.size());
    m_frames.push_back(std::move(frame));
}

const SpriteFrame* SpriteSheet::getFrame(std::string_view name) const
{
    const auto it = m_frameIndices.find(std::string(name));
    return it != m_frameIndices.end() ? &m_frames[it->second] : nullptr;
}

std::vector<const SpriteFrame*> SpriteSheet::getFramesWithPrefix(std::string_view prefix) const
{
    std::vector<const SpriteFrame*> frames;
    for (const SpriteFrame& frame : m_frames) {
        if (std::string_view(frame.name).starts_with(prefix)) {
            frames.push_back(&frame);
        }
    }
    std::sort(frames.begin(), frames.end(),
              [](const SpriteFrame* a, const SpriteFrame* b) { return a->name < b->name; });
    return frames;
}

} // namespace ocf
