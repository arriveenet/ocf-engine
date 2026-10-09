// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/math/Rect.h"
#include "ocf/math/vec2.h"

#include <string_view>
#include <vector>

namespace ocf {

class Font;

enum class TextAlignment {
    Left,
    Center,
    Right,
};

/**
 * @brief One glyph placed by TextLayout::layout().
 *
 * All values are in pixels. The layout space has its origin at the top-left of the text block
 * with y pointing down, matching Font::CharacterDefinition.
 */
struct GlyphQuad {
    math::vec2 position = math::vec2(0.0f, 0.0f); // Top-left corner of the glyph in layout space
    math::vec2 size = math::vec2(0.0f, 0.0f);
    math::Rect texRect;  // Glyph area in the atlas page, in texels
    int page = 0;
};

struct TextLayoutResult {
    std::vector<GlyphQuad> quads;
    math::vec2 size = math::vec2(0.0f, 0.0f); // Bounding size of the whole text block
    int lineCount = 0;
};

class TextLayout {
public:
    TextLayout() = delete;

    /**
     * @brief Place the glyphs of a string using the character definitions of a font.
     *
     * '\n' starts a new line and '\r' is ignored. Characters missing from the font are
     * skipped, and glyphs without a bitmap (e.g. space) only advance the pen.
     *
     * For a FontTrueType with dynamic glyphs, call prepareLetterDefinitions() beforehand so
     * the characters of @p text are registered.
     */
    static TextLayoutResult layout(const Font& font, std::u32string_view text,
                                   TextAlignment alignment = TextAlignment::Left);
};

} // namespace ocf
