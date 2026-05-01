// SPDX-License-Identifier: MIT

#include "VulkanDevice.h"

#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanCommandBuffer.h"
#include "VulkanSwapchain.h"
#include "VulkanPipelineBuilder.h"

#include "ocf/core/Logger.h"

#include <string.h>

#include <fstream>
#include <thread>

namespace {

template <typename T>
void buildExtensionChain(T& last)
{
    last.pNext = nullptr;
}

template <typename T, typename U, typename... Rest>
void buildExtensionChain(T& current, U& next, Rest&... rest)
{
    current.pNext = &next;
    buildExtensionChain(next, rest...);
}

} // namespace

namespace ocf {
namespace rhi {

VkPhysicalDeviceMemoryProperties VulkanDevice::s_memoryProperties = {};

uint32_t VulkanDevice::findMemoryType(const VkMemoryRequirements& requirements,
                                      VkMemoryPropertyFlags properties)
{
    for (uint32_t i = 0; i < s_memoryProperties.memoryTypeCount; i++) {
        const bool isTypeCompatible = (requirements.memoryTypeBits & (1 << i)) != 0;
        const bool hasDesiredProperties =
            (s_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties;

        if (isTypeCompatible && hasDesiredProperties) {
            return i;
        }
    }

    OCF_LOG_FATAL("Faild to find suitable memory type");
    return 0;
}

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
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &s_memoryProperties);
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

    if (m_swapchain != nullptr) {
        m_swapchain->destroy();
        m_swapchain = nullptr;
    }

    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    m_commandPool = VK_NULL_HANDLE;

    vkDestroyDevice(m_device, nullptr);
    m_device = VK_NULL_HANDLE;

