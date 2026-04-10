// SPDX-License-Identifier: MIT

#pragma once

#include "VulkanUtility.h"

#include "ocf/rhi/Context.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <optional>
#include <memory>


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
    struct FrameContext {
        //std::shared_ptr<CommandBuffer> commandBuffer;
        VkFence inflightFence = VK_NULL_HANDLE;
    };

    VulkanContext();
    ~VulkanContext() override;

    bool initialize(const char* appName);

    void terminate();

    /** Get Vulkan instance */
    VkInstance getInstance() const noexcept { return m_instance; }

    /** Get Vulkan device */
    VkDevice getDevice() const noexcept { return m_device; }

    /** Get Vulkan physical device */
    VkPhysicalDevice getPhysicalDevice() const noexcept { return m_physicalDevice; }

    /** Get Vulkan surface */
    VkSurfaceKHR getSurface() const noexcept { return m_surface; }

    VulkanResult createSwapchain(Window* window, uint32_t width, uint32_t height);

    VulkanResult acquireNextImage();

    void submitPresent();

    FrameContext* getCurrentFrameContext();

private:
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
    };

    VulkanResult createInstance(const char* appName);

    VulkanResult createDebugMessenger();

    VulkanResult createLogicalDevice();

    VulkanResult createCommandPool();

    VulkanResult createSurface(Window* window);

    void setDebugObjectName(void* objectHandle, VkObjectType type, const char* name);

    VulkanResult pickPhysicalDevice();

    bool isDeviceSuitable(VkPhysicalDevice device);

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceMemoryProperties m_memoryProperties{};
    VkPhysicalDeviceProperties m_deviceProperties{};
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    PFN_vkSetDebugUtilsObjectNameEXT m_pfnSetDebugUtilsObjectNameEXT = nullptr;
    std::unique_ptr<VulkanSwapchain> m_swapchain;
};

} // namespace rhi
} // namespace ocf
