#include "vec4.h"
#include <assert.h>

namespace ocf {
namespace math {

template <typename T>
inline vec<4, T>::vec(T _x, T _y, T _z, T _w)
    : x(_x)
    , y(_y)
    , z(_z)
    , w(_w)
{
}

template <typename T>
inline vec<4, T>::vec(T scalar)
    : x(scalar)
    , y(scalar)
    , z(scalar)
    , w(scalar)
{
}

template <typename T>
template <typename A, typename B, typename C>
inline constexpr vec<4, T>::vec(const vec<2, A>& _xy, B _z, C _w)
    : x(static_cast<T>(_xy.x))
    , y(static_cast<T>(_xy.y))
    , z(static_cast<T>(_z))
    , w(static_cast<T>(_w))
{
}

template <typename T>
template <typename A, typename B>
inline constexpr vec<4, T>::vec(const vec<3, A>& _xyz, B _w)
    : x(static_cast<T>(_xyz.x))
    , y(static_cast<T>(_xyz.y))
    , z(static_cast<T>(_xyz.z))
    , w(static_cast<T>(_w))
{
}

// Component accessor
template<typename T>
inline T& vec<4, T>::operator[](length_t i)
{
    assert(i >= 0 && i < this->length());

    switch (i) {
    case 0:
        return x;
    case 1:
        return y;
    case 2:
        return z;
    case 3:
        return w;
    default:
        return x;
    }
}

template<typename T>
inline const T& vec<4, T>::operator[](length_t i) const
{
    assert(i >= 0 && i < this->length());
    switch (i) {
    case 0:
        return x;
    case 1:
        return y;
    case 2:
        return z;
    case 3:
        return w;
    default:
        return x;
    }
}

template <typename T>
template <typename U>
inline constexpr vec<4, T>& vec<4, T>::operator=(const vec<4, U>& v)
{
    this->x = static_cast<T>(v.x);
    this->y = static_cast<T>(v.y);
    this->z = static_cast<T>(v.z);
    this->w = static_cast<T>(v.w);
    return *this;
}

template <typename T>
template <typename U>
inline constexpr vec<4, T>& vec<4, T>::operator+=(const vec<4, U>& v)
{
    this->x += static_cast<T>(v.x);
    this->y += static_cast<T>(v.y);
    this->z += static_cast<T>(v.z);
    this->w += static_cast<T>(v.w);
    return *this;
}

template <typename T>
template <typename U>
inline constexpr vec<4, T>& vec<4, T>::operator-=(const vec<4, U>& v)
{
    this->x -= static_cast<T>(v.x);
    this->y -= static_cast<T>(v.y);
    this->z -= static_cast<T>(v.z);
    this->w -= static_cast<T>(v.w);
    return *this;
}

template <typename T>
template <typename U>
inline constexpr vec<4, T>& vec<4, T>::operator*=(const U& scalar)
{
    this->x *= static_cast<T>(scalar);
    this->y *= static_cast<T>(scalar);
    this->z *= static_cast<T>(scalar);
    this->w *= static_cast<T>(scalar);
    return *this;
}

template <typename T>
template <typename U>
inline constexpr vec<4, T>& vec<4, T>::operator/=(const U& scalar)
{
    this->x /= static_cast<T>(scalar);
    this->y /= static_cast<T>(scalar);
    this->z /= static_cast<T>(scalar);
    this->w /= static_cast<T>(scalar);
    return *this;
}

template <typename T> inline constexpr vec<4, T> operator+(const vec<4, T>& v1, const vec<4, T>& v2)
{
    return vec<4, T>(
        v1.x + v2.x,
        v1.y + v2.y,
        v1.z + v2.z,
        v1.w + v2.w);
}

template <typename T> inline constexpr vec<4, T> operator-(const vec<4, T>& v1, const vec<4, T>& v2)
{
    return vec<4, T>(
        v1.x - v2.x,
        v1.y - v2.y,
        v1.z - v2.z,
        v1.w - v2.w);
}

template <typename T> inline constexpr vec<4, T> operator-(const vec<4, T>& v, const T& scalar)
{
    return vec<4, T>(
        v.x - scalar,
        v.y - scalar,
        v.z - scalar,
        v.w - scalar);
}

template <typename T> inline constexpr vec<4, T> operator*(const vec<4, T>& v1, const T& scalar)
{
    return vec<4, T>(
        v1.x * scalar,
        v1.y * scalar,
        v1.z * scalar,
        v1.w * scalar);
}

template <typename T> inline constexpr vec<4, T> operator*(const vec<4, T>& v1, const vec<4, T>& v2)
{
    return vec<4, T>(
        v1.x * v2.x,
        v1.y * v2.y,
        v1.z * v2.z,
        v1.w * v2.w);
}

template <typename T> inline constexpr vec<4, T> operator/(const vec<4, T>& v1, const T& scalar)
{
    return vec<4, T>(
        v1.x / scalar,
        v1.y / scalar,
        v1.z / scalar,
        v1.w / scalar);
}

template <typename T> inline constexpr bool operator==(const vec<4, T>& v1, const vec<4, T>& v2)
{
    return (v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z) && (v1.w == v2.w);
}

template <typename T> inline constexpr bool operator!=(const vec<4, T>& v1, const vec<4, T>& v2)
{
    return !(v1 == v2);
}

template <typename T>
inline constexpr T dot(const vec<4, T>& v1, const vec<4, T>& v2)
{
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}

} // namespace math
} // namespace ocf
