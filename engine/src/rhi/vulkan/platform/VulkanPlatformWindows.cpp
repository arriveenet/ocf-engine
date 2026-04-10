// SPDX-License-Identifier: MIT
#include "VulkanPlatformWindows.h"

#include "ocf/platform/Window.h"

#include <Windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

namespace ocf::rhi {

Result<VkSurfaceKHR, VulkanError>
VulkanPlatformWindows::createSurface(VkInstance instance, Window::NativeHandle nativeHandle)
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = static_cast<HWND>(nativeHandle.win32.hWnd);
    createInfo.hinstance = static_cast<HINSTANCE>(nativeHandle.win32.hInstance);
    VkResult result = vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);
    if (result == VK_SUCCESS) {
        return Result<VkSurfaceKHR, VulkanError>::Ok(surface);
    }

    return Result<VkSurfaceKHR, VulkanError>::Err(
        VulkanError(result, "Failed to create Win32 surface"));
}

} // namespace ocf::rhi
