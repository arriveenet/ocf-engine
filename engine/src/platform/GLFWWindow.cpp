// SPDX-License-Identifier: MIT

#include "GLFWWindow.h"

#include "ocf/core/event/EventDispatcher.h"
#include "ocf/core/event/Events.h"
#include "ocf/core/input/KeyCode.h"
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

#include <unordered_map>

namespace ocf {

class GLFWEventHandler {
public:
    static void onGLFWMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        GLFWWindow* glfwWindow = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        if (glfwWindow != nullptr) {
            glfwWindow->onGLFWMouseButtonCallback(window, button, action, mods);
        }
    }

    static void onGLFWMouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
    {
        GLFWWindow* glfwWindow = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        if (glfwWindow != nullptr) {
            glfwWindow->onGLFWMouseMoveCallback(window, xpos, ypos);
        }
    }

    static void onGLFWScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
    {
        GLFWWindow* glfwWindow = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        if (glfwWindow != nullptr) {
            glfwWindow->onGLFWScrollCallback(window, xoffset, yoffset);
        }
    }

    static void onGLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        GLFWWindow* glfwWindow = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        if (glfwWindow != nullptr) {
            glfwWindow->onGLFWKeyCallback(window, key, scancode, action, mods);
        }
    }

    static void onGLFWWindowSizeCallback(GLFWwindow* window, int width, int height)
    {
        GLFWWindow* glfwWindow = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        if (glfwWindow != nullptr) {
            glfwWindow->onGLFWWindowSizeCallback(window, width, height);
        }
    }
};

