#pragma once
#include "ocf/math/mat4.h"
#include "ocf/math/vec3.h"
#include "ocf/math/geometric.h"
#include <cmath>

namespace ocf {
namespace math {

// Perspective projection matrix
template <typename T>
mat<4, 4, T> perspective(T fovy, T aspect, T zNear, T zFar);

// Orthographic projection matrix  
template <typename T>
mat<4, 4, T> ortho(T left, T right, T bottom, T top, T zNear, T zFar);

// Translation matrix
template <typename T>
mat<4, 4, T> translate(const mat<4, 4, T>& m, const vec<3, T>& v);

template <typename T>
mat<4, 4, T> translate(const vec<3, T>& v);

// Rotation matrices
template <typename T>
mat<4, 4, T> rotate(const mat<4, 4, T>& m, T angle, const vec<3, T>& axis);

template <typename T>
mat<4, 4, T> rotate(T angle, const vec<3, T>& axis);

template <typename T>
mat<4, 4, T> rotateX(T angle);

template <typename T>
mat<4, 4, T> rotateY(T angle);

template <typename T>
mat<4, 4, T> rotateZ(T angle);

// Scale matrix
template <typename T>
mat<4, 4, T> scale(const mat<4, 4, T>& m, const vec<3, T>& v);

template <typename T>
mat<4, 4, T> scale(const vec<3, T>& v);

// Look-at view matrix
template <typename T>
mat<4, 4, T> lookAt(const vec<3, T>& eye, const vec<3, T>& center, const vec<3, T>& up);

template <typename T, typename U>
vec<3, T> unProject(const vec<3, T>& win, const mat<4, 4, T>& model, const mat<4, 4, T>& proj,
                    const vec<4, U>& viewport);

} // namespace math
} // namespace ocf

#include "ocf/math/matrix_transform.inl"