#if !defined(OCF_USE_FREETYPE)

#include "FontEngineStb.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <cmath>

namespace ocf {

FontEngineStb::FontEngineStb() = default;

FontEngineStb::~FontEngineStb() = default;

bool FontEngineStb::loadFont(std::vector<unsigned char>&& data, int pixelSize)
{
    m_fontData = std::move(data);

    m_fontInfo = std::make_unique<stbtt_fontinfo>();
    const int offset = stbtt_GetFontOffsetForIndex(m_fontData.data(), 0);
    if (offset < 0 || stbtt_InitFont(m_fontInfo.get(), m_fontData.data(), offset) == 0) {
        m_fontInfo.reset();
        return false;
    }

    // Match FreeType's FT_Set_Pixel_Sizes: pixelSize is the em size
    m_scale = stbtt_ScaleForMappingEmToPixels(m_fontInfo.get(), static_cast<float>(pixelSize));

    int ascent = 0, descent = 0, lineGap = 0;
    stbtt_GetFontVMetrics(m_fontInfo.get(), &ascent, &descent, &lineGap);

    m_metrics.ascender = static_cast<int>(std::round(ascent * m_scale));
    m_metrics.descender = static_cast<int>(std::round(descent * m_scale));
    m_metrics.lineHeight = std::round((ascent - descent + lineGap) * m_scale);

    return true;
}

bool FontEngineStb::renderGlyph(char32_t codepoint, FontGlyph& outGlyph)
{
    const int glyphIndex = stbtt_FindGlyphIndex(m_fontInfo.get(), static_cast<int>(codepoint));
    if (glyphIndex == 0) {
        return false;
    }

    int advanceWidth = 0;
    int leftSideBearing = 0;
    stbtt_GetGlyphHMetrics(m_fontInfo.get(), glyphIndex, &advanceWidth, &leftSideBearing);

    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    stbtt_GetGlyphBitmapBox(m_fontInfo.get(), glyphIndex, m_scale, m_scale, &x0, &y0, &x1, &y1);

    outGlyph.width = x1 - x0;
    outGlyph.height = y1 - y0;
    outGlyph.left = x0;
    outGlyph.top = -y0; // stb_truetype boxes are y-down
    outGlyph.advance = std::round(static_cast<float>(advanceWidth) * m_scale);
    outGlyph.bitmap.assign(static_cast<size_t>(outGlyph.width) * outGlyph.height, 0);

    if (outGlyph.width > 0 && outGlyph.height > 0) {
        stbtt_MakeGlyphBitmap(m_fontInfo.get(), outGlyph.bitmap.data(), outGlyph.width,
                              outGlyph.height, outGlyph.width, m_scale, m_scale, glyphIndex);
    }

    return true;
}

std::unique_ptr<IFontEngine> IFontEngine::create()
{
    return std::make_unique<FontEngineStb>();
}

} // namespace ocf

#endif // !OCF_USE_FREETYPE
