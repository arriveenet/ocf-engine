// SPDX-License-Identifier: MIT
#include "ocf/rhi/vulkan/VulkanPlatform.h"

namespace ocf::rhi {

class VulkanPlatformLinux final : public VulkanPlatform {
public:
    Result<VkSurfaceKHR, VulkanError> createSurface(VkInstance instance,
                                                    Window::NativeHandle nativeHandle) override;

private:
    VkResult createXlibSurface(VkInstance instance, Window::NativeHandle nativeHandle, VkSurfaceKHR* surface);
    VkResult createWaylandSurface(VkInstance instance, Window::NativeHandle nativeHandle,  VkSurfaceKHR* surface);
};

} // namespace ocf::rhi
