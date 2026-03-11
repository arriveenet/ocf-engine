#pragma once
#include "ocf/RenderCommand.h"
#include "ocf/RenderQueue.h"

namespace ocf {

class Scene;
class VertexArrayObject;
class Program;

class Renderer {
public:
    Renderer();
    virtual ~Renderer();

    void pushCommand(const RenderCommand& command);

    void render(Scene* scene);

private:
    RenderQueue m_renderQueue;
};

} // namespace ocf
