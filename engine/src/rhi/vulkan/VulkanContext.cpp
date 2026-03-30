// SPDX - License - Identifier : MIT

#include "VulkanContext.h"

#include "ocf/core/Logger.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <vector>

namespace {

VKAPI_ATTR VkBool32 VKAPI_CALL
vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    OCF_LOG_DEBUG("Vulkan validation layer: {}", pCallbackData->pMessage);
    return VK_FALSE;
}

} // namespace

namespace ocf {
namespace rhi {

VulkanContext::VulkanContext()
    : m_instance(VK_NULL_HANDLE)
    , m_device(VK_NULL_HANDLE)
    , m_physicalDevice(VK_NULL_HANDLE)
    , m_memoryProperties{}
    , m_deviceProperties{}
    , m_graphicsQueue(VK_NULL_HANDLE)
    , m_commandPool(VK_NULL_HANDLE)
    , m_debugMessenger(VK_NULL_HANDLE)
    , m_pfnSetDebugUtilsObjectNameEXT(nullptr)
{
}

VulkanContext::~VulkanContext()
{
}

bool VulkanContext::initialize(const char* appName)
{
    // Create Vulkan instance
    if(!createInstance(appName)) {
        OCF_LOG_ERROR("Failed to create Vulkan instance");
        return false;
    }

    // Select physical device
    pickPhysicalDevice();

    // Create debug messenger
    createDebugMessenger();

    // Create logical device and queues
    if (!createLogicalDevice()) {
        OCF_LOG_ERROR("Failed to create logical device");
        return false;
    }

    // Create command pool
    if (!createCommandPool()) {
        OCF_LOG_ERROR("Failed to create command pool");
        return false;
    }

    OCF_LOG_INFO("Vulkan instance created successfully");
    return true;
}

void VulkanContext::terminate()
{
    // Wait for the device to be idle before destroying resources
    vkDeviceWaitIdle(m_device);

    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    m_commandPool = VK_NULL_HANDLE;

    if (m_debugMessenger != VK_NULL_HANDLE) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            m_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func) {
            func(m_instance, m_debugMessenger, nullptr);
        }
        m_debugMessenger = VK_NULL_HANDLE;
    }

    vkDestroyDevice(m_device, nullptr);
    vkDestroyInstance(m_instance, nullptr);
    m_device = VK_NULL_HANDLE;
    m_instance = VK_NULL_HANDLE;

    OCF_LOG_INFO("Vulkan instance terminated.");
}

bool VulkanContext::createInstance(const char* appName)
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
    OCF_LOG_DEBUG("Available Vulkan instance extensions:");
    for (const auto& extention : extensions) {
        OCF_LOG_DEBUG("    {} (version {})", extention.extensionName, extention.specVersion);
    }

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

    VkResult ret =  vkCreateInstance(&createInfo, nullptr, &m_instance);
    return ret == VK_SUCCESS;
}

bool VulkanContext::createDebugMessenger()
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

    if (vkCreateDebugUtilsMessenger != nullptr &&
        vkCreateDebugUtilsMessenger(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
        OCF_LOG_ERROR("Failed to set up debug messenger");
        return false;
    }

    m_pfnSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(
        m_instance, "vkSetDebugUtilsObjectNameEXT");

    return true;
}

bool VulkanContext::createLogicalDevice()
{
    const QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.enabledExtensionCount = uint32_t(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    createInfo.pNext = nullptr;
    createInfo.pEnabledFeatures = nullptr;


    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
        return false;
    }

    vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
    return true;
}

bool VulkanContext::createCommandPool()
{
    const QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

    VkCommandPoolCreateInfo commandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    };
    commandPoolCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &m_commandPool) !=
        VK_SUCCESS) {
        OCF_LOG_ERROR("Failed to create command pool");
        return false;
    }

    return true;
}

void VulkanContext::setDebugObjectName(void* objectHandle, VkObjectType type, const char* name)
{
#ifndef NDEBUG
    if (m_pfnSetDebugUtilsObjectNameEXT) {
        VkDebugUtilsObjectNameInfoEXT nameInfo{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType = type,
            .objectHandle = reinterpret_cast<uint64_t>(objectHandle),
            .pObjectName = name,
        };
        m_pfnSetDebugUtilsObjectNameEXT(m_device, &nameInfo);
    }
#endif
}

void VulkanContext::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        OCF_LOG_ERROR("Failed to find GPUs with Vulkan support");
        return;
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
        OCF_LOG_ERROR("Failed to find a suitable GPU");
        return;
    }

    // Get memory properties and device properties
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_memoryProperties);
    vkGetPhysicalDeviceProperties(m_physicalDevice, &m_deviceProperties);

    OCF_LOG_DEBUG("Selected GPU: {} (type: {})", m_deviceProperties.deviceName,
                  static_cast<int>(m_deviceProperties.deviceType));
}

bool VulkanContext::isDeviceSuitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = findQueueFamilies(device);
    return indices.graphicsFamily.has_value();
}

VulkanContext::QueueFamilyIndices VulkanContext::findQueueFamilies(VkPhysicalDevice device) const
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
            break;
        }
        i++;
    }

    return indices;
}

} // namespace rhi
} // namespace ocf
