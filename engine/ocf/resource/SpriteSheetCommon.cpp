// SPDX-License-Identifier: MIT
#include "ocf/resource/SpriteSheetCommon.h"

namespace ocf::spritesheet {

std::optional<std::string> validateFrame(const SpriteFrame& frame)
{
    const math::Rect& rect = frame.textureRect;
    if (rect.m_position.x < 0.0f || rect.m_position.y < 0.0f || rect.m_size.x <= 0.0f ||
        rect.m_size.y <= 0.0f) {
        return "the region in the texture must have a non-negative position and a positive size";
    }
    if (frame.offset.x < 0.0f || frame.offset.y < 0.0f ||
        frame.offset.x + rect.m_size.x > frame.sourceSize.x ||
        frame.offset.y + rect.m_size.y > frame.sourceSize.y) {
        return "the trimmed image does not fit in sourceSize";
    }
    return std::nullopt;
}

} // namespace ocf::spritesheet
