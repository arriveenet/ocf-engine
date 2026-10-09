#include "ocf/resource/FontTrueType.h"

#include "ocf/core/Logger.h"
#include "ocf/math/Rect.h"
#include "ocf/platform/FileSystem.h"
#include "ocf/resource/FontAtlas.h"

#include "IFontEngine.h"

#include <cassert>
#include <fstream>
#include <iterator>

namespace ocf {

using namespace std::string_view_literals;

static constexpr std::string_view GLYPH_ASCII =
    "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~ "sv;

FontTrueType* FontTrueType::create(std::string_view fontPath, int fontSize, GlyphCollection glyphs)
{
    FontTrueType* pFont = new FontTrueType();

    if (pFont->initFont(fontPath, fontSize)) {
        pFont->setGlyphCollection(glyphs);
        return pFont;
    }
    delete pFont;

    return nullptr;
}

FontTrueType::FontTrueType()
    : m_fontSize(0)
    , m_ascender(0)
    , m_descender(0)
    , m_glyphCollection(GlyphCollection::Dynamic)
{
}

FontTrueType::~FontTrueType() = default;

FontAtlas* FontTrueType::createFontAtlas(Engine& engine)
{
    m_fontAtlas = Ref<FontAtlas>(new FontAtlas(engine));
    m_fontAtlas->addNewPage();

    if (m_glyphCollection != GlyphCollection::Dynamic) {
        // The built-in collections are ASCII only, so each char maps directly to a code point
        std::string_view collection = getGlyphCollection();
        std::u32string utf32Text(collection.begin(), collection.end());
        prepareLetterDefinitions(utf32Text);
    }

    return m_fontAtlas.ptr();
}

bool FontTrueType::prepareLetterDefinitions(const std::u32string& utf32Text)
{
    assert(m_fontAtlas != nullptr);
    assert(m_fontEngine != nullptr);

    std::unordered_set<char32_t> charCodeSet;
    findNewCharacters(utf32Text, charCodeSet);
    if (charCodeSet.empty()) {
        return false;
    }

    FontGlyph glyph;

    for (auto&& charCode : charCodeSet) {
        if (!m_fontEngine->renderGlyph(charCode, glyph)) {
            continue;
        }

        math::Rect tempRect;
        // Glyphs without a bitmap (e.g. space) only need their metrics
        if (glyph.width > 0 && glyph.height > 0) {
            if (!m_fontAtlas->insert(tempRect, glyph.bitmap.data(), glyph.width, glyph.height)) {
                m_fontAtlas->addNewPage();
                if (!m_fontAtlas->insert(tempRect, glyph.bitmap.data(), glyph.width, glyph.height)) {
                    OCF_LOG_WARN("Glyph U+{:04X} does not fit in the font atlas",
                                 static_cast<uint32_t>(charCode));
                    continue;
                }
            }
        }

        CharacterDefinition definition = {
            .x = tempRect.m_position.x,
            .y = tempRect.m_position.y,
            .width = tempRect.m_size.x,
            .height = tempRect.m_size.y,
            .xoffset = static_cast<float>(glyph.left),
            .yoffset = static_cast<float>(m_ascender - glyph.top),
            .xadvance = glyph.advance,
            .page = m_fontAtlas->getCurrentPage()
        };

        addCharacterDefinition(charCode, definition);
    }

    m_fontAtlas->updateTexture();

    return true;
}

bool FontTrueType::initFont(std::string_view fontPath, int fontSize)
{
    std::string path = FileSystem::getInstance()->getAssetFullPath(fontPath);

    if (path.empty()) {
        OCF_LOG_ERROR("Could not find font file: {}", fontPath);
        return false;
    }

    std::ifstream file(path, std::ios::binary);
    std::vector<unsigned char> data((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
    if (data.empty()) {
        OCF_LOG_ERROR("Could not read font file: {}", path);
        return false;
    }

    m_fontEngine = IFontEngine::create();
    if (!m_fontEngine->loadFont(std::move(data), fontSize)) {
        OCF_LOG_ERROR("Could not load font file: {}", path);
        m_fontEngine.reset();
        return false;
    }

    m_fontSize = fontSize;
    m_fontName = path;

    const FontMetrics metrics = m_fontEngine->getMetrics();
    m_ascender = metrics.ascender;
    m_descender = metrics.descender;
    m_lineHeight = metrics.lineHeight;

    return true;
}

void FontTrueType::findNewCharacters(const std::u32string& utf32Text,
                                     std::unordered_set<char32_t>& charset)
{
    if (m_characterDefinition.empty()) {
        std::copy(utf32Text.begin(), utf32Text.end(),
                  std::inserter(charset, charset.end()));
    }
    else {
        for (auto&& charCode : utf32Text) {
            if (m_characterDefinition.find(charCode) == m_characterDefinition.end()) {
                charset.insert(charCode);
            }
        }
    }
}

std::string_view FontTrueType::getGlyphCollection() const
{
    std::string_view collection;

    switch (m_glyphCollection) {
    case GlyphCollection::Dynamic:
        break;
    case GlyphCollection::Ascii:
        collection = GLYPH_ASCII;
        break;
    case GlyphCollection::Custom:
        break;
    default:
        break;
    }

    return collection;
}

} // namespace ocf
