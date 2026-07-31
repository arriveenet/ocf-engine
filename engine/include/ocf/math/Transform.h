// SPDX-License-Identifier: MIT
#pragma once
#include "ocf/math/mat4.h"
#include "ocf/math/vec3.h"

namespace ocf {
namespace math {

class Transform {
public:
    Transform();
    ~Transform() = default;

    void setPosition(const vec3& position);

    void setRotation(const vec3& rotation);

    void setScale(const vec3& scale);

    const vec3& getPosition() const noexcept { return m_position; }
    const vec3& getRotation() const noexcept { return m_rotation; }
    const vec3& getScale() const noexcept { return m_scale; }

    const mat4& getLocalMatrix() const;
    const mat4& getWorldMatrix() const;

    bool updateWorldMatrix(const mat4& parentWorldMatrix, bool parentDirty = false);

private:
    mutable mat4 m_localMatrix;
    mutable mat4 m_worldMatrix;
    mutable bool m_isLocalMatrixDirty = true;
    mutable bool m_isWorldMatrixDirty = true;
    vec3 m_position;
    vec3 m_rotation;
    vec3 m_scale;
};

} // namespace math
} // namespace ocf
