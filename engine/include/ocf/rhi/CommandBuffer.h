// SPDX-License-Identifier: MIT

#pragma once

#include "ocf/rhi/RHIEnums.h"
#include "ocf/rhi/Handle.h"

namespace ocf::rhi {

class CommandBuffer {
public:
    virtual ~CommandBuffer() = default;

    virtual void begin() = 0;

    virtual void end() = 0;

    virtual void reset() = 0;

    virtual void beginRendering(const RenderingInfo& info) = 0;

    virtual void endRendering() = 0;

    virtual void bindPipeline(PipelineHandle ph) = 0;

    virtual void bindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount,
                                   VertexBufferHandle vbh) = 0;

    virtual void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                      uint32_t firstInstance) = 0;

    virtual void transitionLayout(ResourceState oldState, ResourceState newState) = 0;
};

} // namespace ocf::rhi
