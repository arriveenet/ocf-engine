#pragma once
#include "ocf/RenderCommand.h"
#include "ocf/RenderQueue.h"

namespace ocf {

class VertexArrayObject;
class Program;

class Renderer {
public:
    Renderer();
    virtual ~Renderer();

    void pushCommand(const RenderCommand& command);

    void render();

private:
    Program* m_program = nullptr;
    VertexArrayObject* m_vertexArrayObject = nullptr;
    RenderQueue m_renderQueue;
};

} // namespace ocf
