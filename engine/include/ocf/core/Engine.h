// SPDX - License - Identifier : MIT

#pragma once
#include <chrono>
#include <memory>

namespace ocf {

class Scene;
class Renderer;

class Engine {
public:
    static Engine* create();
    static void destroy(Engine* engine);

    void update();
    void draw();

    void mainLoop();

    Scene* createScene();

private:
    Engine();
    ~Engine();

    bool init();

    void calculateDeltaTime();

private:
    float m_deltaTime;
    std::chrono::steady_clock::time_point m_lastUpdate;
    std::unique_ptr<Scene> m_currentScene;
};

} // namespace ocf
