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
                void* window;
            } x11;
            struct {
                void* display;
                void* surface;
            } wayland;
        };
    };

    static const char* platformToString(Platform platform);

    Window();
    virtual ~Window() = default;

    void setTitle(std::string_view title);

    std::string getTitle() const;

    virtual Platform getPlatform() const;

    virtual bool create(const Application::Config& config, std::string_view title, int width, int height) = 0;

    /** Poll for and process events. */
    virtual void pollEvents() = 0;

    /** Check if the window should close. */
    virtual bool windowShouldClose() const = 0;

    /** Get native handle */
    virtual NativeHandle getNativeHandle() const = 0;

protected:
    std::string m_title;
    int m_width;
    int m_height;
};

} // namespace ocf
