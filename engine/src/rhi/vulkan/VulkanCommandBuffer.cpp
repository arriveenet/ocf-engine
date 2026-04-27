#include "VulkanCommandBuffer.h"

#include "VulkanContext.h"

namespace ocf::rhi {

VulkanCommandBuffer::VulkanCommandBuffer(VulkanContext& context, VkCommandBuffer commandBuffer)
    : m_context(context)
    , m_commandBuffer(commandBuffer)
{
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
    vkFreeCommandBuffers(m_context.getDevice(), m_context.getCommandPool(), 1, &m_commandBuffer);
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
