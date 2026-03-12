#pragma once
#include <string_view>

namespace ocf {

class Scene;
class Renderer;
class Window;

class Engine {
public:
    static Engine* create();
    static void destroy(Engine* engine);

    Engine();
    ~Engine();

    bool init(std::string_view title, int width, int height, Scene *scene);

    Scene *getCurrentScene() const noexcept;

    Scene* createScene();

    void destroyScene(Scene* scene);

    Renderer* getRenderer() const noexcept { return m_renderer; }


    void terminate();

private:
    Renderer* m_renderer = nullptr;
    Scene* m_currentScene = nullptr;
};

} // namespace ocf
