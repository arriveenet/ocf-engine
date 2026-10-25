// SPDX-License-Identifier: MIT
#pragma once
#include "ocf/math/qualifie.h"

namespace ocf {
namespace math {

template <typename T> struct qua {
    typedef qua<T> type;
    typedef T value_type;
    
    T x, y, z, w;

    static constexpr length_t length() { return 4; }

    qua();

    // Identity quaternion constructor 
    explicit qua(T s);

    // Component constructor
    qua(T s, const vec<3, T>& v);
    qua(T _x, T _y, T _z, T _w);

    constexpr explicit qua(const vec<3, T>& eulerAngles);
    explicit qua(const mat<3, 3, T>& m);
    explicit qua(const mat<4, 4, T>& m);

    // Copy constructor
    qua(const type& q);

    // Component access
    T& operator[](length_t i);
    const T& operator[](length_t i) const;

    // Assignment operators
    template <typename U> qua<T>& operator=(const qua<U>& q);
    template <typename U> qua<T>& operator+=(const qua<U>& q);
    template <typename U> qua<T>& operator-=(const qua<U>& q);
    template <typename U> qua<T>& operator*=(const qua<U>& q);
    template <typename U> qua<T>& operator*=(const U& scalar);
    template <typename U> qua<T>& operator/=(const U& scalar);
};

// Arithmetic operators
template <typename T>
qua<T> operator+(const qua<T>& q1, const qua<T>& q2);

template <typename T>
qua<T> operator-(const qua<T>& q1, const qua<T>& q2);

template <typename T>
qua<T> operator*(const qua<T>& q1, const qua<T>& q2);

template <typename T>
qua<T> operator*(const qua<T>& q, const T& scalar);

template <typename T>
qua<T> operator*(const T& scalar, const qua<T>& q);

template <typename T>
qua<T> operator/(const qua<T>& q, const T& scalar);

// Unary operators
template <typename T>
qua<T> operator-(const qua<T>& q);

// Boolean operators
template <typename T>
bool operator==(const qua<T>& q1, const qua<T>& q2);

template <typename T>
bool operator!=(const qua<T>& q1, const qua<T>& q2);

// Conversion functions
template <typename T>
qua<T> quat_cast(const mat<3, 3, T>& m);

template <typename T>
qua<T> quat_cast(const mat<4, 4, T>& m);

template <typename T>
mat<3, 3, T> mat3_cast(const qua<T>& q);

template <typename T>
mat<4, 4, T> mat4_cast(const qua<T>& q);

using quat = qua<float>;
using dquat = qua<double>;

} // namespace math
} // namespace ocf

#include "ocf/math/quat.inl"