#pragma once

#include "VulkanUtility.h"

#include "ocf/core/Result.h"
#include "ocf/platform/Window.h"

#include <vulkan/vulkan.h>

#include <memory>

namespace ocf::rhi {

class VulkanPlatform {
public:
    static std::unique_ptr<VulkanPlatform> create();

    virtual Result<VkSurfaceKHR, VulkanError> createSurface(VkInstance instance,
                                                            Window::NativeHandle nativeHandle) = 0;
};

} // namespace ocf::rhi
