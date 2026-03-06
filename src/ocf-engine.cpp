#include "ocf-engine.h"

#include <SDL3/SDL.h>

namespace ocf {

Engine::Engine()
    : m_running(false)
{
}

Engine::~Engine()
{
}

bool Engine::init()
{
    if (m_running) {
        return true;
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    m_running = true;
    return true;
}

void Engine::run()
{
    SDL_Event event;
    while (m_running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                m_running = false;
            }
        }
    }
}

void Engine::quit()
{
    m_running = false;
    SDL_Quit();
}

} // namespace ocf
