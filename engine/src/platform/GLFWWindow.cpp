// SPDX-License-Identifier: MIT

#include "GLFWWindow.h"

#include "ocf/core/Logger.h"
#include "ocf/platform/platform.h"


#if OCF_TARGET_PLATFORM == OCF_PLATFORM_WIN32
#   define GLFW_EXPOSE_NATIVE_WIN32
#   include <GLFW/glfw3native.h>
#elif OCF_TARGET_PLATFORM == OCF_PLATFORM_LINUX
#   define GLFW_EXPOSE_NATIVE_X11
#   define GLFW_EXPOSE_NATIVE_WAYLAND
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace ocf {

GLFWWindow::GLFWWindow()
{

}

GLFWWindow::~GLFWWindow()
{
    if (m_pMainWindow) {
        glfwDestroyWindow(m_pMainWindow);
    }
    glfwTerminate();
}

bool GLFWWindow::create(const Application::Config& config, std::string_view title,
                        int width, int height)
{
    glfwSetErrorCallback([](int error, const char* description) {
        OCF_LOG_ERROR("GLFW Error ({}): {}", error, description);
    });

    if (!glfwInit()) {
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_pMainWindow = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
    if (!m_pMainWindow) {
        glfwTerminate();
        return false;
    }

    glfwSwapInterval(config.vsync ? 1 : 0);

    return true;
}

void GLFWWindow::pollEvents()
{
    glfwPollEvents();
}

bool GLFWWindow::windowShouldClose() const
{
    return glfwWindowShouldClose(m_pMainWindow);
}

Window::Platform GLFWWindow::getPlatform() const
{
    const int platform = glfwGetPlatform();

    switch (platform) {
    case GLFW_PLATFORM_WIN32:       return Platform::Win32;
    case GLFW_PLATFORM_COCOA:       return Platform::Cocoa;
    case GLFW_PLATFORM_WAYLAND:     return Platform::Wayland;
    case GLFW_PLATFORM_X11:         return Platform::X11;
    default:                        return Platform::Unknown;
    }
}

Window::NativeHandle GLFWWindow::getNativeHandle() const
{
    NativeHandle handle{};
    handle.platform = getPlatform();
#if OCF_TARGET_PLATFORM == OCF_PLATFORM_WIN32
    handle.win32.hInstance = GetModuleHandle(nullptr);
    handle.win32.hWnd = glfwGetWin32Window(m_pMainWindow);
#elif OCF_TARGET_PLATFORM == OCF_PLATFORM_LINUX
    if (handle.platform == Platform::Wayland) {
        handle.wayland.display = glfwGetWaylandDisplay();
        handle.wayland.surface = glfwGetWaylandWindow(m_pMainWindow);
    }
    else if (handle.platform == Platform::X11) {
        handle.x11.display = glfwGetX11Display();
        handle.x11.window = glfwGetX11Window(m_pMainWindow);
    }
#else
    handle.platform = Platform::Unknown;
    handle.handle = nullptr;
#endif

    return handle;
}

} // namespace ocf
