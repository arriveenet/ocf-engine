// SPDX-License-Identifier: MIT
#include "VulkanCommandBuffer.h"

#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanHandles.h"
#include "VulkanSwapchain.h"
#include "VulkanUtility.h"
#include "resource/ImageResource.h"

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
    m_renderingCount = 0;
}

void VulkanCommandBuffer::begin(VkCommandBufferUsageFlags flags)
{
    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = flags,
    };
    vkBeginCommandBuffer(m_commandBuffer, &beginInfo);
    m_renderingCount = 0;
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
    auto depthBuffer = m_device.getDepthBuffer();

    // A previous pass in this command buffer wrote the same attachments, so its writes must
    // finish before this pass loads or clears them
    if (m_renderingCount > 0) {
        constexpr VkPipelineStageFlags2 attachmentStages =
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT |
            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
            VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

        VkMemoryBarrier2 memoryBarrier{
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
            .srcStageMask = attachmentStages,
            .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstStageMask = attachmentStages,
            .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT |
                             VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                             VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        };
        VkDependencyInfo dependencyInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .memoryBarrierCount = 1,
            .pMemoryBarriers = &memoryBarrier,
        };
        vkCmdPipelineBarrier2(m_commandBuffer, &dependencyInfo);
    }
    m_renderingCount++;

    const math::vec4 color = info.clearColor;

    // Color
    VkRenderingAttachmentInfo colorAttachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = swapchain->getCurrentImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = info.clearColorBuffer ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = VkClearValue{.color = {color.x, color.y, color.z, color.w}},
    };
    // Depth
    VkRenderingAttachmentInfo depthAttachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = depthBuffer->getImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .loadOp = info.clearDepthBuffer ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = {.depthStencil{1.0f, 0}}
    };

    VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {{0, 0}, swapchain->getExtent()},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment,
        .pDepthAttachment = info.useDepthBuffer ? &depthAttachment : nullptr,
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

void VulkanCommandBuffer::bindDescriptorSets(PipelineHandle ph, DescriptorSetHandle dsh,
                                             uint32_t firstSet, uint32_t descriptorSetCount)
{
    VulkanPipeline* pipeline = m_device.handle_cast<VulkanPipeline*>(ph);
    VulkanDescriptorSet* descriptorSet = m_device.handle_cast<VulkanDescriptorSet*>(dsh);

    vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vk.layout,
                            firstSet, descriptorSetCount, &descriptorSet->vk.id, 0, nullptr);
}

void VulkanCommandBuffer::pushConstants(PipelineHandle ph, ShaderStageFlags stages,
                                        uint32_t offset, uint32_t size, const void* data)
{
    VulkanPipeline* pipeline = m_device.handle_cast<VulkanPipeline*>(ph);

    vkCmdPushConstants(m_commandBuffer, pipeline->vk.layout,
                       VulkanUtility::getShaderStageFlagBits(stages), offset, size, data);
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

void VulkanCommandBuffer::transitionLayout(TextureHandle texture, ResourceState oldState,
                                           ResourceState newState)
{
    if (!texture) {
        return;
    }

    VulkanTexture* tex = m_device.handle_cast<VulkanTexture*>(texture);

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
        .image = tex->image->getImage(),
        .subresourceRange = tex->image->getSubresourceRange(),
    };

    VkDependencyInfo dependencyInfo{.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                    .imageMemoryBarrierCount = 1,
                                    .pImageMemoryBarriers = &imageBarrier};
    vkCmdPipelineBarrier2(m_commandBuffer, &dependencyInfo);
}

} // namespace ocf::rhi
