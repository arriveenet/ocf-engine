#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace ocf {

struct FontMetrics {
    int ascender = 0;     // Pixels above the baseline
    int descender = 0;    // Pixels below the baseline (negative)
    float lineHeight = 0.0f;
};

struct FontGlyph {
    int width = 0;
    int height = 0;
    int left = 0;         // Horizontal offset from the pen position to the bitmap
    int top = 0;          // Distance from the baseline up to the top of the bitmap
    float advance = 0.0f;
    std::vector<uint8_t> bitmap; // 8-bit coverage, tightly packed (stride == width)
};

/**
 * @brief Backend that rasterizes TrueType/OpenType glyphs.
 *
 * The implementation is selected at build time (stb_truetype or FreeType).
 */
class IFontEngine {
public:
    static std::unique_ptr<IFontEngine> create();

    virtual ~IFontEngine() = default;

    /**
     * @brief Load a font from memory.
     * @param data Font file contents. The engine keeps it alive while in use.
     * @param pixelSize Em size in pixels.
     */
    virtual bool loadFont(std::vector<unsigned char>&& data, int pixelSize) = 0;

    /**
     * @brief Get the font metrics.
     * @return Font metrics including ascender, descender, and line height.
     */
    virtual FontMetrics getMetrics() const = 0;

    /**
     * @brief Rasterize the glyph for a code point.
     * @return false if the font has no glyph for the code point.
     */
    virtual bool renderGlyph(char32_t codepoint, FontGlyph& outGlyph) = 0;
};

} // namespace ocf
