// SPDX-License-Identifier: MIT
#include "ocf/resource/FontManager.h"

#include "ocf/core/Logger.h"
#include "ocf/platform/FileSystem.h"
#include "ocf/resource/Font.h"
#include "ocf/resource/FontAtlas.h"
#include "ocf/resource/FontFNT.h"
#include "ocf/resource/FontTrueType.h"

#include <filesystem>
#include <format>

namespace ocf {

FontManager::FontManager(Engine& engine)
    : m_engine(engine)
{
}

FontManager::~FontManager() = default;

Ref<Font> FontManager::loadFontFNT(std::string_view fontFileName, TextureFilter filter)
{
    // The type prefix keeps FNT and TTF keys from colliding
    std::string key =
        std::format("fnt:{}:{}", resolvePath(fontFileName), static_cast<int>(filter));

    if (auto iter = m_fontMap.find(key); iter != m_fontMap.end()) {
        return iter->second;
    }

    // FontFNT::create() logs why loading failed
    return addFont(std::move(key), FontFNT::create(fontFileName), filter);
}

Ref<Font> FontManager::loadFontTTF(const FontTrueTypeConfig& config)
{
    // The glyph collection decides which glyphs the atlas is pre-filled with, so fonts that
    // differ only in it cannot be shared
    std::string key = std::format("ttf:{}:{}:{}:{}", resolvePath(config.fontPath),
                                  config.fontSize, static_cast<int>(config.glyphs),
                                  static_cast<int>(config.filter));

    if (auto iter = m_fontMap.find(key); iter != m_fontMap.end()) {
        return iter->second;
    }

    // FontTrueType::create() logs why loading failed
    FontTrueType* font = FontTrueType::create(config.fontPath, config.fontSize, config.glyphs);
    return addFont(std::move(key), font, config.filter);
}

bool FontManager::unloadFont(const Ref<Font>& font)
{
    if (font.ptr() == nullptr) {
        return false;
    }

    for (auto iter = m_fontMap.begin(); iter != m_fontMap.end(); ++iter) {
        if (iter->second == font) {
            m_fontMap.erase(iter);
            return true;
        }
    }

    return false;
}

size_t FontManager::removeUnusedFonts()
{
    // The cache's own Ref accounts for one reference
    return std::erase_if(m_fontMap, [](const auto& entry) {
        return entry.second->getReferenceCount() <= 1;
    });
}

void FontManager::clear()
{
    m_defaultFont = nullptr;
    m_fontMap.clear();
}

Ref<Font> FontManager::addFont(std::string&& key, Font* font, TextureFilter filter)
{
    if (font == nullptr) {
        return nullptr;
    }

    // Own the font right away so it is freed on every failure path
    Ref<Font> ref(font);

    font->setTextureFilter(filter);
    if (font->createFontAtlas(m_engine) == nullptr) {
        OCF_LOG_ERROR("Could not create font atlas: {}", font->getFontName());
        return nullptr;
    }

    // Return the stored copy; ref itself is empty after being moved into the map
    return m_fontMap.emplace(std::move(key), std::move(ref)).first->second;
}

std::string FontManager::resolvePath(std::string_view fileName)
{
    const std::string fullPath = FileSystem::getInstance()->getAssetFullPath(fileName);
    if (fullPath.empty()) {
        // Missing file: loading fails anyway, so the key only needs to be deterministic
        return std::string(fileName);
    }

    return std::filesystem::path(fullPath).lexically_normal().generic_string();
}

} // namespace ocf
