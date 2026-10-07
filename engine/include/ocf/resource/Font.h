#pragma once

#include "ocf/resource/Resource.h"

#include <string>
#include <unordered_map>
#include <memory>

namespace ocf {

class Engine;
class FontAtlas;

class Font : public Resource {
public:
    struct CharacterDefinition {
        float x;
        float y;
        float width;
        float height;
        float xoffset;
        float yoffset;
        float xadvance;
        int page;
    };

    Font();
    virtual ~Font();

    void addCharacterDefinition(char32_t utf32char, const CharacterDefinition& defintition);
    bool getCharacterDefinition(char32_t utf32char, CharacterDefinition& defintition);

    float getLineHeight() const { return m_lineHeight; }
    void setLineHeight(float lineHeight);

    virtual FontAtlas* createFontAtlas(Engine& engine) = 0;

    std::string_view getFontName() const;

    FontAtlas* getFontAtlas() const { return m_fontAtlas.ptr(); }

protected:
    std::string m_fontName;
    Ref<FontAtlas> m_fontAtlas;
    float m_lineHeight = 0.0f;
    std::unordered_map<char32_t, CharacterDefinition> m_characterDefinition;

    friend class FontManager;
};

} // namespace ocf
