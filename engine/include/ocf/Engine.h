#pragma once
#include <string_view>

namespace ocf {

class Scene;
class Renderer;
class Window;

class Engine {
public:
    Engine();
    ~Engine();

    bool init(std::string_view title, int width, int height, Scene *scene);

    Scene *getCurrentScene() const noexcept;

    void run();

    void terminate();

private:
    Renderer* m_renderer = nullptr;
    Window* m_window = nullptr;
    Scene* m_currentScene = nullptr;
};

} // namespace ocf
