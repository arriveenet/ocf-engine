// SPDX-License-Identifier: MIT

#pragma once

#include "ocf/math/vec3.h"
#include "ocf/renderer/VertexBuffer.h"

namespace ocf {

namespace rhi {
class Device;
}

struct Vertex {
    math::vec3 position;
    math::vec3 color;
};

class Renderer {
public:
    Renderer(Engine& engine, rhi::Device* device);
    ~Renderer();

    void beginFrame();

    void endFrame();

    void render();

private:
    Engine& m_engine;
    rhi::Device* m_device = nullptr;
    VertexBuffer* m_vertexBuffer = nullptr;
};

} // namespace ocf
