#include "ocf/math/Rect.h"

namespace ocf {
namespace math {

Rect::Rect()
{
    setRect(0.0f, 0.0f, 0.0f, 0.0f);
}

Rect::Rect(float x, float y, float widht, float height)
{
    setRect(x, y, widht, height);
}

Rect::Rect(const vec2& position, const vec2& size)
{
    setRect(position.x, position.y, size.x, size.y);
}

void Rect::setRect(float x, float y, float widht, float height)
{
    m_position.x = x;
    m_position.y = y;

    m_size.x = widht;
    m_size.y = height;
}

float Rect::getMaxX() const
{
    return m_position.x + m_size.x;
}

float Rect::getMaxY() const
{
    return m_position.y + m_size.y;
}

float Rect::getMinX() const
{
    return m_position.x;
}

float Rect::getMinY() const
{
    return m_position.y;
}

bool Rect::equals(const Rect& rect) const
{
    return (m_position == rect.m_position && m_size == rect.m_size);
}

bool Rect::intersect(const vec2& point) const
{
    if (point.x >= getMinX() && point.x <= getMaxX() && point.y >= getMinY() &&
        point.y <= getMaxY()) {
        return true;
    }

    return false;
}

bool Rect::intersect(const Rect& rect) const
{
    if ((getMaxX() > rect.getMinX()) && (getMinX() < rect.getMaxX()) &&
        (getMaxY() > rect.getMinY()) && (getMinY() < rect.getMaxY())) {
        return true;
    }
    return false;
}

bool Rect::contain(const Rect& rect) const
{
    if (getMinX() <= rect.getMinX() && getMinY() <= rect.getMinY() && getMaxX() >= rect.getMaxX() &&
        getMaxY() >= rect.getMaxY()) {
        return true;
    }
    return false;
}

} // namespace math
} // namespace ocf
