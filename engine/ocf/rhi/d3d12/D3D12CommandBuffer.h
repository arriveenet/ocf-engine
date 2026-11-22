// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/rhi/CommandBuffer.h"

namespace ocf::rhi {

class D3D12Device;

/**
 * @brief Command buffer of the Direct3D 12 backend.
 *
 * @note Not implemented yet: every command is ignored.
 */
class D3D12CommandBuffer : public CommandBuffer {
public:
    explicit D3D12CommandBuffer(D3D12Device& device);
    ~D3D12CommandBuffer() override;

    void begin() override;

    void end() override;

    void reset() override;

    void beginRendering(RenderTargetHandle rth, const RenderingInfo& info) override;

    void endRendering() override;

    void bindPipeline(PipelineHandle ph) override;

    void bindDescriptorSets(PipelineHandle ph, DescriptorSetHandle dsh, uint32_t firstSet,
                            uint32_t descriptorSetCount) override;

    void pushConstants(PipelineHandle ph, ShaderStageFlags stages, uint32_t offset, uint32_t size,
                       const void* data) override;

    void bindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount,
                           VertexBufferHandle vbh) override;

    void bindIndexBuffer(IndexBufferHandle ibh, uint32_t offset) override;

    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
              uint32_t firstInstance) override;

    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                     uint32_t vertexOffset, uint32_t firstInstance) override;

    void transitionLayout(ResourceState oldState, ResourceState newState) override;

    void transitionLayout(TextureHandle texture, ResourceState oldState,
                          ResourceState newState) override;

private:
    D3D12Device& m_device;
};

} // namespace ocf::rhi
