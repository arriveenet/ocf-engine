// SPDX-License-Identifier: MIT

#pragma once
#include "ocf/platform/Window.h"
#include "ocf/math/vec2.h"

#include <GLFW/glfw3.h>

namespace ocf {

class EventDispatcher;

class OCF_API GLFWWindow : public Window {
    friend class GLFWEventHandler;

public:
    explicit GLFWWindow(EventDispatcher& eventDispatcher);
    ~GLFWWindow() override;

    bool create(const Application::Config &config, std::string_view title,
                int width, int height) override;

    void pollEvents() override;

    bool windowShouldClose() const override;

    Platform getPlatform() const override;

    NativeHandle getNativeHandle() const override;

private:
    void onGLFWMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    void onGLFWMouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
    void onGLFWScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    void onGLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void onGLFWWindowSizeCallback(GLFWwindow* window, int width, int height);

private:
    GLFWwindow* m_pMainWindow = nullptr;
    math::vec2 m_mousePosition = {0.0f, 0.0f};
    math::vec2 m_lastMousePosition = {0.0f, 0.0f};
};

} // namespace ocf
