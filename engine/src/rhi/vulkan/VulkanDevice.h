// SPDX-License-Identifier: MIT

#pragma once

#include "rhi/HandleAllocator.h"
#include "VulkanUtility.h"
#include "VulkanHandles.h"
#include "resource/SamplerCache.h"

#include "ocf/rhi/Device.h"

#include <vulkan/vulkan.h>

#include <array>
#include <memory>
#include <vector>

namespace ocf::rhi {

class VulkanCommandBuffer;
class VulkanContext;
class VulkanSwapchain;
class ResourceUploader;
class DepthBuffer;

class VulkanDevice : public Device {
    friend class VulkanCommandBuffer;
    friend class ResourceUploader;

public:
    static const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    static uint32_t findMemoryType(const VkMemoryRequirements& requirements,
                                   VkMemoryPropertyFlags properties);

    VulkanDevice(const DeviceConfig& config, VulkanContext& context);
    ~VulkanDevice() override;

    bool initialize();
    void terminate();

    VertexBufferInfoHandle createVertexBufferInfo(uint8_t attributeCount,
                                                  AttributeArray attributes) override;

    VertexBufferHandle createVertexBuffer(uint32_t bufferSize, BufferUsage usage,
                                          VertexBufferInfoHandle vbih) override;

    IndexBufferHandle createIndexBuffer(ElementType elementType, uint32_t indexCount,
                                        BufferUsage usage) override;

    BufferObjectHandle createBufferObject(BufferType type, uint32_t byteCount) override;

    TextureHandle createTexture(SamplerType target, uint8_t levels, TextureFormat format,
                                uint32_t width, uint32_t height, uint32_t depth) override;

    ShaderModuleHandle createShaderModule(ShaderStage stage, std::string_view filename,
                                          const char* entryPoint = "main") override;

    DescriptorSetLayoutHandle createDescriptorLayoutSet(const DescriptorSetLayout& layout) override;

    DescriptorSetHandle createDescriptorSet(DescriptorSetLayoutHandle dslh) override;

    PipelineHandle createPipeline(const PipelineState& pipeline) override;

    SwapchainHandle createSwapchain(Window* window, uint32_t width, uint32_t height) override;

    void createDepthBuffer(uint32_t width, uint32_t height) override;

    void destroyVertexBufferInfo(VertexBufferInfoHandle handle) override;

    void destroyVertexBuffer(VertexBufferHandle handle) override;

    void destroyIndexBuffer(IndexBufferHandle handle) override;

    void destroyPipeline(PipelineHandle handle) override;

    void destroyBufferObject(BufferObjectHandle handle) override;

    void destroyDescriptorSetLayout(DescriptorSetLayoutHandle handle) override;

    void destroyDescriptorSet(DescriptorSetHandle handle) override;

    void updateBufferData(VertexBufferHandle handle, const void* data, size_t size,
                          size_t offset) override;

    void updateIndexBufferData(IndexBufferHandle handle, const void* data, size_t size,
                               size_t offset) override;

    void updateBufferObject(BufferObjectHandle handle, const void* data, size_t size,
                            size_t offset) override;

    void updateDescriptorSet(DescriptorSetHandle handle, BufferObjectHandle buffer,
                             size_t offset) override;

    void updateTextureImage(TextureHandle handle, uint8_t level, uint32_t xoffset, uint32_t yoffset,
                            uint32_t zoffset, uint32_t width, uint32_t height, uint32_t depth,
                            PixelBufferDescriptor&& data) override;

    std::shared_ptr<CommandBuffer> getCommandBuffer() override;

    uint32_t getCurrentFrameIndex() override  { return m_currentFrameIndex; }

    void beginFrame() override;

    void endFrame() override;

    VkDevice getDevice() const noexcept { return m_device; }

    VulkanContext& getContext() const noexcept { return m_context; }

    VkCommandPool getCommandPool() const noexcept { return m_commandPool; }

    VkQueue getGraphicsQueue() const noexcept { return m_graphicsQueue; }

    std::shared_ptr<DepthBuffer> getDepthBuffer() const noexcept { return m_depthBuffer; }

private:
    struct FrameContext {
        std::shared_ptr<VulkanCommandBuffer> commandBuffer;
        VkFence inFlightFence = VK_NULL_HANDLE;
    };

    template <typename D, typename... ARGS>
    Handle<D> initHandle(ARGS&&... args)
    {
        return m_handleAllocator.allocateAndConstruct<D>(std::forward<ARGS>(args)...);
    }

    template <typename D, typename B, typename... ARGS>
    std::enable_if_t<std::is_base_of_v<B, D>, D>* construct(Handle<B> const& handle, ARGS&&... args)
    {
        return m_handleAllocator.destroyAndConstruct<D, B>(handle, std::forward<ARGS>(args)...);
    }

    template <typename B, typename D, typename = std::enable_if_t<std::is_base_of_v<B, D>, D>>
    void destruct(Handle<B>& handle, D const* p) noexcept
    {
        m_handleAllocator.deallocate(handle, p);
    }

    template <typename Dp, typename B>
    std::enable_if_t<std::is_pointer_v<Dp> && std::is_base_of_v<B, std::remove_pointer_t<Dp>>, Dp>
    handle_cast(Handle<B>& handle)
    {
        return m_handleAllocator.handle_cast<Dp, B>(handle);
    }

    template <typename Dp, typename B>
    std::enable_if_t<std::is_pointer_v<Dp> && std::is_base_of_v<B, std::remove_pointer_t<Dp>>, Dp>
    handle_cast(const Handle<B>& handle)
    {
        return m_handleAllocator.handle_cast<Dp, B>(handle);
    }

    VulkanResult createLogicalDevice();

    VulkanResult createCommandPool();

    VulkanResult createDescriptorPool();

    VkDescriptorSet allocateDescriptorSet(VkDescriptorSetLayout layout);

    void freeDescriptorSet(VkDescriptorSet descriptorSet);

    void setDebugObjectName(void* objectHandle, VkObjectType type, const char* name);

    /** Switches the drawable swapchain image */
    VulkanResult acquireNextImage();

    /** Execute the command in the current frame context and publish the presentation */
    void submitPresent();

    VulkanSwapchain* getSwapchain() const noexcept { return m_swapchain; }

    std::shared_ptr<VulkanCommandBuffer> createCommandBuffer();

    void createFrameContexts();

    void destroyFrameContexts();

    FrameContext* getCurrentFrameContext();

    void advanceFrame();

    void buildFeatures();

private:
    static VkPhysicalDeviceMemoryProperties s_memoryProperties;

    VulkanContext& m_context;
    HandleAllocatorVK m_handleAllocator;

    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties m_deviceProperties{};
    VulkanSwapchain* m_swapchain = nullptr;
    std::vector<FrameContext> m_frameContext;
    uint32_t m_currentFrameIndex = 0;

    std::unique_ptr<ResourceUploader> m_resourceUploader;
    std::shared_ptr<DepthBuffer> m_depthBuffer;
    std::unique_ptr<SamplerCache> m_samplerCache;

    VkPhysicalDeviceFeatures2 m_physicalDeviceFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    VkPhysicalDeviceVulkan11Features m_vulkan11Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    VkPhysicalDeviceVulkan12Features m_vulkan12Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    VkPhysicalDeviceVulkan13Features m_vulkan13Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    VkPhysicalDeviceShaderAtomicFloatFeaturesEXT m_atomicFloatFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT};
};

} // namespace ocf::rhi
