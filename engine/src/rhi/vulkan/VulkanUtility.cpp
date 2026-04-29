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

VulkanResourceStateInfo VulkanUtility::getResourceState(ResourceState state)
{
    switch (state) {
    case ocf::rhi::ResourceState::Undefined:
        return {.layout = VK_IMAGE_LAYOUT_UNDEFINED,
                .accessMask = 0,
                .stageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        };
    case ocf::rhi::ResourceState::ColorAttachment:
        return {
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .accessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        };
    case ocf::rhi::ResourceState::Present:
        return {
            .layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .accessMask = 0,
            .stageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        };
    default:
        return {
            .layout = VK_IMAGE_LAYOUT_UNDEFINED,
            .accessMask = 0,
            .stageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        };
    }
}

} // namespace rhi
} // namespace ocf
