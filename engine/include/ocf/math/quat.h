#pragma once
#include "ocf/math/qualifie.h"
#include "ocf/math/vec4.h"
#include "ocf/math/vec3.h"

namespace ocf {
namespace math {

template <typename T> struct qua {
    typedef qua<T> type;
    typedef T value_type;
    
    T x, y, z, w;

    static constexpr length_t length() { return 4; }

    qua() = default;

    // Identity quaternion constructor 
    explicit qua(T s);

    // Component constructor
    qua(T _x, T _y, T _z, T _w);

    // Constructor from axis and angle (axis should be normalized)
    qua(const vec<3, T>& axis, T angle);

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

using quat = qua<float>;
using dquat = qua<double>;

} // namespace math
} // namespace ocf

#include "ocf/math/quat.inl"