// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/Result.h"

#include <vulkan/vulkan.h>

namespace ocf {
namespace rhi {

struct VulkanError {
    VkResult result;
    const char* message;
    const char* file;
    size_t line;
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

} // namespace VulkanUtility

} // namespace rhi
} // namespace ocf
