#pragma once
#include "ocf/math/vec2.h"

namespace ocf {
namespace math {

class Rect {
public:
    vec2 m_position;
    vec2 m_size;

public:
    Rect();
    Rect(float x, float y, float widht, float height);
    Rect(const vec2& position, const vec2& size);

    void setRect(float x, float y, float widht, float height);

    float getMaxX() const;
    float getMaxY() const;
    
    float getMinX() const;
    float getMinY() const;

    bool equals(const Rect& rect) const;

    bool intersect(const vec2& point) const;

    bool intersect(const Rect& rect) const;

    bool contain(const Rect& rect) const;
};

} // namespace math
} // namespace ocf
