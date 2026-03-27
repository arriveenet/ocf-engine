#include "vec2.h"
namespace ocf {
namespace math {

template <typename T>
constexpr inline vec<2, T>::vec(T _x, T _y)
    : x(_x), y(_y)
{
}

template <typename T>
template <typename U>
inline vec<2, T>::vec(const vec<4, U>& v)
    : x(static_cast<T>(v.x))
    , y(static_cast<T>(v.y))
{
}

template <typename T>
template <typename A, typename B>
inline vec<2, T>::vec(A x, B y)
    : x(static_cast<T>(x))
    , y(static_cast<T>(y))
{
}

template <typename T>
template <typename U>
inline constexpr vec<2, T>& vec<2, T>::operator+=(const vec<2, U>& v)
{
    this->x += static_cast<T>(v.x);
    this->y += static_cast<T>(v.y);
    return *this;
}

template <typename T>
template <typename U>
inline constexpr vec<2, T>& vec<2, T>::operator-=(const vec<2, U>& v)
{
    this->x -= static_cast<T>(v.x);
    this->y -= static_cast<T>(v.y);
    return *this;
}

template <typename T>
template <typename U>
inline constexpr vec<2, T>& vec<2, T>::operator*=(const U& scalar)
{
    this->x *= static_cast<T>(scalar);
    this->y *= static_cast<T>(scalar);
    return *this;
}

template <typename T>
template <typename U>
inline constexpr vec<2, T>& vec<2, T>::operator/=(const U& scalar)
{
    this->x /= static_cast<T>(scalar);
    this->y /= static_cast<T>(scalar);
    return *this;
}

template <typename T>
inline constexpr vec<2, T> operator+(const vec<2, T>& v1, const vec<2, T>& v2)
{
    return vec<2, T>(
        v1.x + v2.x,
        v1.y + v2.y);
}

template <typename T>
inline constexpr vec<2, T> operator-(const vec<2, T>& v1, const vec<2, T>& v2)
{
    return vec<2, T>(
        v1.x - v2.x,
        v1.y - v2.y);
}

template <typename T>
inline constexpr vec<2, T> operator*(const vec<2, T>& v1, const T& scalar)
{
    return vec<2, T>(
        v1.x * scalar,
        v1.y * scalar);
}

template <typename T> inline constexpr vec<2, T> operator/(const vec<2, T>& v1, const T& scalar)
{
    return vec<2, T>(
        v1.x / scalar,
        v1.y / scalar);
}

template <typename T>
inline constexpr bool operator==(const vec<2, T>& v1, const vec<2, T>& v2)
{
    return (v1.x == v2.x) && (v1.y == v2.y);
}

template <typename T> inline constexpr bool operator!=(const vec<2, T>& v1, const vec<2, T>& v2)
{
    return !(v1 == v2);
}

} // namespace math
} // namespace ocf
