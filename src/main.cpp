#include <SDL3/SDL.h>
#include <glm/glm.hpp>

int main(int argc, char* argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "ocf-engine",
        1280, 720,
        0
    );
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Example GLM usage: 3D position vector
    glm::vec3 position(0.0f, 0.0f, 0.0f);
    SDL_Log("Initial position: (%.1f, %.1f, %.1f)", position.x, position.y, position.z);

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
