#pragma once
#include "ocf/Component.h"
#include "ocf/Types.h"

namespace ocf {

class CameraComponent : public Component {
public:
    CameraComponent();
    ~CameraComponent() override;

    void update(float deltaTime) override;

    CameraFlag getCameraFlag() const noexcept { return m_cameraFlag; }
    void setCameraFlag(CameraFlag flag) { m_cameraFlag = flag; }

    bool isActive() const noexcept { return m_active; }
    void setActive(bool active) { m_active = active; }

    int getDepth() const noexcept { return m_depth; }
    void setDepth(int depth) { m_depth = depth; }

private:
    CameraFlag m_cameraFlag = CameraFlag::Default;
    bool m_active = true;
    int m_depth = 0;
};

} // namespace ocf
