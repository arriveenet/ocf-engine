/* SPDX - License - Identifier : MIT */

#include "GLFWWindow.h"

#include "ocf/platform/platform.h"

#if OCF_TARGET_PLATFORM == OCF_PLATFORM_WIN32
#    define GLFW_EXPOSE_NATIVE_WIN32
#    include <GLFW/glfw3native.h>
#endif


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
    if (!glfwInit()) {
        return false;
    }

    m_pMainWindow = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
    if (!m_pMainWindow) {
        glfwTerminate();
        return false;
    }

    glfwSwapInterval(config.vsync ? 1 : 0);

    return true;
}

void GLFWWindow::swapBuffers()
{
    glfwSwapBuffers(m_pMainWindow);
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

void* GLFWWindow::getNativeHandle() const
{
#if OCF_TARGET_PLATFORM == OCF_PLATFORM_WIN32
    return glfwGetWin32Window(m_pMainWindow);
#else
    return nullptr;
#endif
}

} // namespace ocf
