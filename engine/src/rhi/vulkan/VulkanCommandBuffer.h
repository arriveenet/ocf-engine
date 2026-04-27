// SPDX-License-Identifier: MIT

#pragma once

#include "ocf/rhi/CommandBuffer.h"

#include <vulkan/vulkan.h>

namespace ocf::rhi {

class VulkanContext;

class VulkanCommandBuffer : public CommandBuffer {
public:
    VulkanCommandBuffer(VulkanContext& context, VkCommandBuffer commandBuffer);
    ~VulkanCommandBuffer() override;

    void begin() override;

    void end() override;

    void reset() override;

    VkCommandBuffer getHandle() const noexcept { return m_commandBuffer; }

    operator VkCommandBuffer() noexcept  { return m_commandBuffer; }
    operator VkCommandBuffer() const noexcept { return m_commandBuffer; }

private:
    VulkanContext& m_context;
    VkCommandBuffer m_commandBuffer;
};

} // namespace ocf::rhi
