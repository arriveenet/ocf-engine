// SPDX-License-Identifier: MIT
#include "VulkanCommandBuffer.h"

#include "VulkanBuffer.h"
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

void VulkanCommandBuffer::begin(VkCommandBufferUsageFlags flags)
{
    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = flags,
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

    const math::vec4 color = info.clearColor;

    // Color
    VkRenderingAttachmentInfo colorAttachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = swapchain->getCurrentImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = VkClearValue{.color = {color.x, color.y, color.z, color.w}},
    };
    // Depth
    VkRenderingAttachmentInfo depthAttachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = swapchain->getCurrentImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = {.depthStencil{1.0f, 0}}
    };

    VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {{0, 0}, swapchain->getExtent()},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment,
        .pDepthAttachment = &depthAttachment,
    };

    vkCmdBeginRendering(m_commandBuffer, &renderingInfo);
}

void VulkanCommandBuffer::endRendering()
{
    vkCmdEndRendering(m_commandBuffer);
}

void VulkanCommandBuffer::bindPipeline(PipelineHandle ph)
{
    VulkanPipeline* pipeline = m_device.handle_cast<VulkanPipeline*>(ph);

    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vk.pipeline);
}

void VulkanCommandBuffer::bindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount,
                                            VertexBufferHandle vbh)
{
    VulkanVertexBuffer* vb = m_device.handle_cast<VulkanVertexBuffer*>(vbh);
    VkBuffer buffer = vb->getBuffer();
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(m_commandBuffer, firstBinding, bindingCount, &buffer, offsets);
}

void VulkanCommandBuffer::bindIndexBuffer(IndexBufferHandle ibh, uint32_t offset)
{
    VulkanIndexBuffer* ib = m_device.handle_cast<VulkanIndexBuffer*>(ibh);
    VkBuffer buffer = ib->getBuffer();
    VkIndexType indexType = (ib->elementSize == 4) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
    vkCmdBindIndexBuffer(m_commandBuffer, buffer, 0, indexType);
}

void VulkanCommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                               uint32_t firstInstance)
{
    vkCmdDraw(m_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount,
                                    uint32_t firstIndex, uint32_t vertexOffset,
                                    uint32_t firstInstance)
{
    vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset,
                     firstInstance);
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
