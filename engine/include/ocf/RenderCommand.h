#pragma once

namespace ocf {

class Program;
class VertexArrayObject;

struct RenderCommand {
    Program* program = nullptr;
    VertexArrayObject* vertexArrayObject = nullptr;
};

} // namespace ocf
