#include <cmath>

#include "quat.h"
#include "mat3.h"

namespace ocf {
namespace math {

// Constructors

template <typename T>
inline qua<T>::qua()
    : x(T(0))
    , y(T(0))
    , z(T(0))
    , w(T(1))
{
}

template <typename T>
inline qua<T>::qua(T s)
    : x(T(0))
    , y(T(0))
    , z(T(0))
    , w(s)
{
}

template <typename T>
inline qua<T>::qua(T s, const vec<3, T>& v)
    : x(v.x)
    , y(v.y)
    , z(v.z)
    , w(s)
{
}

template <typename T>
inline qua<T>::qua(T _x, T _y, T _z, T _w)
    : x(_x)
    , y(_y)
    , z(_z)
    , w(_w)
{
}

template <typename T>
inline constexpr qua<T>::qua(const vec<3, T>& eulerAngles)
{
    vec<3, T> halfAngles = eulerAngles * T(0.5);
    vec<3, T> c = vec<3, T>(std::cos(halfAngles.x), std::cos(halfAngles.y), std::cos(halfAngles.z));
    vec<3, T> s = vec<3, T>(std::sin(halfAngles.x), std::sin(halfAngles.y), std::sin(halfAngles.z));

    this->w = c.x * c.y * c.z + s.x * s.y * s.z;
    this->x = s.x * c.y * c.z - c.x * s.y * s.z;
    this->y = c.x * s.y * c.z + s.x * c.y * s.z;
    this->z = c.x * c.y * s.z - s.x * s.y * c.z;
}

template <typename T>
inline qua<T>::qua(const mat<3, 3, T>& m)
{
    *this = quat_cast(m);
}

template <typename T>
inline qua<T>::qua(const mat<4, 4, T>& m)
{
    *this = quat_cast(m);
}

template <typename T>
inline qua<T>::qua(const type& q)
    : x(q.x)
    , y(q.y)
    , z(q.z)
    , w(q.w)
{
}

// Component access
template <typename T>
inline T& qua<T>::operator[](length_t i)
{
    switch (i) {
    default:
    case 0:
        return x;
    case 1:
        return y;
    case 2:
        return z;
    case 3:
        return w;
    }
}

template <typename T>
inline const T& qua<T>::operator[](length_t i) const
{
    switch (i) {
    default:
    case 0:
        return x;
    case 1:
        return y;
    case 2:
        return z;
    case 3:
        return w;
    }
}

// Assignment operators

template <typename T>
template <typename U>
inline qua<T>& qua<T>::operator=(const qua<U>& q)
{
    this->x = static_cast<T>(q.x);
    this->y = static_cast<T>(q.y);
    this->z = static_cast<T>(q.z);
    this->w = static_cast<T>(q.w);
    return *this;
}

template <typename T>
template <typename U>
inline qua<T>& qua<T>::operator+=(const qua<U>& q)
{
    this->x += static_cast<T>(q.x);
    this->y += static_cast<T>(q.y);
    this->z += static_cast<T>(q.z);
    this->w += static_cast<T>(q.w);
    return *this;
}

template <typename T>
template <typename U>
inline qua<T>& qua<T>::operator-=(const qua<U>& q)
{
    this->x -= static_cast<T>(q.x);
    this->y -= static_cast<T>(q.y);
    this->z -= static_cast<T>(q.z);
    this->w -= static_cast<T>(q.w);
    return *this;
}

template <typename T>
template <typename U>
inline qua<T>& qua<T>::operator*=(const qua<U>& q)
{
    return (*this = *this * q);
}

template <typename T>
template <typename U>
inline qua<T>& qua<T>::operator*=(const U& scalar)
{
    this->x *= static_cast<T>(scalar);
    this->y *= static_cast<T>(scalar);
    this->z *= static_cast<T>(scalar);
    this->w *= static_cast<T>(scalar);
    return *this;
}

template <typename T>
template <typename U>
inline qua<T>& qua<T>::operator/=(const U& scalar)
{
    this->x /= static_cast<T>(scalar);
    this->y /= static_cast<T>(scalar);
    this->z /= static_cast<T>(scalar);
    this->w /= static_cast<T>(scalar);
    return *this;
}

// Arithmetic operators

template <typename T>
inline qua<T> operator+(const qua<T>& q1, const qua<T>& q2)
{
    return qua<T>(q1.x + q2.x, q1.y + q2.y, q1.z + q2.z, q1.w + q2.w);
}

template <typename T>
inline qua<T> operator-(const qua<T>& q1, const qua<T>& q2)
{
    return qua<T>(q1.x - q2.x, q1.y - q2.y, q1.z - q2.z, q1.w - q2.w);
}

template <typename T>
inline qua<T> operator*(const qua<T>& q1, const qua<T>& q2)
{
    return qua<T>(q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
                  q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,
                  q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w,
                  q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z);
}

template <typename T>
inline qua<T> operator*(const qua<T>& q, const T& scalar)
{
    return qua<T>(q.x * scalar, q.y * scalar, q.z * scalar, q.w * scalar);
}

template <typename T>
inline qua<T> operator*(const T& scalar, const qua<T>& q)
{
    return qua<T>(q.x * scalar, q.y * scalar, q.z * scalar, q.w * scalar);
}

template <typename T>
inline qua<T> operator/(const qua<T>& q, const T& scalar)
{
    return qua<T>(q.x / scalar, q.y / scalar, q.z / scalar, q.w / scalar);
}

// Unary operators

template <typename T>
inline qua<T> operator-(const qua<T>& q)
{
    return qua<T>(-q.x, -q.y, -q.z, -q.w);
}

// Boolean operators

template <typename T>
inline bool operator==(const qua<T>& q1, const qua<T>& q2)
{
    return (q1.x == q2.x) && (q1.y == q2.y) && (q1.z == q2.z) && (q1.w == q2.w);
}

template <typename T>
inline bool operator!=(const qua<T>& q1, const qua<T>& q2)
{
    return !(q1 == q2);
}

template <typename T>
qua<T> quat_cast(const mat<3, 3, T>& m)
{
    T fourXSquaredMinus1 = m[0][0] - m[1][1] - m[2][2];
    T fourYSquaredMinus1 = m[1][1] - m[0][0] - m[2][2];
    T fourZSquaredMinus1 = m[2][2] - m[0][0] - m[1][1];
    T fourWSquaredMinus1 = m[0][0] + m[1][1] + m[2][2];

    int biggestIndex = 0;
    T fourBiggestSquaredMinus1 = fourWSquaredMinus1;
    if (fourXSquaredMinus1 > fourBiggestSquaredMinus1) {
        fourBiggestSquaredMinus1 = fourXSquaredMinus1;
        biggestIndex = 1;
    }
    if (fourYSquaredMinus1 > fourBiggestSquaredMinus1) {
        fourBiggestSquaredMinus1 = fourYSquaredMinus1;
        biggestIndex = 2;
    }
    if (fourZSquaredMinus1 > fourBiggestSquaredMinus1) {
        fourBiggestSquaredMinus1 = fourZSquaredMinus1;
        biggestIndex = 3;
    }

    T biggestVal = sqrt(fourBiggestSquaredMinus1 + static_cast<T>(1)) * static_cast<T>(0.5);
    T mult = static_cast<T>(0.25) / biggestVal;

    switch (biggestIndex) {
    case 0:
        return qua<T>((m[1][2] - m[2][1]) * mult, (m[2][0] - m[0][2]) * mult,
                      (m[0][1] - m[1][0]) * mult, biggestVal);
    case 1:
        return qua<T>(biggestVal, (m[0][1] + m[1][0]) * mult, (m[2][0] + m[0][2]) * mult,
                      (m[1][2] - m[2][1]) * mult);
    case 2:
        return qua<T>((m[0][1] + m[1][0]) * mult, biggestVal, (m[1][2] + m[2][1]) * mult,
                      (m[2][0] - m[0][2]) * mult);
    case 3:
        return qua<T>((m[2][0] + m[0][2]) * mult, (m[1][2] + m[2][1]) * mult, biggestVal,
                      (m[0][1] - m[1][0]) * mult);
    default:
        assert(false);
        return qua<T>(1, 0, 0, 0);
    }
}


template <typename T>
qua<T> quat_cast(const mat<4, 4, T>& m)
{
    return quat_cast(mat<3, 3, T>(m));
}

template <typename T>
mat<3, 3, T> mat3_cast(const qua<T>& q)
{
    mat<3, 3, T> result(T(1));
    T qxx(q.x * q.x);
    T qyy(q.y * q.y);
    T qzz(q.z * q.z);
    T qxz(q.x * q.z);
    T qxy(q.x * q.y);
    T qyz(q.y * q.z);
    T qwx(q.w * q.x);
    T qwy(q.w * q.y);
    T qwz(q.w * q.z);

    result[0][0] = T(1) - T(2) * (qyy + qzz);
    result[0][1] = T(2) * (qxy + qwz);
    result[0][2] = T(2) * (qxz - qwy);

    result[1][0] = T(2) * (qxy - qwz);
    result[1][1] = T(1) - T(2) * (qxx + qzz);
    result[1][2] = T(2) * (qyz + qwx);

    result[2][0] = T(2) * (qxz + qwy);
    result[2][1] = T(2) * (qyz - qwx);
    result[2][2] = T(1) - T(2) * (qxx + qyy);

    return result;
}

template <typename T>
mat<4, 4, T> mat4_cast(const qua<T>& q)
{
    return mat<4, 4, T>(mat3_cast(q));
}

} // namespace math
} // namespace ocf