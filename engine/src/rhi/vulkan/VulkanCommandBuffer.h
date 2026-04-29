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

    VkCommandBuffer getHandle() const noexcept { return m_commandBuffer; }

    operator VkCommandBuffer() noexcept  { return m_commandBuffer; }
    operator VkCommandBuffer() const noexcept { return m_commandBuffer; }

private:
    VulkanDevice& m_device;
    VkCommandBuffer m_commandBuffer;
};

} // namespace ocf::rhi