static const std::unordered_map<int, KeyCode> g_keyCodeMap = {
    /* The unknown key */
    {GLFW_KEY_UNKNOWN, KeyCode::Unknown},

    /* Printable keys */
    {GLFW_KEY_SPACE, KeyCode::Space},
    {GLFW_KEY_APOSTROPHE, KeyCode::Apostrophe},
    {GLFW_KEY_COMMA, KeyCode::Comma},
    {GLFW_KEY_MINUS, KeyCode::Minus},
    {GLFW_KEY_PERIOD, KeyCode::Period},
    {GLFW_KEY_SLASH, KeyCode::Slash},
    {GLFW_KEY_0, KeyCode::Alpha0},
    {GLFW_KEY_1, KeyCode::Alpha1},
    {GLFW_KEY_2, KeyCode::Alpha2},
    {GLFW_KEY_3, KeyCode::Alpha3},
    {GLFW_KEY_4, KeyCode::Alpha4},
    {GLFW_KEY_5, KeyCode::Alpha5},
    {GLFW_KEY_6, KeyCode::Alpha6},
    {GLFW_KEY_7, KeyCode::Alpha7},
    {GLFW_KEY_8, KeyCode::Alpha8},
    {GLFW_KEY_9, KeyCode::Alpha9},
    {GLFW_KEY_SEMICOLON, KeyCode::Semicolon},
    {GLFW_KEY_EQUAL, KeyCode::Equal},
    {GLFW_KEY_A, KeyCode::A},
    {GLFW_KEY_B, KeyCode::B},
    {GLFW_KEY_C, KeyCode::C},
    {GLFW_KEY_D, KeyCode::D},
    {GLFW_KEY_E, KeyCode::E},
    {GLFW_KEY_F, KeyCode::F},
    {GLFW_KEY_G, KeyCode::G},
    {GLFW_KEY_H, KeyCode::H},
    {GLFW_KEY_I, KeyCode::I},
    {GLFW_KEY_J, KeyCode::J},
    {GLFW_KEY_K, KeyCode::K},
    {GLFW_KEY_L, KeyCode::L},
    {GLFW_KEY_M, KeyCode::M},
    {GLFW_KEY_N, KeyCode::N},
    {GLFW_KEY_O, KeyCode::O},
    {GLFW_KEY_P, KeyCode::P},
    {GLFW_KEY_Q, KeyCode::Q},
    {GLFW_KEY_R, KeyCode::R},
    {GLFW_KEY_S, KeyCode::S},
    {GLFW_KEY_T, KeyCode::T},
    {GLFW_KEY_U, KeyCode::U},
    {GLFW_KEY_V, KeyCode::V},
    {GLFW_KEY_W, KeyCode::W},
    {GLFW_KEY_X, KeyCode::X},
    {GLFW_KEY_Y, KeyCode::Y},
    {GLFW_KEY_Z, KeyCode::Z},
    {GLFW_KEY_LEFT_BRACKET, KeyCode::LeftBracket},
    {GLFW_KEY_BACKSLASH, KeyCode::Backslash},
    {GLFW_KEY_RIGHT_BRACKET, KeyCode::RightBracket},
    {GLFW_KEY_GRAVE_ACCENT, KeyCode::GraveAccent},
    {GLFW_KEY_WORLD_1, KeyCode::World1},
    {GLFW_KEY_WORLD_2, KeyCode::World2},

    /* Function keys */
    {GLFW_KEY_ESCAPE, KeyCode::Escape},
    {GLFW_KEY_ENTER, KeyCode::Enter},
    {GLFW_KEY_TAB, KeyCode::Tab},
    {GLFW_KEY_BACKSPACE, KeyCode::Backspace},
    {GLFW_KEY_INSERT, KeyCode::Insert},
    {GLFW_KEY_DELETE, KeyCode::Delete},
    {GLFW_KEY_RIGHT, KeyCode::Right},
    {GLFW_KEY_LEFT, KeyCode::Left},
    {GLFW_KEY_DOWN, KeyCode::Down},
    {GLFW_KEY_UP, KeyCode::Up},
    {GLFW_KEY_PAGE_UP, KeyCode::PageUp},
    {GLFW_KEY_PAGE_DOWN, KeyCode::PageDown},
    {GLFW_KEY_HOME, KeyCode::Home},
    {GLFW_KEY_END, KeyCode::End},
    {GLFW_KEY_CAPS_LOCK, KeyCode::CapsLock},
    {GLFW_KEY_SCROLL_LOCK, KeyCode::ScrollLock},
    {GLFW_KEY_NUM_LOCK, KeyCode::NumLock},
    {GLFW_KEY_PRINT_SCREEN, KeyCode::PrintScreen},
    {GLFW_KEY_PAUSE, KeyCode::Pause},
    {GLFW_KEY_F1, KeyCode::F1},
    {GLFW_KEY_F2, KeyCode::F2},
    {GLFW_KEY_F3, KeyCode::F3},
    {GLFW_KEY_F4, KeyCode::F4},
    {GLFW_KEY_F5, KeyCode::F5},
    {GLFW_KEY_F6, KeyCode::F6},
    {GLFW_KEY_F7, KeyCode::F7},
    {GLFW_KEY_F8, KeyCode::F8},
    {GLFW_KEY_F9, KeyCode::F9},
    {GLFW_KEY_F10, KeyCode::F10},
    {GLFW_KEY_F11, KeyCode::F11},
    {GLFW_KEY_F12, KeyCode::F12},
    {GLFW_KEY_F13, KeyCode::F13},
    {GLFW_KEY_F14, KeyCode::F14},
    {GLFW_KEY_F15, KeyCode::F15},
    {GLFW_KEY_F16, KeyCode::F16},
    {GLFW_KEY_F17, KeyCode::F17},
    {GLFW_KEY_F18, KeyCode::F18},
    {GLFW_KEY_F19, KeyCode::F19},
    {GLFW_KEY_F20, KeyCode::F20},
    {GLFW_KEY_F21, KeyCode::F21},
    {GLFW_KEY_F22, KeyCode::F22},
    {GLFW_KEY_F23, KeyCode::F23},
    {GLFW_KEY_F24, KeyCode::F24},
    {GLFW_KEY_F25, KeyCode::F25},
    {GLFW_KEY_KP_0, KeyCode::Keypad0},
    {GLFW_KEY_KP_1, KeyCode::Keypad1},
    {GLFW_KEY_KP_2, KeyCode::Keypad2},
    {GLFW_KEY_KP_3, KeyCode::Keypad3},
    {GLFW_KEY_KP_4, KeyCode::Keypad4},
    {GLFW_KEY_KP_5, KeyCode::Keypad5},
    {GLFW_KEY_KP_6, KeyCode::Keypad6},
    {GLFW_KEY_KP_7, KeyCode::Keypad7},
    {GLFW_KEY_KP_8, KeyCode::Keypad8},
    {GLFW_KEY_KP_9, KeyCode::Keypad9},
    {GLFW_KEY_KP_DECIMAL, KeyCode::KeypadDecimal},
    {GLFW_KEY_KP_DIVIDE, KeyCode::KeypadDivide},
    {GLFW_KEY_KP_MULTIPLY, KeyCode::KeypadMultiply},
    {GLFW_KEY_KP_SUBTRACT, KeyCode::KeypadSubtract},
    {GLFW_KEY_KP_ADD, KeyCode::KeypadAdd},
    {GLFW_KEY_KP_ENTER, KeyCode::KeypadEnter},
    {GLFW_KEY_KP_EQUAL, KeyCode::KeypadEqual},
    {GLFW_KEY_LEFT_SHIFT, KeyCode::LeftShift},
    {GLFW_KEY_LEFT_CONTROL, KeyCode::LeftControl},
    {GLFW_KEY_LEFT_ALT, KeyCode::LeftAlt},
    {GLFW_KEY_LEFT_SUPER, KeyCode::LeftSuper},
    {GLFW_KEY_RIGHT_SHIFT, KeyCode::RightShift},
    {GLFW_KEY_RIGHT_CONTROL, KeyCode::RightControl},
    {GLFW_KEY_RIGHT_ALT, KeyCode::RightAlt},
    {GLFW_KEY_RIGHT_SUPER, KeyCode::RightSuper},
    {GLFW_KEY_MENU, KeyCode::Menu},
    {GLFW_KEY_LAST, KeyCode::Last},
};

