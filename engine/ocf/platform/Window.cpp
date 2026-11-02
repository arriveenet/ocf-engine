// SPDX-License-Identifier: MIT

#include "ocf/platform/Window.h"

namespace ocf {

const char* Window::platformToString(Window::Platform platform)
{
    switch (platform) {
    case Platform::Win32:       return "Win32";
    case Platform::Cocoa:       return "Cocoa";
    case Platform::Wayland:     return "Wayland";
    case Platform::X11:         return "X11";
    default:                    return "Unknown";
    }
}

Window::Window(EventDispatcher& eventDispatcher)
    : m_title("")
    , m_windowWidth(0)
    , m_windowHeight(0)
    , m_framebufferWidth(0)
    , m_framebufferHeight(0)
    , m_eventDispatcher(eventDispatcher)
{
}

void Window::setTitle(std::string_view title)
{
    m_title = title;
}

std::string Window::getTitle() const
{
    return m_title;
}

Window::Platform Window::getPlatform() const noexcept
{
    return Platform::Unknown;
}

} // namespace ocf
