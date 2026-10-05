// SPDX-License-Identifier: MIT
#pragma once
#include "ocf/math/mat4.h"
#include "ocf/math/vec3.h"
#include "ocf/scene/Component.h"

namespace ocf {

class Camera : public Component {
public:
    enum class Type {
        Perspective,
        Orthographic,
    };

    Camera();
    virtual ~Camera();

    void perspective(float fovy, float aspect, float near, float far);
    void orthographic(float left, float right, float bottom, float top, float near = -1.0f,
                      float far = 1.0f);

    Type getType() const noexcept { return m_type; }

    const math::mat4& getProjection() const noexcept { return m_projection; }

    const math::mat4& getView() const noexcept { return m_view; }

    const math::vec3& getPosition() const noexcept { return m_position; }

    void lookAt(const math::vec3&, const math::vec3& center,
                const math::vec3& up = math::vec3(0.0f, 1.0f, 0.0f));

private:
    Type m_type;
    math::mat4 m_projection;
    math::mat4 m_view;
    math::vec3 m_position;
};

} // namespace ocf
