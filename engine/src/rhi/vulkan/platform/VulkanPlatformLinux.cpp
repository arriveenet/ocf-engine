// SPDX-License-Identifier: MIT
#include "VulkanPlatformLinux.h"

#include "ocf/platform/Window.h"

#include <X11/Xlib.h>
#include <wayland-client.h>

#define VK_USE_PLATFORM_XLIB_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xlib.h>
#include <vulkan/vulkan_wayland.h>

namespace ocf::rhi {

Result<VkSurfaceKHR, VulkanError>
VulkanPlatformLinux::createSurface(VkInstance instance, Window::NativeHandle nativeHandle)
{
    VkResult result = VK_ERROR_INITIALIZATION_FAILED;

    if (nativeHandle.platform == Window::Platform::X11) {
        VkSurfaceKHR surface;
        result = createXlibSurface(instance, nativeHandle, &surface);
        if (result == VK_SUCCESS) {
            return Result<VkSurfaceKHR, VulkanError>::Ok(surface);
        }
    } else if (nativeHandle.platform == Window::Platform::Wayland) {
        VkSurfaceKHR surface;
        result = createWaylandSurface(instance, nativeHandle, &surface);
        if (result == VK_SUCCESS) {
            return Result<VkSurfaceKHR, VulkanError>::Ok(surface);
        }
    }

    return Result<VkSurfaceKHR, VulkanError>::Err(
        VulkanError(result, "Failed to create Linux surface"));
}

VkResult VulkanPlatformLinux::createXlibSurface(VkInstance instance, Window::NativeHandle nativeHandle, VkSurfaceKHR* surface)
{
    VkXlibSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    createInfo.dpy = static_cast<Display*>(nativeHandle.x11.display);
    createInfo.window = static_cast<::Window>(nativeHandle.x11.window);
    return vkCreateXlibSurfaceKHR(instance, &createInfo, nullptr, surface);
}

VkResult VulkanPlatformLinux::createWaylandSurface(VkInstance instance, Window::NativeHandle nativeHandle, VkSurfaceKHR* surface)
{
    VkWaylandSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    createInfo.display = static_cast<wl_display*>(nativeHandle.wayland.display);
    createInfo.surface = static_cast<wl_surface*>(nativeHandle.wayland.surface);
    return vkCreateWaylandSurfaceKHR(instance, &createInfo, nullptr, surface);
}

} // namespace ocf::rhi
