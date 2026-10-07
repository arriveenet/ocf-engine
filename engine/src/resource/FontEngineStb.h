#pragma once

#include "IFontEngine.h"

struct stbtt_fontinfo;

namespace ocf {

class FontEngineStb : public IFontEngine {
public:
    FontEngineStb();
    ~FontEngineStb() override;

    bool loadFont(std::vector<unsigned char>&& data, int pixelSize) override;

    FontMetrics getMetrics() const override { return m_metrics; }

    bool renderGlyph(char32_t codepoint, FontGlyph& outGlyph) override;

private:
    std::vector<unsigned char> m_fontData; // stbtt_fontinfo references this buffer
    std::unique_ptr<stbtt_fontinfo> m_fontInfo;
    float m_scale = 0.0f;
    FontMetrics m_metrics;
};

} // namespace ocf
