#include "VulkanPlatformLinux.h"

#include "ocf/platform/Window.h"

#define VK_USE_PLATFORM_XLIB_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xlib.h>
#include <vulkan/vulkan_wayland.h>

namespace ocf::rhi {

Result<VkSurfaceKHR, VulkanError>
VulkanPlatformLinux::createSurface(VkInstance instance, Window::NativeHandle nativeHandle)
{
    if (nativeHandle.platform == Window::Platform::Xlib) {
        VkSurfaceKHR surface = createXlibSurface(instance, nativeHandle);
        if (surface != VK_NULL_HANDLE) {
            return Result<VkSurfaceKHR, VulkanError>::Ok(surface);
        }
    } else if (nativeHandle.platform == Window::Platform::Wayland) {
        VkSurfaceKHR surface = createWaylandSurface(instance, nativeHandle);
        if (surface != VK_NULL_HANDLE) {
            return Result<VkSurfaceKHR, VulkanError>::Ok(surface);
        }
    }

    return Result<VkSurfaceKHR, VulkanError>::Err(
        VulkanError(result, "Failed to create Linux surface"));
}

VkSurfaceKHR createXlibSurface(VkInstance instance, Window::NativeHandle nativeHandle)
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    VkXlibSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    createInfo.dpy = static_cast<Display*>(nativeHandle.xlib.display);
    createInfo.window = static_cast<Window>(nativeHandle.xlib.window);
    return vkCreateXlibSurfaceKHR(instance, &createInfo, nullptr, &surface);
}

VkSurfaceKHR createWaylandSurface(VkInstance instance, Window::NativeHandle nativeHandle)
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    VkWaylandSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    createInfo.display = static_cast<wl_display*>(nativeHandle.wayland.display);
    createInfo.surface = static_cast<wl_surface*>(nativeHandle.wayland.surface);
    return vkCreateWaylandSurfaceKHR(instance, &createInfo, nullptr, &surface);
}

} // namespace ocf::rhi
