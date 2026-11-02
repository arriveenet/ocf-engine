// SPDX-License-Identifier: MIT

#pragma once
#include "ocf/core/compiler.h"
#include "ocf/platform/Application.h"
#include <string>

namespace ocf {

class EventDispatcher;

class OCF_API Window {
public:
    enum class Platform {
        Win32,
        Cocoa,
        Wayland,
        X11,
        Unknown,
    };

    struct NativeHandle {
        Platform platform;
        union {
            void* handle; // Generic handle for unknown platforms
            struct {
                void* hInstance;
                void* hWnd;
            } win32;
            struct {
                void* nsWindow;
            } cocoa;
            struct {
                void* display;
                unsigned long window;
            } x11;
            struct {
                void* display;
                void* surface;
            } wayland;
        };
    };

    static const char* platformToString(Platform platform);

    explicit Window(EventDispatcher& eventDispatcher);
    virtual ~Window() = default;

    void setTitle(std::string_view title);

    std::string getTitle() const;

    int getWindowWidth() const noexcept { return m_windowWidth; }

    int getWindowHeight() const noexcept { return m_windowHeight; }

    int getFramebufferWidth() const noexcept { return m_framebufferWidth; }

    int getFramebufferHeight() const noexcept { return m_framebufferHeight; }

    virtual Platform getPlatform() const noexcept;

    virtual bool create(const Application::Config& config, std::string_view title,
                        int width, int height) = 0;

    /** Poll for and process events. */
    virtual void pollEvents() = 0;

    /** Check if the window should close. */
    virtual bool windowShouldClose() const = 0;

    /** Get native handle */
    virtual NativeHandle getNativeHandle() const = 0;

protected:
    std::string m_title;
    int m_windowWidth;
    int m_windowHeight;
    int m_framebufferWidth;
    int m_framebufferHeight;
    EventDispatcher& m_eventDispatcher;
};

} // namespace ocf
