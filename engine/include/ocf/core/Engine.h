// SPDX-License-Identifier: MIT

#pragma once
#include <chrono>
#include <memory>

namespace ocf {

namespace rhi {
class Device;
}

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

    rhi::Device* getDevice() const;

private:
    Engine();
    ~Engine();

    bool init();

    void calculateDeltaTime();

private:
    float m_deltaTime;
    std::chrono::steady_clock::time_point m_lastUpdate;
    std::unique_ptr<Scene> m_currentScene;
    std::unique_ptr<rhi::Device> m_device;
};

} // namespace ocf
