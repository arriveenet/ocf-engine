#pragma once
#include "ocf/resource/Font.h"

#include <string>
#include <unordered_map>

namespace ocf {

class Engine;
struct FontTrueTypeConfig;

class FontManager {
public:
    FontManager(Engine& engine);
    ~FontManager();

    Ref<Font> getFontFNT(std::string_view fontFileName);
    Ref<Font> getFontTTF(const FontTrueTypeConfig& config);

    void release();

private:
    Engine& m_engine;
    std::unordered_map<std::string, Ref<Font>> m_fontMap;
};

} // namespace ocf
