#include "matrix_transform.h"
namespace ocf {
namespace math {

// Perspective projection matrix
template <typename T>
mat<4, 4, T> perspective(T fovy, T aspect, T zNear, T zFar)
{
    T const tanHalfFovy = std::tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0].x = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1].y = static_cast<T>(1) / (tanHalfFovy);
    Result[2].z = -(zFar + zNear) / (zFar - zNear);
    Result[2].w = -static_cast<T>(1);
    Result[3].z = -(static_cast<T>(2) * zFar * zNear) / (zFar - zNear);
    
    return Result;
}

// Orthographic projection matrix  
template <typename T>
mat<4, 4, T> ortho(T left, T right, T bottom, T top, T zNear, T zFar)
{
    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0].x = static_cast<T>(2) / (right - left);
    Result[1].y = static_cast<T>(2) / (top - bottom);
    Result[2].z = -static_cast<T>(2) / (zFar - zNear);
    Result[3].x = -(right + left) / (right - left);
    Result[3].y = -(top + bottom) / (top - bottom);
    Result[3].z = -(zFar + zNear) / (zFar - zNear);
    
    return Result;
}

// Translation matrix
template <typename T>
mat<4, 4, T> translate(const mat<4, 4, T>& m, const vec<3, T>& v)
{
    mat<4, 4, T> Result(m);
    Result[3] = m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3];
    
    return Result;
}

template <typename T>
mat<4, 4, T> translate(const vec<3, T>& v)
{
    mat<4, 4, T> Result(static_cast<T>(1));
    Result[3].x = v.x;
    Result[3].y = v.y;
    Result[3].z = v.z;
    
    return Result;
}

// Rotation matrices
template <typename T>
mat<4, 4, T> rotate(const mat<4, 4, T>& m, T angle, const vec<3, T>& axis)
{
    T const a = angle;
    T const c = std::cos(a);
    T const s = std::sin(a);

    vec<3, T> normalizedAxis = normalize(axis);
    vec<3, T> temp = normalizedAxis * (static_cast<T>(1) - c);

    mat<4, 4, T> Rotate(static_cast<T>(0));
    Rotate[0].x = c + temp.x * normalizedAxis.x;
    Rotate[0].y = temp.x * normalizedAxis.y + s * normalizedAxis.z;
    Rotate[0].z = temp.x * normalizedAxis.z - s * normalizedAxis.y;

    Rotate[1].x = temp.y * normalizedAxis.x - s * normalizedAxis.z;
    Rotate[1].y = c + temp.y * normalizedAxis.y;
    Rotate[1].z = temp.y * normalizedAxis.z + s * normalizedAxis.x;

    Rotate[2].x = temp.z * normalizedAxis.x + s * normalizedAxis.y;
    Rotate[2].y = temp.z * normalizedAxis.y - s * normalizedAxis.x;
    Rotate[2].z = c + temp.z * normalizedAxis.z;

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0] = m[0] * Rotate[0].x + m[1] * Rotate[0].y + m[2] * Rotate[0].z;
    Result[1] = m[0] * Rotate[1].x + m[1] * Rotate[1].y + m[2] * Rotate[1].z;
    Result[2] = m[0] * Rotate[2].x + m[1] * Rotate[2].y + m[2] * Rotate[2].z;
    Result[3] = m[3];
    
    return Result;
}

template <typename T>
mat<4, 4, T> rotate(T angle, const vec<3, T>& axis)
{
    return rotate(mat<4, 4, T>(static_cast<T>(1)), angle, axis);
}

template <typename T>
mat<4, 4, T> rotateX(T angle)
{
    T const c = std::cos(angle);
    T const s = std::sin(angle);

    mat<4, 4, T> Result(static_cast<T>(1));
    Result[1].y = c;
    Result[1].z = s;
    Result[2].y = -s;
    Result[2].z = c;
    
    return Result;
}

template <typename T>
mat<4, 4, T> rotateY(T angle)
{
    T const c = std::cos(angle);
    T const s = std::sin(angle);

    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0].x = c;
    Result[0].z = -s;
    Result[2].x = s;
    Result[2].z = c;
    
    return Result;
}

template <typename T>
mat<4, 4, T> rotateZ(T angle)
{
    T const c = std::cos(angle);
    T const s = std::sin(angle);

    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0].x = c;
    Result[0].y = s;
    Result[1].x = -s;
    Result[1].y = c;
    
    return Result;
}

// Scale matrix
template <typename T>
mat<4, 4, T> scale(const mat<4, 4, T>& m, const vec<3, T>& v)
{
    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0] = m[0] * v.x;
    Result[1] = m[1] * v.y;
    Result[2] = m[2] * v.z;
    Result[3] = m[3];
    
    return Result;
}

template <typename T>
mat<4, 4, T> scale(const vec<3, T>& v)
{
    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0].x = v.x;
    Result[1].y = v.y;
    Result[2].z = v.z;
    
    return Result;
}

// Look-at view matrix
template <typename T>
mat<4, 4, T> lookAt(const vec<3, T>& eye, const vec<3, T>& center, const vec<3, T>& up)
{
    vec<3, T> const f = normalize(center - eye);
    vec<3, T> const s = normalize(cross(f, up));
    vec<3, T> const u = cross(s, f);

    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0].x = s.x;
    Result[1].x = s.y;
    Result[2].x = s.z;
    Result[0].y = u.x;
    Result[1].y = u.y;
    Result[2].y = u.z;
    Result[0].z = -f.x;
    Result[1].z = -f.y;
    Result[2].z = -f.z;
    Result[3].x = -dot(s, eye);
    Result[3].y = -dot(u, eye);
    Result[3].z = dot(f, eye);
    
    return Result;
}

template <typename T, typename U>
vec<3, T> unProject(const vec<3, T>& win, const mat<4, 4, T>& model, const mat<4, 4, T>& proj,
                    const vec<4, U>& viewport)
{
    mat<4, 4, T> Inverse = inverse(proj * model);

    vec<4, T> tmp = vec<4, T>(win, T(1));
    tmp.x = (tmp.x - T(viewport[0])) / T(viewport[2]);
    tmp.y = (tmp.y - T(viewport[1])) / T(viewport[3]);
    tmp = tmp * static_cast<T>(2) - static_cast<T>(1);

    vec<4, T> obj = Inverse * tmp;
    obj /= obj.w;

    return vec<3, T>(obj);
}

} // namespace math
} // namespace ocf