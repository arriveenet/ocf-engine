// SPDX-License-Identifier: MIT

#pragma once

#include "ocf/math/mat4.h"
#include "ocf/math/vec2.h"
#include "ocf/math/vec3.h"
#include "ocf/rhi/Handle.h"
#include "ocf/rhi/PipelineState.h"
#include "ocf/renderer/Material.h"
#include "ocf/renderer/RenderQueue.h"

#include <memory>
#include <vector>

namespace ocf {

class VertexBuffer;
class IndexBuffer;
class Texture;
class Material;
class MaterialInstance;
class Engine;
class View;

namespace rhi {
class CommandBuffer;
class Device;
}

struct Vertex {
    math::vec3 position;
    math::vec3 normal;
    math::vec3 color;
};

struct Vertex2 {
    math::vec3 position;
    math::vec2 texCoord;
};

class Renderer {
public:
    /**
     * @brief Per-draw data (the model matrix) pushed to every pipeline the renderer draws.
     *
     * Pipelines drawn by the renderer must declare this range in their layout.
     */
    static constexpr rhi::PushConstantRange OBJECT_PUSH_CONSTANT_RANGE{
        .stages = rhi::ShaderStageFlags::Vertex,
        .size = sizeof(math::mat4),
    };

    Renderer(Engine& engine, rhi::Device* device);
    ~Renderer();

    bool init();

    /** @brief Start recording a frame. Call render() for each view, then endFrame(). */
    void beginFrame();

    /** @brief Finish recording and present the frame. */
    void endFrame();

    /** @brief Record one rendering pass that draws the view over the passes before it. */
    void render(const View* view);

    /** @brief Material of the per-view uniforms bound at set 0. */
    Material* getUBOMaterial() const { return m_material; }

protected:

private:
    MaterialInstance* acquireViewUniforms();

    Engine& m_engine;
    rhi::Device* m_device = nullptr;
    rhi::TextureHandle m_depthTexture;
    VertexBuffer* m_vertexBuffer = nullptr;
    IndexBuffer* m_indexBuffer = nullptr;
    Texture* m_texture = nullptr;
    Material* m_material = nullptr;
    rhi::PipelineHandle m_pipelineHandle;
    RenderQueue m_renderQueue;

    std::shared_ptr<rhi::CommandBuffer> m_commandBuffer; // Recording between begin/endFrame
    // Camera uniforms, one per view drawn in a frame. Material cannot release a single
    // instance, so they are pooled and reused every frame.
    std::vector<MaterialInstance*> m_viewUniforms;
    size_t m_viewCount = 0; // Views drawn so far in the current frame
};

} // namespace ocf
