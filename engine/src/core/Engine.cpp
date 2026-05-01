// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Tsuyoshi Katayama

#include "ocf/core/Engine.h"

#include "ocf/core/Logger.h"
#include "ocf/renderer/Renderer.h"
#include "ocf/rhi/Device.h"
#include "ocf/rhi/DeviceFactory.h"
#include "ocf/scene/Scene.h"
#include "ocf/platform/FileSystem.h"

namespace ocf {

using namespace logger;
using namespace rhi;

Engine::Engine()
    : m_deltaTime(0.0f)
    , m_lastUpdate(std::chrono::steady_clock::now())
{
}

Engine::~Engine()
{
    m_renderer.reset();
    m_device.reset();
}

bool Engine::init()
{
    // Setup Logger
    auto consoleAppender = std::make_unique<ConsoleAppender>();
    Logger::getInstance().addAppender(std::move(consoleAppender));
    Logger::getInstance().setLogLevel(LogLevel::Debug);

    m_device = DeviceFactory::getInstance().create();

    m_renderer = std::make_unique<Renderer>(*this, m_device.get());

    return true;
}

Engine* Engine::create()
{
    Engine* engine = new Engine();
    if (engine && engine->init()) {
        return engine;
    }

    return engine;
}

void Engine::destroy(Engine* engine)
{
    FileSystem::destroyInstance();
    delete engine;
}

void Engine::update()
{
    calculateDeltaTime();
}

void Engine::draw()
{
    m_renderer->beginFrame();
    m_renderer->render();
    m_renderer->endFrame();
}

void Engine::mainLoop()
{
    update();
    draw();
}

Scene* Engine::createScene()
{
    m_currentScene = std::make_unique<Scene>();
    return m_currentScene.get();
}

Device& Engine::getDevice() const
{
    return *m_device.get();
}

Renderer& Engine::getRenderer() const
{
    return *m_renderer.get();
}

void Engine::calculateDeltaTime()
{
    auto now = std::chrono::steady_clock::now();
    m_deltaTime =
        std::chrono::duration_cast<std::chrono::microseconds>(now - m_lastUpdate).count() /
        1000000.0f;
    m_lastUpdate = now;
}

} // namespace ocf
