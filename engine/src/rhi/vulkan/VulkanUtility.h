#pragma once
#include <vulkan/vulkan.h>

namespace ocf {
namespace rhi {

namespace VulkanUtility {

void assertVkError(VkResult ret, const char* function, size_t line) noexcept;

#ifdef NDEBUG
#   define VK_CHECK_RESULT(ret)
#else
#   define VK_CHECK_RESULT(ret) { VulkanUtility::assertVkError(ret, __func__, __LINE__); }
#endif

} // namespace VulkanUtility

} // namespace rhi
} // namespace ocf
