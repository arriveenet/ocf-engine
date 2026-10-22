#pragma once

#include "ocf/core/Reference.h"
#include "ocf/math/Rect.h"
#include "ocf/resource/Resource.h"
#include "ocf/resource/Font.h"

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace ocf {

class BMFontConfiguration : public Resource {
public:
    struct BMFontDef {
        unsigned int charID = 0;
        math::Rect rect;
        short xOffset = 0;
        short yOffset = 0;
        short xAdvance = 0;
        unsigned char page = 0;
    };

    struct BMFontPadding {
        int left = 0;
        int top = 0;
        int right = 0;
        int bottom = 0;
    };

    static BMFontConfiguration* create(std::string_view fntFile);

    BMFontConfiguration();
    virtual ~BMFontConfiguration();

    bool initWithFNTFile(std::string_view fntFile);

protected:
    virtual std::set<unsigned int>* parseConfigFile(std::string_view controlFile);
    virtual std::set<unsigned int>* parseBinaryConfigFile(unsigned char* pData, uint32_t size, std::string_view controlFile);

public:
    std::unordered_map<int, BMFontDef> m_fontDefDictionary;
    int m_commonHeight;
    unsigned short m_pages;
    BMFontPadding m_padding;
    std::set<unsigned int>* m_charactorSet;
    int m_fontSize;
    std::string m_fontName;
    std::vector<std::string> m_atlasNames;
};

class FontFNT : public Font {
public:
    static FontFNT* create(std::string_view fntFilePath);

    FontAtlas* createFontAtlas(Engine& engine) override;

protected:
    FontFNT(BMFontConfiguration* config);
    ~FontFNT();

    Ref<BMFontConfiguration> m_pConfiguration;
};

} // namespace ocf
