// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/Result.h"
#include "ocf/rhi/RHIEnums.h"

#include <vulkan/vulkan.h>

#include <optional>

namespace ocf {
namespace rhi {

struct VulkanError {
    VkResult result;
    const char* message;
    const char* file;
    size_t line;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
};

struct VulkanResourceStateInfo {
    VkImageLayout layout;
    VkAccessFlags accessMask;
    VkPipelineStageFlags stageFlags;
};

using VulkanResult = ocf::Result<void, VulkanError>;

namespace VulkanUtility {

void assertVkError(VkResult ret, const char* function, size_t line) noexcept;

#ifdef NDEBUG
#   define VK_CHECK_RESULT(ret)
#else
#   define VK_CHECK_RESULT(ret) { VulkanUtility::assertVkError(ret, __func__, __LINE__); }
#endif

#define VK_ERROR(result, msg)                                                                      \
    VulkanError                                                                                    \
    {                                                                                              \
        result, msg, __FILE__, __LINE__                                                            \
    }

void logError(const VulkanError& error);

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

VulkanResourceStateInfo getResourceState(ResourceState state);

} // namespace VulkanUtility

} // namespace rhi
} // namespace ocf
