// SPDX-License-Identifier: MIT
#include "ocf/scene/Camera.h"

#include "ocf/math/matrix_transform.h"

namespace ocf {

Camera::Camera()
    : m_type(Camera::Type::Perspective)
    , m_projection(1.0f)
    , m_view(1.0f)
{
}

Camera::~Camera()
{
}

void Camera::perspective(float fovy, float aspect, float near, float far)
{
    m_type = Camera::Type::Perspective;
    m_projection = math::perspective(fovy, aspect, near, far);
}

void Camera::orthographic(float left, float right, float bottom, float top, float near, float far)
{
    m_type = Camera::Type::Orthographic;
    m_projection = math::ortho(left, right, bottom, top, near, far);
}

void Camera::lookAt(const math::vec3& eye, const math::vec3& center, const math::vec3& up)
{
    m_position = eye;
    m_view = math::lookAt(eye, center, up);
}

} // namespace ocf
