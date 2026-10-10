#include "ocf/resource/Font.h"

#include "ocf/resource/FontAtlas.h"

namespace ocf {

Font::Font() = default;

Font::~Font() = default;

void Font::addCharacterDefinition(char32_t utf32char, const CharacterDefinition& defintition)
{
    m_characterDefinition[utf32char] = defintition;
}

bool Font::getCharacterDefinition(char32_t utf32char, CharacterDefinition& defintition) const
{
    auto iter = m_characterDefinition.find(utf32char);
    if (iter != m_characterDefinition.end()) {
        defintition = (*iter).second;
        return true;
    }
    else {
        return false;
    }
}

void Font::setLineHeight(float lineHeight)
{
    m_lineHeight = lineHeight;
}

std::string_view Font::getFontName() const
{
    return m_fontName;
}

} // namespace ocf
