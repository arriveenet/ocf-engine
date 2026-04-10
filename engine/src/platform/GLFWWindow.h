// SPDX-License-Identifier: MIT

#pragma once
#include "ocf/platform/Window.h"
#include <GLFW/glfw3.h>

namespace ocf {

class OCF_API GLFWWindow : public Window {
public:
    GLFWWindow();
    ~GLFWWindow() override;

    bool create(const Application::Config &config, std::string_view title,
                int width, int height) override;

    void pollEvents() override;

    bool windowShouldClose() const override;

    Platform getPlatform() const override;

    NativeHandle getNativeHandle() const override;

private:
    GLFWwindow* m_pMainWindow = nullptr;
};

} // namespace ocf
