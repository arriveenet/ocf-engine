#include "ocf/Engine.h"
#include "ocf/FileUtils.h"
#include "ocf/Renderer.h"
#include "ocf/Scene.h"
#include "ocf/Window.h"

namespace ocf {

Engine::Engine()
{
}

Engine::~Engine()
{
    FileUtils::destroyInstance();
}

bool Engine::init(std::string_view title, int width, int height, Scene *scene)
{
  m_window = new Window(title.data(), width, height);
    if(!m_window->create()) {
        delete m_window;
        return false;
    }

    m_renderer = new Renderer();
    m_currentScene = scene;

    return true;
}

Scene* Engine::getCurrentScene() const noexcept
{
    return m_currentScene;
}

void Engine::run()
{
    while (!m_window->shouldClose()) {
        m_window->pollEvents();

        // Assume a fixed time step of 16ms
        m_currentScene->update(0.016f);

        m_renderer->render(m_currentScene);

        m_window->swapBuffers();
    }
}

void Engine::terminate()
{
    delete m_renderer;
    m_renderer = nullptr;

    delete m_window;
    m_window = nullptr;
}

} // namespace ocf
