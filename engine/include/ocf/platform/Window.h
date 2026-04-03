// SPDX-License-Identifier: MIT

#pragma once
#include "ocf/platform/Application.h"
#include "ocf/platform/platform.h"
#include <string>

namespace ocf {

class OCF_API Window {
public:
    enum class Platform {
        Win32,
        Cocoa,
        Wayland,
        X11,
        Unknown,
    };

    Window();
    virtual ~Window() = default;

    void setTitle(std::string_view title);

    std::string getTitle() const;

    virtual bool create(const Application::Config& config, std::string_view title, int width, int height) = 0;

    /** Swap the front and back buffers. */
    virtual void swapBuffers() = 0;

    /** Poll for and process events. */
    virtual void pollEvents() = 0;

    /** Check if the window should close. */
    virtual bool windowShouldClose() const = 0;

    virtual Platform getPlatform() const;

    virtual void* getNativeHandle() const = 0;

protected:
    std::string m_title;
    int m_width;
    int m_height;
};

} // namespace ocf
