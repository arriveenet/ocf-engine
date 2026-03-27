#pragma once
#include "ocf/math/qualifie.h"

namespace ocf {
namespace math {

template <typename T> struct vec<4, T> {

#if OCF_XYZW_ONLY
    T x, y, z, w;
#else
    union { T x, s; };
    union { T y, t; };
    union { T z, p; };
    union { T w, q; };
#endif

    static constexpr length_t length() { return 4; }

    vec() = default;

    vec(T _x, T _y, T _z, T _w);

    // Scalar constructor (fills all components with the same value)
    explicit vec(T scalar);

    // Conversion constructor (from different types)
    template<typename A, typename B, typename C>
    constexpr vec(const vec<2, A>& _xy, B _z, C _w);

    template <typename A, typename B>
    constexpr vec(const vec<3, A>& _xyz, B _w);

    // Array indexer
    T& operator[](length_t i);
    const T& operator[](length_t i) const;

    template <typename U>
    constexpr vec<4, T>& operator=(const vec<4, U>& v);

    template <typename U> inline constexpr vec<4, T>& operator+=(const vec<4, U>& v);

    template <typename U> inline constexpr vec<4, T>& operator-=(const vec<4, U>& v);

    template <typename U> inline constexpr vec<4, T>& operator*=(const U& scalar);

    template <typename U> inline constexpr vec<4, T>& operator/=(const U& scalar);
};

// arithmetic operators
template <typename T>
inline constexpr vec<4, T> operator+(const vec<4, T>& v1, const vec<4, T>& v2);

template <typename T>
inline constexpr vec<4, T> operator-(const vec<4, T>& v1, const vec<4, T>& v2);

template <typename T>
inline constexpr vec<4, T> operator-(const vec<4, T>& v, const T& scalar);

template <typename T> inline constexpr vec<4, T> operator*(const vec<4, T>& v1, const T& scalar);

template <typename T> inline constexpr vec<4, T> operator*(const vec<4, T>& v1, const vec<4, T>& v2);

template <typename T> inline constexpr vec<4, T> operator/(const vec<4, T>& v1, const T& scalar);

// boolean operators
template <typename T> inline constexpr bool operator==(const vec<4, T>& v1, const vec<4, T>& v2);

template <typename T> inline constexpr bool operator!=(const vec<4, T>& v1, const vec<4, T>& v2);

// Dot product
template <typename T>
inline constexpr T dot(const vec<4, T>& v1, const vec<4, T>& v2);

using u8vec4 = vec<4, uint8_t>;
using ivec4 = vec<4, int>;
using vec4 = vec<4, float>;

} // namespace math
} // namespace ocf

#include "ocf/math/vec4.inl"
