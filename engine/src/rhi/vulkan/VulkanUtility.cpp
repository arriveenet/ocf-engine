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

} // namespace rhi
} // namespace ocf
