// SPDX-License-Identifier: MIT
#pragma once
#include "ocf/rhi/Handle.h"
#include "ocf/math/mat4.h"

#include <cstdint>
#include <vector>

namespace ocf {

class VertexBuffer;
class IndexBuffer;
class MaterialInstance;

struct RenderCommand {
    VertexBuffer* vertexBuffer = nullptr;
    IndexBuffer* indexBuffer = nullptr;
    MaterialInstance* materialInstance = nullptr;
    rhi::PipelineHandle pipelineHandle;

    math::mat4 matWorld = math::mat4(1.0f);

    uint32_t indexCount = 0;
    uint32_t indexOffset = 0;
    uint32_t vertexCount = 0;
    uint32_t vertexOffset = 0;
};

class RenderQueue {
public:
    RenderQueue();
    virtual ~RenderQueue();

    void clear();

    bool empty() const;

    void addCommand(const RenderCommand& command);

    const std::vector<RenderCommand>& getRenderCommands() const noexcept { return m_renderCommands; }

    void sort();

private:
    std::vector<RenderCommand> m_renderCommands;

};

} // namespace ocf
