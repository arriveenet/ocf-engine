#pragma once

#include "ocf/resource/Font.h"

#include <memory>
#include <unordered_set>

namespace ocf {

class IFontEngine;

enum class GlyphCollection {
    Dynamic,
    Ascii,
    Custom,
};

struct FontTrueTypeConfig {
    static constexpr int DEFAULT_FONT_SIZE = 32;

    std::string fontPath;
    int fontSize = DEFAULT_FONT_SIZE; // Em size in pixels; must be positive
    GlyphCollection glyphs = GlyphCollection::Ascii;
    TextureFilter filter = TextureFilter::Linear;
};

class FontTrueType : public Font {
public:
    static FontTrueType* create(std::string_view fontPath, int fontSize, GlyphCollection glyphs);

    FontTrueType();
    virtual ~FontTrueType();

    FontAtlas* createFontAtlas(Engine& engine) override;

    bool prepareLetterDefinitions(const std::u32string& utf32Text);

private:
    bool initFont(std::string_view fontPath, int fontSize);

    void findNewCharacters(const std::u32string& utf32Text,
                           std::unordered_set<char32_t>& charset);

    void setGlyphCollection(GlyphCollection glyphs) { m_glyphCollection = glyphs; }
    std::string_view getGlyphCollection() const;

private:
    int m_fontSize;
    int m_ascender;
    int m_descender;
    std::unique_ptr<IFontEngine> m_fontEngine;
    GlyphCollection m_glyphCollection;
};

} // namespace ocf
