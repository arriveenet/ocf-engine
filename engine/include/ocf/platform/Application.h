// SPDX-License-Identifier: MIT

#pragma once
#include "ocf/platform/platform.h"
#include <functional>
#include <memory>
#include <string>

namespace ocf {

class Engine;
class Window;
class Scene;

class OCF_API Application {
public:
    using SetupCallback = std::function<void(Engine& engine, Scene* scene)>;
    using CleanupCallback = std::function<void(Engine& engine, Scene* scene)>;

    struct Config {
        std::string title;
        bool vsync = true;
    };

    static Application& getInstance();

    ~Application();

    void run(const Config& config, SetupCallback setupCallback, CleanupCallback cleanupCallback,
             int width = 800, int height = 600);

    Application(const Application& rhs) = delete;
    Application& operator=(const Application& rhs) = delete;
    Application(Application&& rhs) = delete;
    Application& operator=(Application&& rhs) = delete;
private:
    Application();

private:
    Engine* m_engine = nullptr;
    std::unique_ptr<Window> m_window;
};

} // namespace ocf
