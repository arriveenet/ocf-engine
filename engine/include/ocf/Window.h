#include "ocf/Application.h"
#include <SDL3/SDL.h>
#include <string>

namespace ocf {

class Window {
public:
    Window(std::string_view title, const Config& config, int width, int height);
    ~Window();

    std::string getTitle() const noexcept;

    int getWidth() const noexcept;

    int getHeight() const noexcept;

    bool create();

    void pollEvents();

    void swapBuffers();

    bool shouldClose() const;

private:
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    SDL_GLContext m_glContext = nullptr;
    std::string m_title;
    Config m_config;
    int m_width = 800;
    int m_height = 600;
    bool m_shouldClose = false;
};

} // namespace ocf