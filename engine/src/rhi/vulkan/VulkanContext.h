// SPDX-License-Identifier: MIT

#pragma once

#include "VulkanCommandBuffer.h"
#include "VulkanUtility.h"

#include "ocf/rhi/Context.h"

#include <vulkan/vulkan.h>

namespace ocf {
class Window;

namespace rhi {

class VulkanSwapchain;

/**
 * @brief VulkanContext is responsible for managing the Vulkan instance, physical device, logical
 * device, and related resources.
 */
class VulkanContext : public Context {
public:
    VulkanContext();
    ~VulkanContext() override;

    bool initialize(const char* appName);

    void terminate();

    /** Get Vulkan instance */
    VkInstance getInstance() const noexcept { return m_instance; }

    /** Get Vulkan physical device */
    VkPhysicalDevice getPhysicalDevice() const noexcept { return m_physicalDevice; }

    /** Get Vulkan surface */
    VkSurfaceKHR getSurface() const noexcept { return m_surface; }

    /** Set debug object name */
    void setDebugObjectName(VkDevice device, void* objectHandle, VkObjectType type,
                            const char* name);
    /** Create surface */
    VulkanResult createSurface(Window* window);

private:
    VulkanResult createInstance(const char* appName);

    VulkanResult createDebugMessenger();

    VulkanResult pickPhysicalDevice();

    bool isDeviceSuitable(VkPhysicalDevice device);

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    PFN_vkSetDebugUtilsObjectNameEXT m_pfnSetDebugUtilsObjectNameEXT = nullptr;
};

} // namespace rhi
} // namespace ocf
