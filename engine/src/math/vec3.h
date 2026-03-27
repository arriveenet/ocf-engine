#pragma once
#include "ocf/math/qualifie.h"

namespace ocf {
namespace math {

template <typename T> struct vec<3, T> {

#if OCF_XYZW_ONLY
    T x, y, z;
#else
    union { T x, s; };
    union { T y, t; };
    union { T z, p; };
#endif

    static constexpr length_t length() { return 3; }

    vec() = default;

    // Explicit base constructor
    explicit vec(const T& scalar);
    vec(T _x, T _y, T _z);

    // Conversion constructor (from different types)
    template <typename A, typename B>
    constexpr vec(const vec<2, A>& _xy, B _z);

    template <typename U>
    constexpr vec(const vec<4, U>& v);

    template <typename X, typename Y, typename Z>
    constexpr vec(X x, Y y, Z z);

    template <typename U> inline constexpr vec<3, T>& operator+=(const vec<3, U>& v);

    template <typename U> inline constexpr vec<3, T>& operator-=(const vec<3, U>& v);

    template <typename U> inline constexpr vec<3, T>& operator*=(const U& scalar);

    template <typename U> inline constexpr vec<3, T>& operator/=(const U& scalar);
};

// arithmetic operators
template <typename T>
inline constexpr vec<3, T> operator+(const vec<3, T>& v1, const vec<3, T>& v2);

template <typename T>
inline constexpr vec<3, T> operator-(const vec<3, T>& v1, const vec<3, T>& v2);

template <typename T>
inline constexpr vec<3, T> operator*(const vec<3, T>& v1, const T& scalar);

template <typename T>
inline constexpr vec<3, T> operator*(const T& scalar, const vec<3, T>& v);

template <typename T>
inline constexpr vec<3, T> operator/(const vec<3, T>& v1, const T& scalar);

// boolean operators
template <typename T> inline constexpr bool operator==(const vec<3, T>& v1, const vec<3, T>& v2);

template <typename T> inline constexpr bool operator!=(const vec<3, T>& v1, const vec<3, T>& v2);

using u8vec3 = vec<3, uint8_t>;
using ivec3 = vec<3, int>;
using vec3 = vec<3, float>;

} // namespace math
} // namespace ocf

#include "ocf/math/vec3.inl"
