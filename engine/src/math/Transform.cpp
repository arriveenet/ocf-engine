// SPDX-License-Identifier: MIT
#include "ocf/math/Transform.h"

#include "ocf/math/matrix_transform.h"

namespace ocf {
namespace math {

Transform::Transform()
    : m_localMatrix(mat4(1.0f))
    , m_worldMatrix(mat4(1.0f))
    , m_position(vec3(0.0f, 0.0f, 0.0f))
    , m_rotation(vec3(0.0f, 0.0f, 0.0f))
    , m_scale(vec3(1.0f, 1.0f, 1.0f))
{
}

void Transform::setPosition(const vec3& position)
{
    m_position = position;

    m_isLocalMatrixDirty = true;
    m_isWorldMatrixDirty = true;
}

void Transform::setRotation(const vec3& rotation)
{
    m_rotation = rotation;

    m_isLocalMatrixDirty = true;
    m_isWorldMatrixDirty = true;
}

void Transform::setScale(const vec3& scale)
{
    m_scale = scale;

    m_isLocalMatrixDirty = true;
    m_isWorldMatrixDirty = true;
}

const mat4& Transform::getLocalMatrix() const
{
    if (m_isLocalMatrixDirty) {
        m_localMatrix = mat4(1.0f);
        m_localMatrix = translate(m_localMatrix, m_position);
        m_localMatrix = rotate(m_localMatrix, radians(m_rotation.x), vec3(1.0f, 0.0f, 0.0f));
        m_localMatrix = rotate(m_localMatrix, radians(m_rotation.y), vec3(0.0f, 1.0f, 0.0f));
        m_localMatrix = rotate(m_localMatrix, radians(m_rotation.z), vec3(0.0f, 0.0f, 1.0f));
        m_localMatrix = scale(m_localMatrix, m_scale);
        m_isLocalMatrixDirty = false;
    }
    return m_localMatrix;
}

const mat4& Transform::getWorldMatrix() const
{
    return m_worldMatrix;
}

bool Transform::updateWorldMatrix(const mat4& parentWorldMatrix, bool parentDirty)
{
    if (m_isWorldMatrixDirty || parentDirty) {
        m_worldMatrix = parentWorldMatrix * getLocalMatrix();
        m_isWorldMatrixDirty = false;
        return true;
    }

    return false;
}

} // namespace math
} // namespace ocf
