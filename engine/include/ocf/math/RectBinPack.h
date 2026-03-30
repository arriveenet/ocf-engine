#pragma once
#include "ocf/math/Rect.h"
#include "ocf/math/vec2.h"
#include <vector>

namespace ocf {

class RectBinPack {
public:
    RectBinPack(): m_binWidth(0.0f), m_binHeight(0.0f) {}
    virtual ~RectBinPack() = default;

    virtual void init(float width, float height) = 0;

    virtual void insert(std::vector<math::vec2>, std::vector<math::Rect>& dest) = 0;

    virtual math::Rect insert(float width, float height) = 0;

    float getBinWidth() const { return m_binWidth; }
    float getBinHeight() const { return m_binHeight; }

protected:
    float m_binWidth;
    float m_binHeight;
};

} // namespace ocf