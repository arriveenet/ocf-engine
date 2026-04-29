// SPDX-License-Identifier: MIT

#include "VulkanDevice.h"

#include "VulkanContext.h"
#include "VulkanCommandBuffer.h"
#include "VulkanSwapchain.h"

#include "ocf/core/Logger.h"

#include <fstream>
#include <thread>

namespace ocf {
namespace rhi {

VulkanDevice::VulkanDevice(const DeviceConfig& config, VulkanContext& context)
    : m_context(context)
    , m_handleAllocator("Handles", config.handlePoolSize)
{
}

VulkanDevice::~VulkanDevice()
{
    terminate();
}

bool VulkanDevice::initialize()
{
    // Create logical device and queues
    VulkanResult result = createLogicalDevice();
    if (!result) {
        VulkanUtility::logError(result.unwrapErr());
        return false;
    }

    // Create command pool
    result = createCommandPool();
    if (!result) {
        VulkanUtility::logError(result.unwrapErr());
        return false;
    }

    // Get memory properties and device properties
    VkPhysicalDevice physicalDevice = m_context.getPhysicalDevice();
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &m_memoryProperties);
    vkGetPhysicalDeviceProperties(physicalDevice, &m_deviceProperties);

    OCF_LOG_DEBUG("Selected GPU: {} (type: {})", m_deviceProperties.deviceName,
                  static_cast<int>(m_deviceProperties.deviceType));
    return true;
}

void VulkanDevice::terminate()
{
    // Wait for the device to be idle before destroying resources
    vkDeviceWaitIdle(m_device);

    destroyFrameContexts();

    if (m_swapchain) {
        m_swapchain->destroy();
        m_swapchain.reset();
    }

    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    m_commandPool = VK_NULL_HANDLE;

    vkDestroyDevice(m_device, nullptr);
    m_device = VK_NULL_HANDLE;

    m_context.terminate();
}

TextureHandle VulkanDevice::createTexture()
{
    return TextureHandle();
}

ShaderModuleHandle VulkanDevice::createShaderModule(std::string_view filename)
{
    Handle<VulkanShaderModule> handle = initHandle<VulkanShaderModule>();

    VulkanShaderModule* shader = construct<VulkanShaderModule>(handle);

    std::ifstream file(filename.data(), std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        OCF_LOG_ERROR("Failded to open shader file: {}", filename.data());
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0).read(buffer.data(), fileSize);
    file.close();

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = buffer.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

    auto result = vkCreateShaderModule(m_device, &createInfo, nullptr, &shader->vk.id);
    if (result != VK_SUCCESS) {
        OCF_LOG_ERROR("Failded to create shader module from: {}", filename.data());
    }
 
    return Handle<RHIShaderModule>{handle.getId()};
}

SwapchainHandle VulkanDevice::createSwapchain(Window* window, uint32_t width, uint32_t height)
{
    Handle<VulkanSwapchain> handle = m_handleAllocator.allocate<Handle<VulkanSwapchain>>();

    if (m_swapchain == nullptr) {
        m_swapchain = std::make_unique<VulkanSwapchain>(*this);
    }

    if (m_context.getSurface() == VK_NULL_HANDLE) {
        VulkanResult result = m_context.createSurface(window);
        if (!result) {
            return Handle<RHISwapchain>();
        }
    }

    m_swapchain->create(width, height);

    destroyFrameContexts();
    createFrameContexts();

    // TODO: Handle create result
    return Handle<RHISwapchain>{handle.getId()};
}

std::shared_ptr<CommandBuffer> VulkanDevice::getCommandBuffer()
{
    auto* frameContext = getCurrentFrameContext();
    return frameContext->commandBuffer;
}

void VulkanDevice::beginFrame()
{
    if (acquireNextImage().isErr()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return;
    }
}

void VulkanDevice::endFrame()
{
    submitPresent();
}

VulkanResult VulkanDevice::createLogicalDevice()
{
    VkPhysicalDevice physicalDevice = m_context.getPhysicalDevice();
    const QueueFamilyIndices indices = VulkanUtility::findQueueFamilies(physicalDevice);

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

    VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &m_device);
    if (result != VK_SUCCESS) {
        return VulkanResult::Err(VK_ERROR(result, "Failed to create logical device"));
    }

    vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);

    return VulkanResult::Ok();
}

VulkanResult VulkanDevice::createCommandPool()
{
    const QueueFamilyIndices indices =
        VulkanUtility::findQueueFamilies(m_context.getPhysicalDevice());

    VkCommandPoolCreateInfo commandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    };
    commandPoolCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkResult result =
        vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &m_commandPool);

    if (result != VK_SUCCESS) {
        return VulkanResult::Err(VK_ERROR(result, "Failed to create command pool"));
    }

    return VulkanResult::Ok();
}

void VulkanDevice::setDebugObjectName(void* objectHandle, VkObjectType type, const char* name)
{
    m_context.setDebugObjectName(m_device, objectHandle, type, name);
}

VulkanResult VulkanDevice::acquireNextImage()
{
    auto* frame = getCurrentFrameContext();
    auto fence = frame->inFlightFence;
    vkWaitForFences(m_device, 1, &fence, VK_TRUE, UINT64_MAX);

    auto result = m_swapchain->acquireNextImage();
    if (result.isOk()) {
        vkResetFences(m_device, 1, &fence);
    }
    else {
        auto& error = result.unwrapErr();
        if (error.result == VK_ERROR_OUT_OF_DATE_KHR) {
            // TODO
        }
        assert(error.result != VK_ERROR_DEVICE_LOST);
    }

    return VulkanResult::Ok();
}

void VulkanDevice::submitPresent()
{
    auto& frame = m_frameContext[getCurrentFrameIndex()];

    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    };
    // Get current frame semaphore
    VkSemaphore renderCompleteSemaphore = m_swapchain->getRenderCompleteSemaphore();
    VkSemaphore presentComplateSemaphore = m_swapchain->getPresentCompleteSemaphore();

    VkCommandBuffer commandBuffer = frame.commandBuffer->getHandle();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.pWaitDstStageMask = &waitStageMask;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &presentComplateSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderCompleteSemaphore;
    auto result = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, frame.inFlightFence);
    assert(result != VK_ERROR_DEVICE_LOST);

    m_swapchain->queuePresent(m_graphicsQueue);
    advanceFrame();
}

std::shared_ptr<VulkanCommandBuffer> VulkanDevice::createCommandBuffer()
{
    VkCommandBufferAllocateInfo commandAI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &commandAI, &commandBuffer);

    return std::make_shared<VulkanCommandBuffer>(*this, commandBuffer);
}

void VulkanDevice::createFrameContexts()
{
    m_frameContext.resize(FRAMES_IN_FLIGHT_MAX);
    for (auto& frame : m_frameContext) {
        frame.commandBuffer = createCommandBuffer();
        VkFenceCreateInfo fenceCI{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT};
        vkCreateFence(m_device, &fenceCI, nullptr, &frame.inFlightFence);
    }
}

void VulkanDevice::destroyFrameContexts()
{
    for (auto& frame : m_frameContext) {
        vkDestroyFence(m_device, frame.inFlightFence, nullptr);
    }
    m_frameContext.clear();
}

VulkanDevice::FrameContext* VulkanDevice::getCurrentFrameContext()
{
    return &m_frameContext[m_currentFrameIndex];
}

void VulkanDevice::advanceFrame()
{
    m_currentFrameIndex = (m_currentFrameIndex + 1) % FRAMES_IN_FLIGHT_MAX;
}

} // namespace rhi
} // namespace ocf
