#include "ocf/Engine.h"
#include "ocf/FileUtils.h"
#include "ocf/Renderer.h"
#include "ocf/Scene.h"
#include "ocf/Window.h"

namespace ocf {

Engine* Engine::create()
{
    Engine* engine = new Engine();
    return engine;
}

void Engine::destroy(Engine* engine)
{
    engine->terminate();
    delete engine;
}

Engine::Engine()
{
}

Engine::~Engine()
{
}

bool Engine::init(std::string_view title, int width, int height, Scene *scene)
{
    m_renderer = new Renderer();
    m_currentScene = scene;

    return true;
}

Scene* Engine::getCurrentScene() const noexcept
{
    return m_currentScene;
}

Scene* Engine::createScene()
{
    Scene* scene = new Scene();
    return scene;
}

void Engine::destroyScene(Scene* scene)
{
    delete scene;
}

void Engine::terminate()
{
    delete m_renderer;
    m_renderer = nullptr;

    FileUtils::destroyInstance();
}

} // namespace ocf
