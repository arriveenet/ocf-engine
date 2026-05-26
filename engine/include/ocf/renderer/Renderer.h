// SPDX-License-Identifier: MIT

#pragma once

#include "ocf/math/vec3.h"
#include <ocf/rhi/Handle.h>

namespace ocf {

class VertexBuffer;
class IndexBuffer;
class Texture;
class Material;
class Engine;

namespace rhi {
class Device;
}

struct Vertex {
    math::vec3 position;
    math::vec3 normal;
    math::vec3 color;
};

class Renderer {
public:
    Renderer(Engine& engine, rhi::Device* device);
    ~Renderer();

    bool init();

    void beginFrame();

    void endFrame();

    void render();

private:
    Engine& m_engine;
    rhi::Device* m_device = nullptr;
    VertexBuffer* m_vertexBuffer = nullptr;
    IndexBuffer* m_indexBuffer = nullptr;
    Texture* m_texture = nullptr;
    Material* m_material = nullptr;
    rhi::PipelineHandle m_pipelineHandle;
};

} // namespace ocf
