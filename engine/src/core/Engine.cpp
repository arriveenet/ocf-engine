/* SPDX - License - Identifier : MIT */
/*
 *
 * Copyright (c) 2025 Tsuyoshi KATAYAMA
 *
 */

#include "ocf/core/Engine.h"

#include "ocf/core/Logger.h"
#include "ocf/scene/Scene.h"
#include "ocf/rhi/Device.h"
#include "ocf/rhi/DeviceFactory.h"

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
}

bool Engine::init()
{
    // Setup Logger
    auto consoleAppender = std::make_unique<ConsoleAppender>();
    Logger::getInstance().addAppender(std::move(consoleAppender));
    Logger::getInstance().setLogLevel(LogLevel::Debug);

    m_device = DeviceFactory::getInstance().create();

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
    delete engine;
}

void Engine::update()
{
    calculateDeltaTime();
}

void Engine::draw()
{
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

Device* Engine::getDevice() const
{
    return m_device.get();
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
