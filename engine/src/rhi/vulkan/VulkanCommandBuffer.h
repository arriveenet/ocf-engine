// SPDX-License-Identifier: MIT

#pragma once

#include "ocf/rhi/CommandBuffer.h"

#include <vulkan/vulkan.h>

namespace ocf::rhi {

class VulkanDevice;

class VulkanCommandBuffer : public CommandBuffer {
public:
    VulkanCommandBuffer(VulkanDevice& device, VkCommandBuffer commandBuffer);
    ~VulkanCommandBuffer() override;

    void begin() override;

    void end() override;

    void reset() override;

    void beginRendering(const RenderingInfo& info) override;

    void endRendering() override;

    void bindPipeline(PipelineHandle ph) override;

    void bindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount,
                           VertexBufferHandle vbh) override;

    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
              uint32_t firstInstance) override;

    void transitionLayout(ResourceState oldState, ResourceState newState) override;

    VkCommandBuffer getHandle() const noexcept { return m_commandBuffer; }

    operator VkCommandBuffer() noexcept  { return m_commandBuffer; }
    operator VkCommandBuffer() const noexcept { return m_commandBuffer; }

private:
    VulkanDevice& m_device;
    VkCommandBuffer m_commandBuffer;
};

} // namespace ocf::rhi
