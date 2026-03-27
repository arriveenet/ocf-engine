#include "geometric.h"
namespace ocf {
namespace math {

// Dot product implementations
template <typename T>
inline constexpr T dot(const vec<2, T>& v1, const vec<2, T>& v2)
{
    return v1.x * v2.x + v1.y * v2.y;
}

template <typename T>
inline constexpr T dot(const vec<3, T>& v1, const vec<3, T>& v2)
{
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

template <typename T>
inline constexpr T dot(const qua<T>& q1, const qua<T>& q2)
{
    return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
}

template <typename T>
T clamp(T value, T minVal, T maxVal)
{
    if (value < minVal)
        return minVal;
    if (value > maxVal)
        return maxVal;
    return value;
}


// Length implementations
template <typename T>
inline T length(const vec<2, T>& v)
{
    return static_cast<T>(std::sqrt(dot(v, v)));
}

template <typename T>
inline T length(const vec<3, T>& v)
{
    return static_cast<T>(std::sqrt(dot(v, v)));
}

template <typename T>
inline T length(const vec<4, T>& v)
{
    return static_cast<T>(std::sqrt(dot(v, v)));
}

template <typename T>
inline T length(const qua<T>& q)
{
    return static_cast<T>(std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w));
}

// Normalize implementations
template <typename T>
inline vec<2, T> normalize(const vec<2, T>& v)
{
    T len = length(v);
    if (len > static_cast<T>(0))
        return v / len;
    return vec<2, T>(static_cast<T>(0), static_cast<T>(0));
}

template <typename T>
inline vec<3, T> normalize(const vec<3, T>& v)
{
    T len = length(v);
    if (len > static_cast<T>(0))
        return v / len;
    return vec<3, T>(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0));
}

template <typename T>
inline vec<4, T> normalize(const vec<4, T>& v)
{
    T len = length(v);
    if (len > static_cast<T>(0))
        return v / len;
    return vec<4, T>(static_cast<T>(0));
}

template <typename T>
inline qua<T> normalize(const qua<T>& q)
{
    T len = length(q);
    if (len > static_cast<T>(0))
        return q / len;
    return qua<T>(T(0), T(0), T(0), T(1));
}

// Cross product implementation
template <typename T>
inline vec<3, T> cross(const vec<3, T>& v1, const vec<3, T>& v2)
{
    return vec<3, T>(
        v1.y * v2.z - v1.z * v2.y,
        v1.z * v2.x - v1.x * v2.z,
        v1.x * v2.y - v1.y * v2.x
    );
}

template <typename T>
inline qua<T> conjugate(const qua<T>& q)
{
    return qua<T>(-q.x, -q.y, -q.z, q.w);
}

template <typename T>
inline qua<T> inverse(const qua<T>& q)
{
    T len = length(q);
    if (len > static_cast<T>(0)) {
        T oneOverLength = static_cast<T>(1) / len;
        return conjugate(q) * (oneOverLength * oneOverLength);
    }
    return qua<T>(T(0), T(0), T(0), T(1));
}

template <typename T>
inline qua<T> slerp(const qua<T>& q1, const qua<T>& q2, T t)
{
    qua<T> z = q2;

    T cosTheta = dot(q1, q2);

    // If cosTheta < 0, the interpolation will take the long way around the sphere.
    // To fix this, one quat must be negated.
    if (cosTheta < static_cast<T>(0)) {
        z = -q2;
        cosTheta = -cosTheta;
    }

    // Perform a linear interpolation when cosTheta is close to 1 to avoid side effect of sin(angle)
    // becoming a zero denominator
    if (cosTheta > static_cast<T>(1) - static_cast<T>(0.001)) {
        // Linear interpolation
        return qua<T>(
            q1.x * (static_cast<T>(1) - t) + z.x * t, q1.y * (static_cast<T>(1) - t) + z.y * t,
            q1.z * (static_cast<T>(1) - t) + z.z * t, q1.w * (static_cast<T>(1) - t) + z.w * t);
    }
    else {
        // Essential Mathematics, page 467
        T angle = static_cast<T>(std::acos(cosTheta));
        return (q1 * static_cast<T>(std::sin((static_cast<T>(1) - t) * angle)) +
                z * static_cast<T>(std::sin(t * angle))) /
               static_cast<T>(std::sin(angle));
    }
}

// Rotation operations

template <typename T>
inline vec<3, T> rotate(const qua<T>& q, const vec<3, T>& v)
{
    vec<3, T> uv = vec<3, T>(q.y * v.z - q.z * v.y, q.z * v.x - q.x * v.z, q.x * v.y - q.y * v.x);
    vec<3, T> uuv =
        vec<3, T>(q.y * uv.z - q.z * uv.y, q.z * uv.x - q.x * uv.z, q.x * uv.y - q.y * uv.x);

    return v + ((uv * q.w) + uuv) * static_cast<T>(2);
}

template <typename T>
inline qua<T> angleAxis(T angle, const vec<3, T>& axis)
{
    T halfAngle = angle * static_cast<T>(0.5);
    T s = static_cast<T>(std::sin(halfAngle));
    vec<3, T> normalizedAxis = normalize(axis);
    return qua<T>(normalizedAxis.x * s, normalizedAxis.y * s, normalizedAxis.z * s,
                  static_cast<T>(std::cos(halfAngle)));
}

template <typename genType>
inline constexpr genType radians(genType degrees)
{
    return degrees * static_cast<genType>(0.01745329251994329576923690768489);
}

template <typename genType>
inline constexpr genType degrees(genType radians)
{
    return radians * static_cast<genType>(57.295779513082320876798154814105);
}


} // namespace math
} // namespace ocf
