#include "ocf/Application.h"
#include "ocf/Window.h"
#include "ocf/Engine.h"
#include "ocf/Scene.h"
#include "ocf/Renderer.h"

namespace ocf {

Application& Application::getInstance()
{
    static Application instance;
    return instance;
}

Application::Application()
{
}

Application::~Application()
{
}

void Application::run(const Config& config, SetupCallback setupCallback,
                      CleanupCallback cleanupCallback, int width, int height)
{
    m_window = std::make_unique<Window>(config.title, config, width, height);

    if (!m_window->create()) {
        return;
    }

    m_engine = Engine::create();
    Scene* scene = m_engine->createScene();
    setupCallback(m_engine, scene);
    if (!m_engine->init(config.title, width, height, scene)) {
        return;
    }

    Renderer* renderer = m_engine->getRenderer();

    while (!m_window->shouldClose()) {
        m_window->pollEvents();

        // Assume a fixed time step of 16ms
        scene->update(0.016f);

        renderer->render(scene);

        m_window->swapBuffers();
     }

     cleanupCallback(m_engine, scene);
     m_engine->destroyScene(scene);
     Engine::destroy(m_engine);
}

} // namespace ocf