    m_context.terminate();
}

VertexBufferInfoHandle VulkanDevice::createVertexBufferInfo(uint8_t attributeCount,
                                                            AttributeArray attributes)
{
    Handle<VulkanVertexBufferInfo> handle = initHandle<VulkanVertexBufferInfo>();
    construct<VulkanVertexBufferInfo>(handle, attributeCount, attributes);
    return VertexBufferInfoHandle{handle.getId()};
}

VertexBufferHandle VulkanDevice::createVertexBuffer(uint32_t bufferSize, BufferUsage usage,
                                                    VertexBufferInfoHandle vbih)
{
    Handle<VulkanVertexBuffer> handle = initHandle<VulkanVertexBuffer>(m_device, vbih);
    VulkanVertexBuffer* vb = handle_cast<VulkanVertexBuffer*>(handle);

    if (!vb->initialize(bufferSize, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        OCF_LOG_ERROR("Failed to create vertex buffer");
        return Handle<VertexBufferHandle>{HandleBase::nullid};
    }

    return Handle<VertexBufferHandle>{handle.getId()};
}

TextureHandle VulkanDevice::createTexture()
{
    return TextureHandle();
}

ShaderModuleHandle VulkanDevice::createShaderModule(ShaderStage stage, std::string_view filename,
                                                    const char* entryPoint)
{
    Handle<VulkanShaderModule> handle = initHandle<VulkanShaderModule>();

    VulkanShaderModule* shader = construct<VulkanShaderModule>(handle);

    shader->stage = stage;
    shader->entryPoint = entryPoint;

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

PipelineHandle VulkanDevice::createPipeline(const PipelineState& state)
{
    Handle<VulkanPipeline> handle = initHandle<VulkanPipeline>();
    VulkanPipeline* pipeline = handle_cast<VulkanPipeline*>(handle);

    // Create pipeline layout
    VkPipelineLayoutCreateInfo layoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    };
    VkPipelineLayout pipelineLayout;
    auto result = vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &pipelineLayout);
    if (result != VK_SUCCESS) {
        OCF_LOG_ERROR("Failed to create pipeline layout");

    }
    pipeline->vk.layout = pipelineLayout;

    VulkanShaderModule* vs = handle_cast<VulkanShaderModule*>(state.vertexShader);
    VulkanShaderModule* fs = handle_cast<VulkanShaderModule*>(state.fragmentShader);

    VulkanVertexBufferInfo* vbi = handle_cast<VulkanVertexBufferInfo*>(state.vertexBufferInfo);
    uint32_t stride = 0;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    for (size_t i = 0, n = vbi->attributes.size(); i < n; i++) {
        const auto& attribute = vbi->attributes[i];
        const uint8_t buffer = attribute.buffer;
        if (buffer != Attribute::BUFFER_UNUSED) {
            VkVertexInputAttributeDescription attributeDescription{
                .location = uint32_t(i),
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = attribute.offset
            };
            attributeDescriptions.push_back(attributeDescription);

            stride = attribute.stride;
        }
    }

    std::vector<VkVertexInputBindingDescription> bindingDescriptions;
    VkVertexInputBindingDescription bindingDescription{
        .binding = 0,
        .stride = stride,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
    bindingDescriptions.push_back(bindingDescription);

    auto swapchainExtent = m_swapchain->getExtent();
    VkRect2D scissor = {
        .offset{0, 0},
        .extent = swapchainExtent,
    };
    VkViewport viewport = {
        .x = 0, .y = 0,
        .width = float(swapchainExtent.width),
        .height = float(swapchainExtent.height),
        .minDepth = 0.0f, .maxDepth = 1.0f
    };

    // Build pipeline
    VulkanPipelineBuilder builder{};
    builder.addShaderStage(VK_SHADER_STAGE_VERTEX_BIT, vs->vk.id, vs->entryPoint);
    builder.addShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, fs->vk.id, fs->entryPoint);
    builder.setVertexInput(bindingDescriptions.data(), uint32_t(bindingDescriptions.size()),
                           attributeDescriptions.data(), uint32_t(attributeDescriptions.size()));
    builder.setViewport(viewport, scissor);
    builder.setPipelineLayout(pipelineLayout);
    builder.useDynamicRendering(m_swapchain->getImageFormat().format);
    pipeline->vk.pipeline = builder.build(m_device);

    vkDestroyShaderModule(m_device, vs->vk.id, nullptr);
    vkDestroyShaderModule(m_device, fs->vk.id, nullptr);

    return Handle<RHIPipeline>{handle.getId()};
}

SwapchainHandle VulkanDevice::createSwapchain(Window* window, uint32_t width, uint32_t height)
{
    Handle<VulkanSwapchain> handle = initHandle<VulkanSwapchain>(*this);
    VulkanSwapchain* swapchain = handle_cast<VulkanSwapchain*>(handle);

    if (m_swapchain == nullptr) {
        m_swapchain = swapchain;
    }

    if (m_context.getSurface() == VK_NULL_HANDLE) {
        VulkanResult result = m_context.createSurface(window);
        if (!result) {
            return Handle<RHISwapchain>();
        }
    }

    swapchain->create(width, height);

    destroyFrameContexts();
    createFrameContexts();

    return Handle<RHISwapchain>{handle.getId()};
}

void VulkanDevice::destroyVertexBufferInfo(VertexBufferInfoHandle handle)
{
    if (handle) {
        VulkanVertexBufferInfo* vbi = handle_cast<VulkanVertexBufferInfo*>(handle);
        destruct(handle, vbi);
    }
}

void VulkanDevice::destroyVertexBuffer(VertexBufferHandle handle)
{
    if (handle) {
        VulkanVertexBuffer* vb = handle_cast<VulkanVertexBuffer*>(handle);
        destruct(handle, vb);
    }
}

void VulkanDevice::destroyPipeline(PipelineHandle handle)
{
    if (handle) {
        VulkanPipeline* pipeline = handle_cast<VulkanPipeline*>(handle);
        if (pipeline == nullptr) {
            return;
        }

        vkDeviceWaitIdle(m_device);

        if (pipeline->vk.pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(m_device, pipeline->vk.pipeline, nullptr);
            pipeline->vk.pipeline = VK_NULL_HANDLE;
        }
        if (pipeline->vk.layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_device, pipeline->vk.layout, nullptr);
            pipeline->vk.layout = VK_NULL_HANDLE;
        }
    }
}

void VulkanDevice::updateBufferData(VertexBufferHandle handle, const void* data, size_t size,
                                    size_t offset)
{
    VulkanVertexBuffer* vb = handle_cast<VulkanVertexBuffer*>(handle);
    void* p = vb->map();
    memcpy(p, data, size);
    vb->unmap();
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

    // Extension settings
    buildFeatures();
    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.enabledExtensionCount = uint32_t(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    createInfo.pNext = &m_physicalDeviceFeatures;
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

void VulkanDevice::buildFeatures()
{
    buildExtensionChain(m_physicalDeviceFeatures, 
                        m_vulkan11Features, m_vulkan12Features, m_vulkan13Features);

    vkGetPhysicalDeviceFeatures2(m_context.getPhysicalDevice(), &m_physicalDeviceFeatures);

    m_vulkan13Features.dynamicRendering = VK_TRUE;
    m_vulkan13Features.synchronization2 = VK_TRUE;
}

} // namespace rhi
} // namespace ocf
