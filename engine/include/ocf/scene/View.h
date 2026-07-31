// SPDX-License-Identifier: MIT
#pragma once

namespace ocf {

class Camera;
class Scene;

class View {
public:
    View();
    virtual ~View();

    void setScene(Scene* scene);

    Scene* getScene() const { return m_scene; }

    void setCamera(Camera* camera);

    Camera* getCamera() const { return m_camera; }

    bool hasCamera() const { return m_camera != nullptr; }

protected:
    Scene* m_scene = nullptr;
    Camera* m_camera = nullptr;
};

} // namespace ocf
