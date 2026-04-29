#include "VulkanCommandBuffer.h"

#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanUtility.h"

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

void VulkanCommandBuffer::beginRendering(const RenderingInfo& info)
{
    auto swapchain = m_device.getSwapchain();

    VkRenderingAttachmentInfo colorAttachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = swapchain->getCurrentImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = VkClearValue{.color = {0.6f, 0.2f, 0.3f, 1.0f}},
    };

    VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {{0, 0}, swapchain->getExtent()},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment
    };

    vkCmdBeginRendering(m_commandBuffer, &renderingInfo);
}

void VulkanCommandBuffer::endRendering()
{
    vkCmdEndRendering(m_commandBuffer);
}

void VulkanCommandBuffer::transitionLayout(ResourceState oldState, ResourceState newState)
{
    auto swapchain = m_device.getSwapchain();

    const auto src = VulkanUtility::getResourceState(oldState);
    const auto dest = VulkanUtility::getResourceState(newState);

    VkImageSubresourceRange range{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    VkImageMemoryBarrier2 imageBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = src.stageFlags,
        .srcAccessMask = src.accessMask,
        .dstStageMask = dest.stageFlags,
        .dstAccessMask = dest.accessMask,
        .oldLayout = src.layout,
        .newLayout = dest.layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapchain->getCurrentImage(),
        .subresourceRange = range,
    };

    VkDependencyInfo dependencyInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imageBarrier
    };
    vkCmdPipelineBarrier2(m_commandBuffer, &dependencyInfo);
}

} // namespace ocf::rhi
