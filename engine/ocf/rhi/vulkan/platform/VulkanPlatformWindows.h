// SPDX-License-Identifier: MIT
#pragma once
#include "ocf/rhi/vulkan/VulkanPlatform.h"

namespace ocf::rhi {

class VulkanPlatformWindows final : public VulkanPlatform {
public:
    Result<VkSurfaceKHR, VulkanError> createSurface(VkInstance instance,
                                                    Window::NativeHandle nativeHandle) override;
};

} // namespace ocf::rhi
