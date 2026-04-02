// SPDX - License - Identifier : MIT

#pragma once

#include "VulkanUtility.h"

#include "ocf/rhi/Context.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <optional>


namespace ocf {
namespace rhi {

class VulkanContext : public Context {
public:
    VulkanContext();
    ~VulkanContext() override;

    bool initialize(const char* appName);

    void terminate();

    VkInstance getInstance() const noexcept { return m_instance; }
    VkDevice getDevice() const noexcept { return m_device; }

private:
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
    };

    VulkanResult createInstance(const char* appName);

    VulkanResult createDebugMessenger();

    VulkanResult createLogicalDevice();

    VulkanResult createCommandPool();

    void setDebugObjectName(void* objectHandle, VkObjectType type, const char* name);

    VulkanResult pickPhysicalDevice();

    bool isDeviceSuitable(VkPhysicalDevice device);

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;

private:
    VkInstance m_instance;
    VkDevice m_device;
    VkPhysicalDevice m_physicalDevice;
    VkPhysicalDeviceMemoryProperties m_memoryProperties;
    VkPhysicalDeviceProperties m_deviceProperties;
    VkQueue m_graphicsQueue;
    VkCommandPool m_commandPool;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    PFN_vkSetDebugUtilsObjectNameEXT m_pfnSetDebugUtilsObjectNameEXT;
};

} // namespace rhi
} // namespace ocf
