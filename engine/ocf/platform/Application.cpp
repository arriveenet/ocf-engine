// SPDX-License-Identifier: MIT

#include "ocf/platform/GLFWWindow.h"

#include "ocf/core/Engine.h"
#include "ocf/platform/Application.h"

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
        .eventDispatcher = m_eventDispatcher,
        .vsync = config.vsync
    };

    m_engine = Engine::create(engineConfig);

    if (setupCallback) {
      setupCallback(*m_engine);
    }

    while (!m_window->windowShouldClose()) {
        // Main application loop
        m_window->pollEvents();
        m_engine->mainLoop();
    }

    if (cleanupCallback) {
      cleanupCallback(*m_engine);
    }

    Engine::destroy(m_engine);
}

} // namespace ocf
