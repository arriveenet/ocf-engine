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

void* GLFWWindow::getNativeHandle() const
{
#if OCF_TARGET_PLATFORM == OCF_PLATFORM_WIN32
    return glfwGetWin32Window(m_pMainWindow);
#else
    return nullptr;
#endif
}

} // namespace ocf
