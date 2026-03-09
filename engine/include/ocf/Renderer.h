#pragma once
#include <glad/gl.h>

namespace ocf {

class VertexArrayObject;
class Program;

class Renderer {
public:
    Renderer();
    virtual ~Renderer();

    void render();

private:
    Program* m_program = nullptr;
    VertexArrayObject* m_vertexArrayObject = nullptr;
};

} // namespace ocf
