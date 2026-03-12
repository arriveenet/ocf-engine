#pragma once
#include "ocf/Component.h"
#include "ocf/Types.h"
#include <glm/glm.hpp>

namespace ocf {

class Camera : public Component {
public:
    Camera();
    ~Camera() override;

    CameraFlag getCameraFlag() const noexcept { return m_cameraFlag; }
    void setCameraFlag(CameraFlag flag) { m_cameraFlag = flag; }

    bool isActive() const noexcept { return m_active; }
    void setActive(bool active) { m_active = active; }

    int getDepth() const noexcept { return m_depth; }
    void setDepth(int depth) { m_depth = depth; }

    const glm::mat4& getProjectionMatrix() const noexcept { return m_projectionMatrix; }

    void setProjectionMatrix(const glm::mat4& projectionMatrix)
    {
        m_projectionMatrix = projectionMatrix;
    }

private:
    CameraFlag m_cameraFlag = CameraFlag::Default;
    bool m_active = true;
    int m_depth = 0;
    glm::mat4 m_projectionMatrix{1.0f};
    glm::mat4 m_viewMatrix{1.0f};
    glm::vec3 m_position{0.0f, 0.0f, 0.0f};
    glm::vec3 m_target{0.0f, 0.0f, -1.0f};
    glm::vec3 m_up{0.0f, 1.0f, 0.0f};
};

} // namespace ocf
