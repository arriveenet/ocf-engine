#include "ocf/Window.h"
#include <glad/gl.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

namespace ocf {

Window::Window(std::string_view title, const Config& config, int width, int height)
    : m_title(title)
    , m_config(config)
    , m_width(width)
    , m_height(height)
{
}

Window::~Window()
{
    SDL_GL_DestroyContext(m_glContext);
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

std::string Window::getTitle() const noexcept
{
    return m_title;
}

int Window::getWidth() const noexcept
{
    return m_width;
}

int Window::getHeight() const noexcept
{
    return m_height;
}

bool Window::create()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    uint32_t windowFlags = SDL_WINDOW_OPENGL;
    if (m_config.resizable) {
        windowFlags |= SDL_WINDOW_RESIZABLE;
    }

    if (!SDL_CreateWindowAndRenderer(m_title.c_str(), m_width, m_height, windowFlags,
                                   &m_window, &m_renderer)) {
        return false;
    }
    m_glContext = SDL_GL_CreateContext(m_window);

    if (!gladLoaderLoadGL()) {
      SDL_Log("Failed to initialize OpenGL context");
      return false;
    }

    return true;
}

void Window::pollEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            m_shouldClose = true;
        }
    }
}

void Window::swapBuffers()
{
    SDL_GL_SwapWindow(m_window);
}

bool Window::shouldClose() const
{
    return m_shouldClose;
}

} // namespace ocf
