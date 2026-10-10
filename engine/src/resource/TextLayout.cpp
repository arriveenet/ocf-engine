// SPDX-License-Identifier: MIT
#include "ocf/resource/TextLayout.h"

#include "ocf/resource/Font.h"

#include <algorithm>

namespace ocf {

namespace {

struct LineInfo {
    size_t firstQuad = 0; // Index of the first quad belonging to the line
    float width = 0.0f;
};

float alignmentFactor(TextAlignment alignment)
{
    switch (alignment) {
    case TextAlignment::Center:
        return 0.5f;
    case TextAlignment::Right:
        return 1.0f;
    case TextAlignment::Left:
    default:
        return 0.0f;
    }
}

} // namespace

TextLayoutResult TextLayout::layout(const Font& font, std::u32string_view text,
                                    TextAlignment alignment)
{
    TextLayoutResult result;

    if (text.empty()) {
        return result;
    }

    const float lineHeight = font.getLineHeight();

    std::vector<LineInfo> lines(1);
    float penX = 0.0f;
    float penY = 0.0f;

    for (char32_t charCode : text) {
        if (charCode == U'\r') {
            continue;
        }

        if (charCode == U'\n') {
            lines.back().width = penX;
            lines.push_back({.firstQuad = result.quads.size()});
            penX = 0.0f;
            penY += lineHeight;
            continue;
        }

        Font::CharacterDefinition definition;
        if (!font.getCharacterDefinition(charCode, definition)) {
            continue;
        }

        if (definition.width > 0.0f && definition.height > 0.0f) {
            GlyphQuad quad;
            quad.position = math::vec2(penX + definition.xoffset, penY + definition.yoffset);
            quad.size = math::vec2(definition.width, definition.height);
            quad.texRect = math::Rect(definition.x, definition.y, definition.width,
                                      definition.height);
            quad.page = definition.page;
            result.quads.push_back(quad);
        }

        penX += definition.xadvance;
    }
    lines.back().width = penX;

    float maxWidth = 0.0f;
    for (const LineInfo& line : lines) {
        maxWidth = std::max(maxWidth, line.width);
    }

    const float factor = alignmentFactor(alignment);
    if (factor > 0.0f) {
        for (size_t i = 0; i < lines.size(); i++) {
            const size_t end = (i + 1 < lines.size()) ? lines[i + 1].firstQuad
                                                      : result.quads.size();
            const float offset = (maxWidth - lines[i].width) * factor;
            for (size_t q = lines[i].firstQuad; q < end; q++) {
                result.quads[q].position.x += offset;
            }
        }
    }

    result.lineCount = static_cast<int>(lines.size());
    result.size = math::vec2(maxWidth, lineHeight * static_cast<float>(lines.size()));

    return result;
}

} // namespace ocf
