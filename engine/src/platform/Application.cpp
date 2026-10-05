// SPDX-License-Identifier: MIT

#include "platform/GLFWWindow.h"

#include "ocf/core/Engine.h"
#include "ocf/platform/Application.h"
#include "ocf/scene/View.h"

namespace ocf {

Application& Application::getInstance()
{
    static Application instance;
    return instance;
}

Application::Application()
{
    // Constructor implementation
}

Application::~Application()
{
    // Destructor implementation
}

void Application::run(const Config& config, SetupCallback setupCallback,
                      CleanupCallback cleanupCallback, int width, int height)
{
    m_window = std::make_shared<GLFWWindow>(m_eventDispatcher);
    if (!m_window->create(config, config.title, width, height)) {
        // Handle window creation failure
        return;
    }

    Engine::Config engineConfig{
        .window = m_window,
        .eventDispatcher = m_eventDispatcher
    };

    m_engine = Engine::create(engineConfig);

    Scene* scene = m_engine->createScene();
    View* view = m_engine->createView();
    view->setScene(scene);
    m_engine->addView(view);

    if (setupCallback) {
      setupCallback(*m_engine, view, scene);
    }

    while (!m_window->windowShouldClose()) {
        // Main application loop
        m_window->pollEvents();
        m_engine->mainLoop();
    }

    if (cleanupCallback) {
      cleanupCallback(*m_engine, view, scene);
    }

    Engine::destroy(m_engine);
}

} // namespace ocf
