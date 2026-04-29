// SPDX-License-Identifier: MIT

#include "VulkanContext.h"

#include "VulkanPlatform.h"
#include "VulkanUtility.h"
#include "VulkanSwapchain.h"

#include "ocf/core/Logger.h"
#include "ocf/platform/Window.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <vector>

namespace {

VKAPI_ATTR VkBool32 VKAPI_CALL
vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                    VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* /* pUserData */)
{
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        OCF_LOG_ERROR("Vulkan validation layer: {}", pCallbackData->pMessage);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        OCF_LOG_WARN("Vulkan validation layer: {}", pCallbackData->pMessage);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        OCF_LOG_INFO("Vulkan validation layer: {}", pCallbackData->pMessage);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        OCF_LOG_DEBUG("Vulkan validation layer: {}", pCallbackData->pMessage);
    }
    return VK_FALSE;
}

} // namespace

namespace ocf {
namespace rhi {

VulkanContext::VulkanContext()
{
}

VulkanContext::~VulkanContext()
{
}

bool VulkanContext::initialize(const char* appName)
{
    // Create Vulkan instance
    VulkanResult result = createInstance(appName);
    if (!result) {
        VulkanUtility::logError(result.unwrapErr());
        return false;
    }

    // Select physical device
    result = pickPhysicalDevice();
    if (!result) {
        VulkanUtility::logError(result.unwrapErr());
        return false;
    }

    // Create debug messenger
    result = createDebugMessenger();
    if (!result) {
        VulkanUtility::logError(result.unwrapErr());
        return false;
    }

    OCF_LOG_INFO("Vulkan instance created successfully");
    return true;
}

void VulkanContext::terminate()
{
    if (m_debugMessenger != VK_NULL_HANDLE) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            m_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func) {
            func(m_instance, m_debugMessenger, nullptr);
        }
        m_debugMessenger = VK_NULL_HANDLE;
    }

    if (m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }

    vkDestroyInstance(m_instance, nullptr);
    m_instance = VK_NULL_HANDLE;

    OCF_LOG_INFO("Vulkan instance terminated.");
}

VulkanResult VulkanContext::createInstance(const char* appName)
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = appName;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "OcfEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
    //OCF_LOG_DEBUG("Available Vulkan instance extensions:");
    //for (const auto& extension : extensions) {
    //    OCF_LOG_DEBUG("    {} (version {})", extension.extensionName, extension.specVersion);
    //}

    std::vector<const char*> extensionList;
    std::vector<const char*> layerList;

#ifndef NDEBUG
    layerList.push_back("VK_LAYER_KHRONOS_validation");
    extensionList.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    // FIXME: This is a bit hacky, we should ideally query the required extensions from the window
    // system integration layer (e.g. GLFW) and add them to the list
    auto getWindowSystemExtensions = [](auto& extensionList) {
        uint32_t extensionCount = 0;
        const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
        if (extensionCount > 0) {
            extensionList.insert(extensionList.end(), extensions, extensions + extensionCount);
        }
    };
    getWindowSystemExtensions(extensionList);

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = uint32_t(extensionList.size());
    createInfo.ppEnabledExtensionNames = extensionList.data();
    createInfo.enabledLayerCount = uint32_t(layerList.size());
    createInfo.ppEnabledLayerNames = layerList.data();

    VkResult result =  vkCreateInstance(&createInfo, nullptr, &m_instance);
    if (result != VK_SUCCESS) {
        return VulkanResult::Err(VK_ERROR(result, "Failed to create Vulkan instance"));
    }

    return VulkanResult::Ok();
}

VulkanResult VulkanContext::createDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = vulkanDebugCallback;

    auto vkCreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        m_instance, "vkCreateDebugUtilsMessengerEXT");

    if (vkCreateDebugUtilsMessenger == nullptr) {
        return VulkanResult::Err(
            VK_ERROR(VK_ERROR_EXTENSION_NOT_PRESENT,
                     "Failed to get address of vkCreateDebugUtilsMessengerEXT"));

    }

    VkResult result =
        vkCreateDebugUtilsMessenger(m_instance, &createInfo, nullptr, &m_debugMessenger);
    if (result != VK_SUCCESS) {
        return VulkanResult::Err(VK_ERROR(result, "Failed to create debug utils messenger"));
    }

    m_pfnSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(
        m_instance, "vkSetDebugUtilsObjectNameEXT");

    return VulkanResult::Ok();
}

void VulkanContext::setDebugObjectName(VkDevice device, void* objectHandle, VkObjectType type,
                                       const char* name)
{
#ifndef NDEBUG
    if (m_pfnSetDebugUtilsObjectNameEXT) {
        VkDebugUtilsObjectNameInfoEXT nameInfo{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType = type,
            .objectHandle = reinterpret_cast<uint64_t>(objectHandle),
            .pObjectName = name,
        };
        m_pfnSetDebugUtilsObjectNameEXT(device, &nameInfo);
    }
#endif
}

VulkanResult VulkanContext::createSurface(Window* window)
{
    std::unique_ptr<VulkanPlatform> platform = VulkanPlatform::create();
    auto surface = platform->createSurface(m_instance, window->getNativeHandle());
    if (surface.isErr()) {
        return VulkanResult::Err(surface.unwrapErr());
    }

    m_surface = surface.unwrap();

    QueueFamilyIndices indices = VulkanUtility::findQueueFamilies(m_physicalDevice);
    VkBool32 presentSupport = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, indices.graphicsFamily.value(),
                                         m_surface, &presentSupport);
    if (presentSupport == VK_FALSE) {
        return VulkanResult::Err(
            VK_ERROR(VK_ERROR_INITIALIZATION_FAILED, "Selected GPU does not support presentation"));
    }

    return VulkanResult::Ok();
}

VulkanResult VulkanContext::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        return VulkanResult::Err(
            VK_ERROR(VK_ERROR_INITIALIZATION_FAILED, "Failed to find GPUs with Vulkan support"));
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
    
    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            m_physicalDevice = device;
            break;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE) {
        return VulkanResult::Err(
            VK_ERROR(VK_ERROR_INITIALIZATION_FAILED, "Failed to find a suitable GPU"));
    }

    return VulkanResult::Ok();
}

bool VulkanContext::isDeviceSuitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = VulkanUtility::findQueueFamilies(device);
    return indices.graphicsFamily.has_value();
}

} // namespace rhi
} // namespace ocf
