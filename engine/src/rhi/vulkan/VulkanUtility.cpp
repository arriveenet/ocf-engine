// SPDX-License-Identifier: MIT
#include "VulkanUtility.h"

#include "ocf/core/Logger.h"

#include <assert.h>

namespace ocf {
namespace rhi {

void VulkanUtility::assertVkError(VkResult ret, const char* function, size_t line) noexcept
{
    if (ret != VK_SUCCESS) {
        OCF_LOG_ERROR("Vulkan error: {} in \"{}\" at line {}",
            static_cast<int>(ret), function, line);
        assert(false);
    }
}

void VulkanUtility::logError(const VulkanError& error)
{
    OCF_LOG_ERROR("[Vulkan Error]: {} ({})\nFile: {}:{}", error.message,
                  static_cast<int>(error.result), error.file, error.line);
}

QueueFamilyIndices VulkanUtility::findQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
            break;
        }
        i++;
    }

    return indices;
}

void VulkanUtility::transitionImageLayout(VkCommandBuffer       cmd,
                                             VkImage               image,
                                             VkImageLayout         oldLayout,
                                             VkImageLayout         newLayout,
                                             VkPipelineStageFlags2 srcStage,
                                             VkAccessFlags2        srcAccess,
                                             VkPipelineStageFlags2 dstStage,
                                             VkAccessFlags2        dstAccess,
                                             uint32_t              baseMipLevel,
                                             uint32_t              levelCount)
{
    VkImageSubresourceRange range{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = baseMipLevel,
        .levelCount = levelCount,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    VkImageMemoryBarrier2 barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = srcStage,
        .srcAccessMask = srcAccess,
        .dstStageMask = dstStage,
        .dstAccessMask = dstAccess,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = range,
    };

    VkDependencyInfo dependencyInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
    };

    vkCmdPipelineBarrier2(cmd, &dependencyInfo);
}

} // namespace rhi
} // namespace ocf
