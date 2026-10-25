// SPDX-License-Identifier: MIT
#include "ocf/rhi/vulkan/VulkanPlatform.h"

#include "ocf/rhi/vulkan/platform/VulkanPlatformLinux.h"
#include "ocf/rhi/vulkan/platform/VulkanPlatformWindows.h"

#include "ocf/platform/platform.h"

namespace ocf::rhi {

std::unique_ptr<VulkanPlatform> VulkanPlatform::create()
{
#if OCF_TARGET_PLATFORM == OCF_PLATFORM_WIN32
    return std::make_unique<VulkanPlatformWindows>();
#elif OCF_TARGET_PLATFORM == OCF_PLATFORM_LINUX
    return std::make_unique<VulkanPlatformLinux>();
#else
    return std::unique_ptr<VulkanPlatform>();
#endif
}

} // namespace ocf::rhi