GLFWWindow::GLFWWindow(EventDispatcher& eventDispatcher)
    : Window(eventDispatcher)
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
    m_width = width;
    m_height = height;
    m_title = title;

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

    glfwSetWindowUserPointer(m_pMainWindow, this);

    glfwSetMouseButtonCallback(m_pMainWindow, GLFWEventHandler::onGLFWMouseButtonCallback);
    glfwSetCursorPosCallback(m_pMainWindow, GLFWEventHandler::onGLFWMouseMoveCallback);
    glfwSetScrollCallback(m_pMainWindow, GLFWEventHandler::onGLFWScrollCallback);
    glfwSetKeyCallback(m_pMainWindow, GLFWEventHandler::onGLFWKeyCallback);
    glfwSetWindowSizeCallback(m_pMainWindow, GLFWEventHandler::onGLFWWindowSizeCallback);

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

void GLFWWindow::onGLFWMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    MouseEvent::MouseEventType eventType = MouseEvent::MouseEventType::Unkown;
    switch (action) {
    case GLFW_PRESS:
        eventType = MouseEvent::MouseEventType::Down;
        break;
    case GLFW_RELEASE:
        eventType = MouseEvent::MouseEventType::Up;
        break;
    default:
        break;
    }

    MouseEvent event(eventType);
    event.setPosition(math::vec2(xpos, ypos));
    m_eventDispatcher.dispatch<MouseEvent>(event);
}

void GLFWWindow::onGLFWMouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
{
    m_mousePosition.x = static_cast<float>(xpos);
    m_mousePosition.y = static_cast<float>(ypos);

    MouseEvent event(MouseEvent::MouseEventType::Move);
    event.setPosition(m_mousePosition);
    event.setLastPosition(m_lastMousePosition);
    m_eventDispatcher.dispatch<MouseEvent>(event);

    m_lastMousePosition = m_mousePosition;
}

void GLFWWindow::onGLFWScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    MouseEvent event(MouseEvent::MouseEventType::Scroll);
    event.setScrollDelta(math::vec2(static_cast<float>(xoffset), static_cast<float>(yoffset)));
    m_eventDispatcher.dispatch<MouseEvent>(event);
}

void GLFWWindow::onGLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    auto it = g_keyCodeMap.find(key);
    KeyCode keyCode = (it != g_keyCodeMap.end()) ? it->second : KeyCode::Unknown;
    const bool isPressed = (action == GLFW_PRESS || action == GLFW_REPEAT);

    KeyboardEvent event(keyCode, isPressed);
    m_eventDispatcher.dispatch<KeyboardEvent>(event);
}

void GLFWWindow::onGLFWWindowSizeCallback(GLFWwindow* window, int width, int height)
{
    m_width = width;
    m_height = height;

    WindowResizeEvent event(width, height);
    m_eventDispatcher.dispatch<WindowResizeEvent>(event);
}

} // namespace ocf
