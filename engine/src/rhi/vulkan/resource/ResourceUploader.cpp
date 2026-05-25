// SPDX-License-Identifier: MIT
#include "ResourceUploader.h"

#include "vulkan/VulkanCommandBuffer.h"

#include <string.h>

namespace ocf::rhi {

bool ResourceUploader::initialize(VulkanDevice& device)
{
    m_vulkanDevice = &device;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    auto result = vkCreateFence(m_vulkanDevice->getDevice(), &fenceInfo, nullptr, &m_transferFence);
    return result == VK_SUCCESS;
}

void ResourceUploader::cleanup()
{
    vkDestroyFence(m_vulkanDevice->getDevice(), m_transferFence, nullptr);
    m_transferFence = VK_NULL_HANDLE;
}

bool ResourceUploader::uploadBuffer(IBufferResource* target, const void* pData, size_t size,
                                          VkAccessFlags nextAccessMask)
{
    if (target->isHostAccessible()) {
        if (void* p = target->map(); p != nullptr) {
            memcpy(p, pData, size);
            target->unmap();
            return true;
        }
        return false;
    }

    auto stagingBuffer = StagingBuffer::create(m_vulkanDevice->getDevice(), size);
    if (!stagingBuffer) {
        return false;
    }

    void* mapped = stagingBuffer->map();
    memcpy(mapped, pData, size);
    stagingBuffer->unmap();

    m_transferEntries.emplace_back(PendingTransfer{.stagingBuffer = std::move(stagingBuffer),
                                                   .destinationBuffer = target,
                                                   .dstAccessMask = nextAccessMask});
    return true;
}

bool ResourceUploader::uploadImage(std::shared_ptr<IImageResource> target,
                                   TextureUploadRequest& request)
{
    auto& entry = m_transferImageEntries.emplace_back();
    entry.destinationTexture = target;
    entry.stagingBuffer = request.staging;
    entry.copyRegions = request.copyRegions;
    entry.genMipmaps = target->getMipmapCount() != request.copyRegions.size();
    entry.dstAccessMask = request.nextAccessFlags;
    entry.dstImageLayout = request.nextLayout;
    entry.dstStageFlags = request.nextStageFlags;

    return true;
}

void ResourceUploader::submitAndWait()
{
    VkDevice device = m_vulkanDevice->getDevice();
    VkQueue queue = m_vulkanDevice->getGraphicsQueue();

    auto commandBuffer = m_vulkanDevice->createCommandBuffer();
    commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    for (auto& entry : m_transferEntries) {
        IBufferResource* dst = entry.destinationBuffer;
        auto& src = entry.stagingBuffer;

        VkBufferCopy copyResion{
            .srcOffset = 0,
            .dstOffset = 0,
            .size = dst->getBufferSize(),
        };

        vkCmdCopyBuffer(*commandBuffer, src->getBuffer(), dst->getBuffer(), 1, &copyResion);
    }

    std::vector<VkBufferMemoryBarrier2> barriers;
    for (auto& entry : m_transferEntries) {
        IBufferResource* dst = entry.destinationBuffer;
        auto dstAccessMask = dst->getAccessFlags();

        VkBufferMemoryBarrier2 barrier{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .dstAccessMask = dstAccessMask,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = dst->getBuffer(),
            .offset = 0,
            .size = dst->getBufferSize()
        };
        barriers.push_back(barrier);
    }

    if (!barriers.empty()) {
        VkDependencyInfo depInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .bufferMemoryBarrierCount = static_cast<uint32_t>(barriers.size()),
            .pBufferMemoryBarriers = barriers.data()
        };
        vkCmdPipelineBarrier2(*commandBuffer, &depInfo);
    }

    commandBuffer->end();
    auto cmd = commandBuffer->getHandle();
    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd
    };

    vkResetFences(device, 1, &m_transferFence);
    vkQueueSubmit(queue, 1, &submitInfo, m_transferFence);
    vkWaitForFences(device, 1, &m_transferFence, VK_TRUE, UINT64_MAX);

    // Process image
    for (auto& entry : m_transferImageEntries) {
        auto dst = entry.destinationTexture;
        auto src = entry.stagingBuffer;

        VkImageSubresourceRange range{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        VkImageMemoryBarrier2 barrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = dst->getImage(),
            .subresourceRange = range,
        };
        VkDependencyInfo dependencyInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier,
        };
        vkCmdPipelineBarrier2(*commandBuffer, &dependencyInfo);

        vkCmdCopyBufferToImage(*commandBuffer, src->getBuffer(), dst->getImage(),
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               uint32_t(entry.copyRegions.size()), entry.copyRegions.data());

        if (entry.genMipmaps) {
            createMipmap(commandBuffer, entry);
        }
        else {
            barrier = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstStageMask = entry.dstStageFlags,
                .dstAccessMask = entry.dstAccessMask,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .newLayout = entry.dstImageLayout,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = dst->getImage(),
                .subresourceRange = range,
            };
            dependencyInfo.pImageMemoryBarriers = &barrier;
            vkCmdPipelineBarrier2(*commandBuffer, &dependencyInfo);

            dst->setLayout(entry.dstImageLayout);
            dst->setAccessFlag(entry.dstAccessMask);
        }
    }

    m_transferEntries.clear();
    commandBuffer.reset();
}

void ResourceUploader::createMipmap(std::shared_ptr<CommandBuffer> commandBuffer,
                                    PendingImageTransfer& entry)
{
}

} // namespace ocf::rhi