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

} // namespace rhi
} // namespace ocf
