#include "ocf/resource/FontManager.h"

#include "ocf/resource/Font.h"
#include "ocf/resource/FontAtlas.h"
#include "ocf/resource/FontFNT.h"
#include "ocf/resource/FontTrueType.h"

#include <format>

namespace ocf {

FontManager::FontManager(Engine& engine)
    : m_engine(engine)
{
}

FontManager::~FontManager() = default;

Ref<Font> FontManager::getFontFNT(std::string_view fontFileName)
{
    std::string fileName(fontFileName);

    auto iter = m_fontMap.find(fileName);
    if (iter == m_fontMap.end()) {
        FontFNT* font = FontFNT::create(fontFileName);
        if (font != nullptr) {
            Ref<Font> ref(font);
            if (font->createFontAtlas(m_engine) == nullptr) {
                return nullptr;
            }
            return *m_fontMap.emplace(std::move(fileName), std::move(ref)).first->second;
        }
    }
    else {
        return iter->second;
    }

    return nullptr; // Ref<Font> can be constructed from nullptr
}

Ref<Font> FontManager::getFontTTF(const FontTrueTypeConfig& config)
{
    std::string key = std::format("{0}_{1}", config.fontPath, config.fontSize);

    auto iter = m_fontMap.find(key);
    if (iter == m_fontMap.end()) {
        FontTrueType* font = FontTrueType::create(config.fontPath,
                                                  config.fontSize,
                                                  config.glyphs);
        if (font != nullptr) {
            Ref<Font> ref(font);
            font->createFontAtlas(m_engine);
            m_fontMap.emplace(std::move(key), std::move(ref));
            return ref;
        }
    }
    else {
        return iter->second;
    }

    return nullptr; // Ref<Font> can be constructed from nullptr
}

void FontManager::release()
{
    m_fontMap.clear();
}

} // namespace ocf
