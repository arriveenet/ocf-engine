// SPDX-License-Identifier: MIT

#include "VulkanDevice.h"

#include "VulkanHandles.h"
#include "VulkanUtility.h"
#include "vulkan/resource/ResourceUploader.h"
#include "vulkan/resource/ImageResource.h"
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanPipelineBuilder.h"
#include "VulkanSwapchain.h"

#include "ocf/core/Logger.h"

#include <cstring>
#include <fstream>
#include <thread>
#include <vulkan/vulkan_core.h>

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

    // Create descriptor pool
    result = createDescriptorPool();
    if (!result) {
        VulkanUtility::logError(result.unwrapErr());
        return false;
    }

    // Initialize Resource Uploader
    m_resourceUploader = std::make_unique<ResourceUploader>();
    m_resourceUploader->initialize(*this);

    // Create sampler cache
    m_samplerCache = std::make_unique<SamplerCache>(m_device);

    // Get memory properties and device properties
    VkPhysicalDevice physicalDevice = m_context.getPhysicalDevice();
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &s_memoryProperties);
    vkGetPhysicalDeviceProperties(physicalDevice, &m_deviceProperties);

    OCF_LOG_INFO("Selected GPU: {} (type: {})", m_deviceProperties.deviceName,
                 VulkanUtility::getPhysicalDeviceTypeString(m_deviceProperties.deviceType));
    return true;
}

