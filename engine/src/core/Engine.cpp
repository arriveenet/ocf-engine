// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Tsuyoshi Katayama

#include "ocf/core/Engine.h"

#include "ocf/audio/AudioSystem.h"
#include "ocf/core/Logger.h"
#include "ocf/core/job/JobSystem.h"
#include "ocf/platform/FileSystem.h"
#include "ocf/platform/Window.h"
#include "ocf/renderer/Renderer.h"
#include "ocf/rhi/Device.h"
#include "ocf/rhi/DeviceFactory.h"
#include "ocf/scene/Scene.h"

namespace ocf {

using namespace audio;
using namespace logger;
using namespace rhi;
using namespace job;

Engine::Engine(const Config& config)
    : m_window(config.window)
{
}

Engine::~Engine()
{
    m_audioSystem->shutdown();

    auto& jobSystem = JobSystem::getInstance();
    jobSystem.waitAll();
    jobSystem.shutdown();

    m_renderer.reset();
    m_device.reset();
    m_audioSystem.reset();
}

bool Engine::init()
{
    // Setup Logger
    auto consoleAppender = std::make_unique<ConsoleAppender>();
    Logger::getInstance().addAppender(std::move(consoleAppender));
    Logger::getInstance().setLogLevel(LogLevel::Debug);

    OCF_LOG_INFO("Window platform: {}", Window::platformToString(m_window->getPlatform()));

    // Initialize JobSystem
    auto& jobSystem = JobSystem::getInstance();
    jobSystem.initialize();

    // Create RHI Device
    m_device = DeviceFactory::getInstance().create();
    m_device->createSwapchain(m_window.get(), m_window->getWidth(), m_window->getHeight());

    // Initialize Renderer
    m_renderer = std::make_unique<Renderer>(*this, m_device.get());
    m_renderer->init();

    // Initialize Audio System
    m_audioSystem = std::make_unique<audio::AudioSystem>();
    m_audioSystem->initialize();

    return true;
}

Engine* Engine::create(const Config& config)
{
    Engine* engine = new Engine(config);
    if (engine && engine->init()) {
        return engine;
    }

    delete engine;
    return nullptr;
}

void Engine::destroy(Engine* engine)
{
    FileSystem::destroyInstance();
    delete engine;
}

void Engine::update()
{
    m_frameCounter.update();

    m_audioSystem->update();
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

audio::AudioSystem& Engine::getAudioSystem() const
{
    return *m_audioSystem.get();
}

math::ivec2 Engine::getWindowSize() const
{
    return math::ivec2(m_window->getWidth(), m_window->getHeight());
}

} // namespace ocf
