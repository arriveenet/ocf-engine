#include "VulkanCommandBuffer.h"

#include "VulkanDevice.h"

namespace ocf::rhi {

VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice& device, VkCommandBuffer commandBuffer)
    : m_device(device)
    , m_commandBuffer(commandBuffer)
{
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
    vkFreeCommandBuffers(m_device.getDevice(), m_device.getCommandPool(), 1, &m_commandBuffer);
    m_commandBuffer = VK_NULL_HANDLE;
}

void VulkanCommandBuffer::begin()
{
    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
    };
    vkBeginCommandBuffer(m_commandBuffer, &beginInfo);
}

void VulkanCommandBuffer::end()
{
    vkEndCommandBuffer(m_commandBuffer);
}

void VulkanCommandBuffer::reset()
{
    vkResetCommandBuffer(m_commandBuffer, 0);
}

} // namespace ocf::rhi