void VulkanDevice::terminate()
{
    // Wait for the device to be idle before destroying resources
    vkDeviceWaitIdle(m_device);

    if (m_depthBuffer) {
        m_depthBuffer->cleanup();
        m_depthBuffer.reset();
    }

    if (m_resourceUploader) {
        m_resourceUploader->cleanup();
        m_resourceUploader.reset();
    }

    if (m_samplerCache) {
        m_samplerCache.reset();
    }

    destroyFrameContexts();

    if (m_swapchain != nullptr) {
        m_swapchain->destroy();
        m_swapchain = nullptr;
    }

    vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

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

    if (!vb->initialize(bufferSize, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
        OCF_LOG_ERROR("Failed to create vertex buffer");
        return Handle<VertexBufferHandle>{HandleBase::nullid};
    }

    return Handle<VertexBufferHandle>{handle.getId()};
}

IndexBufferHandle VulkanDevice::createIndexBuffer(ElementType elementType, uint32_t indexCount,
                                                  BufferUsage usage)
{
    Handle<VulkanIndexBuffer> handle = initHandle<VulkanIndexBuffer>(m_device);
    VulkanIndexBuffer* ib = handle_cast<VulkanIndexBuffer*>(handle);

    const uint8_t elementSize = static_cast<uint8_t>(getElementTypeSize(elementType));
    const VkDeviceSize size = elementSize * static_cast<VkDeviceSize>(indexCount);

    ib->count = indexCount;
    ib->elementSize = elementSize;

    if (!ib->initialize(size, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
        OCF_LOG_ERROR("Failed to create index buffer");
        return Handle<IndexBufferHandle>{HandleBase::nullid};
    }

    return Handle<IndexBufferHandle>{handle.getId()};
}

BufferObjectHandle VulkanDevice::createBufferObject(BufferType type, uint32_t byteCount)
{
    Handle<VulkanBufferObject> handle = initHandle<VulkanBufferObject>();
    VulkanBufferObject* bo = handle_cast<VulkanBufferObject*>(handle);

    if (type == BufferType::Uniform) {
        bo->buffer = UniformBuffer::create(m_device, byteCount);
    }

    return Handle<BufferObjectHandle>{handle.getId()};
}

TextureHandle VulkanDevice::createTexture(SamplerType target, uint8_t levels, TextureFormat format,
                                          uint32_t width, uint32_t height, uint32_t depth)
{
    Handle<VulkanTexture> handle = initHandle<VulkanTexture>();
    VulkanTexture* tex = handle_cast<VulkanTexture*>(handle);

    VkExtent2D extent{
        .width = uint32_t(width),
        .height = uint32_t(height)
    };

    // TODO
    const VkFormat vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
    const uint32_t mipLevels = 1;

    // Set the destination texture
    tex->texture = Texture2D::create(m_device, extent, vkFormat, mipLevels);

    return Handle<TextureHandle>{handle.getId()};
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

    VkResult result = vkCreateShaderModule(m_device, &createInfo, nullptr, &shader->vk.id);
    VK_CHECK_RESULT(result);
 
    return Handle<RHIShaderModule>{handle.getId()};
}

DescriptorSetLayoutHandle VulkanDevice::createDescriptorLayoutSet(const DescriptorSetLayout& layout)
{
    Handle<VulkanDescriptorSetLayout> handle = initHandle<VulkanDescriptorSetLayout>();
    VulkanDescriptorSetLayout* dsl = construct<VulkanDescriptorSetLayout>(handle);

    dsl->layout = layout;

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    for (const auto& descriptor : layout.descriptors) {
        VkDescriptorSetLayoutBinding binding
        {
            .binding = descriptor.binding,
            .descriptorType = VulkanUtility::getDescriptorType(descriptor.type),
            .descriptorCount = 1,
            .stageFlags = VulkanUtility::getShaderStageFlagBits(descriptor.shaderStageFlags),
            .pImmutableSamplers = nullptr,
        };
        bindings.push_back(binding);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = uint32_t(bindings.size()),
        .pBindings = bindings.data(),
    };

    VkResult result = vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &dsl->vk.id);
    VK_CHECK_RESULT(result);

    return Handle<RHIDescriptorSetLayout>{handle.getId()};
}

DescriptorSetHandle VulkanDevice::createDescriptorSet(DescriptorSetLayoutHandle dslh)
{
    Handle<VulkanDescriptorSet> handle = initHandle<VulkanDescriptorSet>();
    VulkanDescriptorSet* ds = handle_cast<VulkanDescriptorSet*>(handle);

    VulkanDescriptorSetLayout* dsl = handle_cast<VulkanDescriptorSetLayout*>(dslh);

    ds->vk.id = allocateDescriptorSet(dsl->vk.id);


    return Handle<RHIDescriptorSet>{handle.getId()};
}

PipelineHandle VulkanDevice::createPipeline(const PipelineState& state)
{
    Handle<VulkanPipeline> handle = initHandle<VulkanPipeline>();
    VulkanPipeline* pipeline = handle_cast<VulkanPipeline*>(handle);
    VulkanDescriptorSetLayout* dsl = handle_cast<VulkanDescriptorSetLayout*>(state.layout);

    // Create pipeline layout
    VkPipelineLayoutCreateInfo layoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &dsl->vk.id,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
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
    VkExtent2D viewport = {
        .width = swapchainExtent.width,
        .height = swapchainExtent.height,
    };

    // Depth buffer settings
    VkPipelineDepthStencilStateCreateInfo depthStencilState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VulkanUtility::getDepthFunc(state.rasterState.depthFunc),
    };

    // Culling settings
    VkPipelineRasterizationStateCreateInfo rasterizerState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VulkanUtility::getCullMode(state.rasterState.culling),
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f,
    };

    // Format
    auto colorFormat = m_swapchain->getImageFormat().format;
    auto depthFormat = m_depthBuffer ? m_depthBuffer->getFormat() : VK_FORMAT_UNDEFINED;

    // Build pipeline
    VulkanPipelineBuilder builder{};
    builder.addShaderStage(VK_SHADER_STAGE_VERTEX_BIT, vs->vk.id, vs->entryPoint);
    builder.addShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, fs->vk.id, fs->entryPoint);
    builder.setVertexInput(bindingDescriptions.data(), uint32_t(bindingDescriptions.size()),
                           attributeDescriptions.data(), uint32_t(attributeDescriptions.size()));
    builder.setViewport(viewport);
    builder.setPipelineLayout(pipelineLayout);
    builder.setDepthStencilState(depthStencilState);
    builder.setRasterizationState(rasterizerState);
    builder.useDynamicRendering(colorFormat, depthFormat);
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

