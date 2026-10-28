// SPDX-License-Identifier: MIT
#include "ocf/rhi/vulkan/VulkanCommandBuffer.h"

#include "ocf/rhi/vulkan/VulkanBuffer.h"
#include "ocf/rhi/vulkan/VulkanDevice.h"
#include "ocf/rhi/vulkan/VulkanHandles.h"
#include "ocf/rhi/vulkan/VulkanSwapchain.h"
#include "ocf/rhi/vulkan/VulkanUtility.h"
#include "ocf/rhi/vulkan/resource/ImageResource.h"

namespace {

struct AttachmentAccess {
    VkPipelineStageFlags2 stage;
    VkAccessFlags2 access;
};

/** The work that last used an attachment of an offscreen render target in the layout */
AttachmentAccess getLastAccess(VkImageLayout layout)
{
    switch (layout) {
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return {VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT};
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
        return {VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                    VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return {VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT};
    default:
        return {VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE};
    }
}

} // namespace

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

void VulkanCommandBuffer::beginRendering(RenderTargetHandle rth, const RenderingInfo& info)
{
    VulkanRenderTarget* target = m_device.handle_cast<VulkanRenderTarget*>(rth);
    m_currentTarget = target;

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

    VkImageView colorView = VK_NULL_HANDLE;
    VkImageView depthView = VK_NULL_HANDLE;
    VkExtent2D extent{};
    bool clearColor = info.clearColorBuffer;
    bool clearDepth = info.clearDepthBuffer;
    m_colorFormat = VK_FORMAT_UNDEFINED;
    m_depthFormat = VK_FORMAT_UNDEFINED;

    if (target->isDefault) {
        // The renderer moves the swapchain image and the depth buffer to the attachment layouts
        const VulkanSwapchain* swapchain = m_device.getSwapchain();
        colorView = swapchain->getCurrentImageView();
        m_colorFormat = swapchain->getImageFormat().format;
        extent = swapchain->getExtent();

        if (info.useDepthBuffer) {
            const auto depthBuffer = m_device.getDepthBuffer();
            depthView = depthBuffer->getImageView();
            m_depthFormat = depthBuffer->getFormat();
        }
    }
    else {
        extent = VkExtent2D{target->width, target->height};

        // An attachment is undefined until a pass has drawn it, so it is cleared rather than loaded
        if (target->color) {
            IImageResource& image = *target->color;
            clearColor = clearColor || image.getLayout() == VK_IMAGE_LAYOUT_UNDEFINED;
            transitionAttachment(image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                 VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                 VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT |
                                     VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
            colorView = image.getImageView();
            m_colorFormat = image.getFormat();
        }

        if (target->depth && info.useDepthBuffer) {
            IImageResource& image = *target->depth;
            clearDepth = clearDepth || image.getLayout() == VK_IMAGE_LAYOUT_UNDEFINED;
            transitionAttachment(image, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                 VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                                     VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                                 VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                     VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
            depthView = image.getImageView();
            m_depthFormat = image.getFormat();
        }
    }

    const math::vec4 color = info.clearColor;

    // Color
    VkRenderingAttachmentInfo colorAttachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = colorView,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = clearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = VkClearValue{.color = {color.x, color.y, color.z, color.w}},
    };
    // Depth
    VkRenderingAttachmentInfo depthAttachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = depthView,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .loadOp = clearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = {.depthStencil{1.0f, 0}}
    };

    const bool hasColor = colorView != VK_NULL_HANDLE;
    const bool hasDepth = depthView != VK_NULL_HANDLE;
    VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {{0, 0}, extent},
        .layerCount = 1,
        .colorAttachmentCount = hasColor ? 1u : 0u,
        .pColorAttachments = hasColor ? &colorAttachment : nullptr,
        .pDepthAttachment = hasDepth ? &depthAttachment : nullptr,
    };

    vkCmdBeginRendering(m_commandBuffer, &renderingInfo);

    // Vertical inversion by VK_KHR_Maintenance1
    const VkViewport viewport{
        .x = 0.0f,
        .y = float(extent.height),
        .width = float(extent.width),
        .height = -float(extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    const VkRect2D scissor{
        .offset = {0, 0},
        .extent = extent,
    };
    vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);
}

void VulkanCommandBuffer::endRendering()
{
    vkCmdEndRendering(m_commandBuffer);

    VulkanRenderTarget* target = m_currentTarget;
    m_currentTarget = nullptr;
    if (target == nullptr || target->isDefault) {
        return;
    }

    // Later passes may sample what this pass drew
    constexpr VkImageLayout readLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (target->color && target->colorSampleable && target->color->getLayout() != readLayout) {
        transitionAttachment(*target->color, readLayout, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                             VK_ACCESS_2_SHADER_READ_BIT);
    }
    if (target->depth && target->depthSampleable && target->depth->getLayout() != readLayout) {
        transitionAttachment(*target->depth, readLayout, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                             VK_ACCESS_2_SHADER_READ_BIT);
    }
}

void VulkanCommandBuffer::bindPipeline(PipelineHandle ph)
{
    VulkanPipeline* pipeline = m_device.handle_cast<VulkanPipeline*>(ph);
    VkPipeline vkPipeline = m_device.getPipelineVariant(*pipeline, m_colorFormat, m_depthFormat);

    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);
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

void VulkanCommandBuffer::transitionAttachment(IImageResource& image, VkImageLayout newLayout,
                                               VkPipelineStageFlags2 dstStage,
                                               VkAccessFlags2 dstAccess)
{
    // The layout is tracked while recording, which is the order the GPU runs the passes in.
    // The barrier is issued even when the layout does not change, since the previous use of the
    // image (possibly in the previous frame) must finish first.
    const VkImageLayout oldLayout = image.getLayout();
    const AttachmentAccess src = getLastAccess(oldLayout);

    VkImageMemoryBarrier2 imageBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = src.stage,
        .srcAccessMask = src.access,
        .dstStageMask = dstStage,
        .dstAccessMask = dstAccess,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image.getImage(),
        .subresourceRange = image.getSubresourceRange(),
    };

    VkDependencyInfo dependencyInfo{.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                    .imageMemoryBarrierCount = 1,
                                    .pImageMemoryBarriers = &imageBarrier};
    vkCmdPipelineBarrier2(m_commandBuffer, &dependencyInfo);

    image.setLayout(newLayout);
}

} // namespace ocf::rhi
