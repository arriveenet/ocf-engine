// SPDX-License-Identifier: MIT
#pragma once
#include "ocf/resource/Font.h"
#include "ocf/resource/FontTrueType.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>

namespace ocf {

class Engine;

/**
 * @brief Loads fonts and caches them for sharing.
 *
 * A font is cached per file and per every setting that changes its atlas or sampling, so
 * loading the same combination again returns the same Font. Files are identified by their
 * resolved asset path, so different spellings of the same path share one font.
 *
 * Removing a font from the cache never invalidates it: holders of a Ref keep it alive, and it
 * is destroyed when the last Ref goes away.
 */
class FontManager {
public:
    explicit FontManager(Engine& engine);
    ~FontManager();

    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    /**
     * @brief Load a bitmap font (.fnt), or return the cached one.
     * @return The font, or nullptr if the file or its atlas images could not be loaded.
     */
    Ref<Font> loadFontFNT(std::string_view fontFileName,
                          TextureFilter filter = TextureFilter::Linear);

    /**
     * @brief Load a TrueType/OpenType font, or return the cached one.
     * @return The font, or nullptr if the file could not be loaded.
     */
    Ref<Font> loadFontTTF(const FontTrueTypeConfig& config);

    /**
     * @brief Remove a font from the cache.
     * @return false if the font was not cached.
     */
    bool unloadFont(const Ref<Font>& font);

    /**
     * @brief Remove every cached font that nothing outside the manager references.
     *
     * Call it at points such as scene changes to free the atlases of fonts no longer drawn.
     * @return The number of fonts removed.
     */
    size_t removeUnusedFonts();

    /** @brief Remove every font from the cache, including the default font. */
    void clear();

    /** @brief Font used when the caller has no particular font, e.g. debug text. */
    void setDefaultFont(const Ref<Font>& font) { m_defaultFont = font; }
    const Ref<Font>& getDefaultFont() const noexcept { return m_defaultFont; }

    size_t getFontCount() const noexcept { return m_fontMap.size(); }

private:
    /**
     * @brief Finish setting up a newly loaded font and add it to the cache.
     * @param font Takes ownership. May be nullptr when loading failed.
     */
    Ref<Font> addFont(std::string&& key, Font* font, TextureFilter filter);

    /** @brief Resolve a path to a canonical form so it can be used in a cache key. */
    static std::string resolvePath(std::string_view fileName);

    Engine& m_engine;
    std::unordered_map<std::string, Ref<Font>> m_fontMap;
    Ref<Font> m_defaultFont;
};

} // namespace ocf
