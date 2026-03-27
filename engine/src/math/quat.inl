#include <cmath>

namespace ocf {
namespace math {

// Constructors

template <typename T>
inline qua<T>::qua(T s) : x(T(0)), y(T(0)), z(T(0)), w(s)
{
}

template <typename T>
inline qua<T>::qua(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w)
{
}

template <typename T>
inline qua<T>::qua(const vec<3, T>& axis, T angle)
{
    T halfAngle = angle * static_cast<T>(0.5);
    T s = std::sin(halfAngle);
    x = axis.x * s;
    y = axis.y * s;
    z = axis.z * s;
    w = std::cos(halfAngle);
}

template <typename T>
inline qua<T>::qua(const type& q) : x(q.x), y(q.y), z(q.z), w(q.w)
{
}

// Component access

template <typename T>
inline T& qua<T>::operator[](length_t i)
{
    switch(i) {
        default:
        case 0: return x;
        case 1: return y;
        case 2: return z;
        case 3: return w;
    }
}

template <typename T>
inline const T& qua<T>::operator[](length_t i) const
{
    switch(i) {
        default:
        case 0: return x;
        case 1: return y;
        case 2: return z;
        case 3: return w;
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
    return qua<T>(
        q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
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

} // namespace math
} // namespace ocf