void VulkanDevice::createDepthBuffer(uint32_t width, uint32_t height)
{
    VkExtent2D extent {
        .width = width,
        .height = height,
    };

    m_depthBuffer = DepthBuffer::create(m_device, extent, VK_FORMAT_D32_SFLOAT);
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

void VulkanDevice::destroyIndexBuffer(IndexBufferHandle handle)
{
    if (handle) {
        VulkanIndexBuffer* ib = handle_cast<VulkanIndexBuffer*>(handle);
        destruct(handle, ib);
    }
}

void VulkanDevice::destroyBufferObject(BufferObjectHandle handle)
{
    if (!handle) {
        return;
    }

    VulkanBufferObject* bo = handle_cast<VulkanBufferObject*>(handle);
    destruct(handle, bo);
}

void VulkanDevice::destroyDescriptorSetLayout(DescriptorSetLayoutHandle handle)
{
    if (!handle) {
        return;
    }

    VulkanDescriptorSetLayout* dsl = handle_cast<VulkanDescriptorSetLayout*>(handle);
    vkDestroyDescriptorSetLayout(m_device, dsl->vk.id, nullptr);

    destruct(handle, dsl);
}

void VulkanDevice::destroyDescriptorSet(DescriptorSetHandle handle)
{
    if (!handle) {
        return;
    }

    VulkanDescriptorSet* ds = handle_cast<VulkanDescriptorSet*>(handle);
    freeDescriptorSet(ds->vk.id);

    destruct(handle, ds);
}

void VulkanDevice::destroyPipeline(PipelineHandle handle)
{
    if (!handle) {
        return;
    }

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

void VulkanDevice::updateBufferData(VertexBufferHandle handle, const void* data, size_t size,
                                    size_t offset)
{
    if (!handle) {
        return;
    }

    VulkanVertexBuffer* vb = handle_cast<VulkanVertexBuffer*>(handle);
    m_resourceUploader->uploadBuffer(vb, data, size, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
}

void VulkanDevice::updateIndexBufferData(IndexBufferHandle handle, const void* data, size_t size,
                                         size_t offset)
{
    if (!handle) {
        return;
    }

    VulkanIndexBuffer* ib = handle_cast<VulkanIndexBuffer*>(handle);
    m_resourceUploader->uploadBuffer(ib, data, size, VK_ACCESS_INDEX_READ_BIT);
}

void VulkanDevice::updateBufferObject(BufferObjectHandle handle, const void* data, size_t size,
                                      size_t offset)
{
    if(!handle){
        return;
    }

    VulkanBufferObject* bufferObject = handle_cast<VulkanBufferObject*>(handle);
    if(void* p = bufferObject->buffer->map(); p != nullptr) {
        memcpy(p, data, size);
        bufferObject->buffer->unmap();
    }
}

void VulkanDevice::updateDescriptorSet(DescriptorSetHandle handle, BufferObjectHandle buffer,
                                       size_t offset)
{
    if (!handle || !buffer) {
        return;
    }

    VulkanDescriptorSet* descriptorSet = handle_cast<VulkanDescriptorSet*>(handle);
    VulkanBufferObject* bufferObject = handle_cast<VulkanBufferObject*>(buffer);

    VkDescriptorBufferInfo bufferInfo{
        .buffer = bufferObject->buffer->getBuffer(),
        .offset = 0,
        .range = bufferObject->buffer->getBufferSize(),
    };

    std::vector<VkWriteDescriptorSet> writes{
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet->vk.id,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &bufferInfo,
        },
    };

    vkUpdateDescriptorSets(m_device, uint32_t(writes.size()), writes.data(), 0, nullptr);
}

void VulkanDevice::updateTextureImage(TextureHandle handle, uint8_t level, uint32_t xoffset,
                                      uint32_t yoffset, uint32_t zoffset, uint32_t width,
                                      uint32_t height, uint32_t depth, PixelBufferDescriptor&& data)
{
    if (!handle) {
        return;
    }

    VulkanTexture* tex = handle_cast<VulkanTexture*>(handle);

    VkExtent2D extent{
        .width = width,
        .height = height,
    };

    // Create staging buffer
    auto staging = StagingBuffer::create(m_device, data.size);
    memcpy(staging->map(), data.getBuffer(), data.size);
    staging->unmap();

    // Set transfer region
    VkBufferImageCopy region{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        .imageOffset = {0, 0, 0},
        .imageExtent = {.width = extent.width, .height = extent.height, .depth = 1}};

    TextureUploadRequest request{.staging = staging,
                                 .copyRegions = {region},
                                 .nextAccessFlags = VK_ACCESS_SHADER_READ_BIT,
                                 .nextLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                 .nextStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT};

    m_resourceUploader->uploadImage(tex->texture, request);
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

    m_resourceUploader->submitAndWait();
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
        // Enable the extension to switch to a right-handed coordinate system
        VK_KHR_MAINTENANCE1_EXTENSION_NAME,
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

VulkanResult VulkanDevice::createDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> poolSize = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 0x1000,
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 0x1000,
        }
    };
    VkDescriptorPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 0x1000,
        .poolSizeCount = uint32_t(poolSize.size()),
        .pPoolSizes = poolSize.data()
    };

    VkResult result = vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool);
    if (result != VK_SUCCESS) {
        return VulkanResult::Err(VK_ERROR(result, "Failed to create descriptor pool"));
    }

    return VulkanResult::Ok();
}

VkDescriptorSet VulkanDevice::allocateDescriptorSet(VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    VkDescriptorSet descriptorSet;
    if (vkAllocateDescriptorSets(m_device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        OCF_LOG_ERROR("Failed to allocate descriptor set");
    }
    return descriptorSet;
}

void VulkanDevice::freeDescriptorSet(VkDescriptorSet descriptorSet)
{
    vkFreeDescriptorSets(m_device, m_descriptorPool, 1, &descriptorSet);
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
    m_frameContext.resize(MAX_FRAMES_IN_FLIGHT);
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
    m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
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
