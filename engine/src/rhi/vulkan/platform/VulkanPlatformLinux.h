#include "../VulkanPlatform.h"

namespace ocf::rhi {

class VulkanPlatformLinux final : public VulkanPlatform {
public:
    Result<VkSurfaceKHR, VulkanError> createSurface(VkInstance instance,
                                                    Window::NativeHandle nativeHandle) override;

private:
    VkSurfaceKHR createXlibSurface(VkInstance instance, Window::NativeHandle nativeHandle);
    VkSurfaceKHR createWaylandSurface(VkInstance instance, Window::NativeHandle nativeHandle);
};

} // namespace ocf::rhi
