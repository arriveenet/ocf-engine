// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Tsuyoshi Katayama

#include "ocf/core/Engine.h"

#include "ocf/audio/AudioSystem.h"
#include "ocf/core/Logger.h"
#include "ocf/core/job/JobSystem.h"
#include "ocf/platform/FileSystem.h"
#include "ocf/platform/Window.h"
#include "ocf/renderer/Renderer.h"
#include "ocf/resource/FontManager.h"
#include "ocf/resource/TextureManager.h"
#include "ocf/rhi/Device.h"
#include "ocf/rhi/DeviceFactory.h"
#include "ocf/scene/SceneManager.h"
#include "ocf/scene/View.h"

#include "ocf/renderer/ImGuiLayer.h"

#include <imgui.h>

namespace ocf {

using namespace audio;
using namespace logger;
using namespace rhi;
using namespace job;

Engine::Engine(const Config& config)
    : m_window(config.window)
    , m_eventDispatcher(config.eventDispatcher)
    , m_vsync(config.vsync)
{
}

Engine::~Engine()
{
    // Scenes own GPU resources and may still be loading on a worker thread, so they go
    // before the managers, the job system and the device
    m_sceneManager.reset();

    // Fonts own atlas textures, so release them while the device is still alive
    m_fontManager->clear();
    m_fontManager.reset();

    m_textureManager.reset();
    m_audioSystem->shutdown();

    auto& jobSystem = JobSystem::getInstance();
    jobSystem.waitAll();
    jobSystem.shutdown();

#if OCF_ENABLE_IMGUI
    // Owns GPU resources, so it goes before the device
    m_imGuiLayer.reset();
#endif

    m_renderer.reset();
    m_device.reset();
    m_audioSystem.reset();
}

bool Engine::init()
{
    // Setup Logger
    auto consoleAppender = std::make_unique<ConsoleAppender>();
    Logger::getInstance().addAppender(std::move(consoleAppender));
    Logger::getInstance().setLogLevel(LogLevel::Trace);

    OCF_LOG_INFO("Window platform: {}", Window::platformToString(m_window->getPlatform()));

    // Initialize JobSystem
    auto& jobSystem = JobSystem::getInstance();
    jobSystem.initialize();

    // Create RHI Device
    m_device = DeviceFactory::getInstance().create();
    m_device->createSwapchain(m_window.get(), m_window->getWidth(), m_window->getHeight(),
                              m_vsync);

    // Initialize Renderer
    m_renderer = std::make_unique<Renderer>(*this, m_device.get());
    m_renderer->init();

    // Initialize Texture Manager
    m_textureManager = std::make_unique<TextureManager>(*this);

    // Initialize Font Manager
    m_fontManager = std::make_unique<FontManager>(*this);
  
    // Initialize Audio System
    m_audioSystem = std::make_unique<audio::AudioSystem>();
    m_audioSystem->initialize();

#if OCF_ENABLE_IMGUI
    // Before the scene manager, so that it receives the input events first
    m_imGuiLayer = std::make_unique<ImGuiLayer>(*m_window, *m_device, m_eventDispatcher);
#endif

    // Initialize Scene Manager
    m_sceneManager = std::make_unique<SceneManager>(this);

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
    m_sceneManager->update(m_frameCounter.getDeltaTime());

#if OCF_ENABLE_IMGUI
    // The ImGui frame only spans Scene::onImGui(), the one place scenes may use ImGui from
    m_imGuiLayer->beginFrame();
    m_sceneManager->updateImGui();
#endif
}

void Engine::draw()
{
    m_sceneManager->collectViews(m_frameViews);

    // No swapchain image this frame (e.g. the window is minimized), so nothing is drawn
    if (!m_renderer->beginFrame()) {
#if OCF_ENABLE_IMGUI
        // update() already started the ImGui frame
        m_imGuiLayer->endFrame();
#endif
        return;
    }

    for (auto view : m_frameViews) {
        m_renderer->render(view);
    }

#if OCF_ENABLE_IMGUI
    // The debug UI is drawn over the views
    m_renderer->renderImGui(*m_imGuiLayer);
#endif

    m_renderer->endFrame();
}

void Engine::mainLoop()
{
    // Update
    update();

    // Draw
    draw();
}

SceneManager& Engine::getSceneManager() const
{
    return *m_sceneManager.get();
}

Device& Engine::getDevice() const
{
    return *m_device.get();
}

Renderer& Engine::getRenderer() const
{
    return *m_renderer.get();
}

TextureManager& Engine::getTextureManager() const
{
    return *m_textureManager.get();
}

FontManager& Engine::getFontManager() const
{
    return *m_fontManager.get();
}

audio::AudioSystem& Engine::getAudioSystem() const
{
    return *m_audioSystem.get();
}

math::ivec2 Engine::getWindowSize() const
{
    return math::ivec2(m_window->getWidth(), m_window->getHeight());
}

void Engine::configureImGui([[maybe_unused]] const std::function<void(ImGuiIO&)>& configure)
{
#if OCF_ENABLE_IMGUI
    configure(ImGui::GetIO());
#endif
}

} // namespace ocf
