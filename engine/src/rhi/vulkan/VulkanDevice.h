// SPDX-License-Identifier: MIT

#pragma once

#include "rhi/HandleAllocator.h"
#include "VulkanCommandBuffer.h"
#include "VulkanUtility.h"

#include "ocf/rhi/Device.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

namespace ocf::rhi {

class VulkanContext;
class VulkanSwapchain;

struct VulkanShaderModule : public RHIShaderModule {
    struct VK {
        VkShaderModule id = VK_NULL_HANDLE;
    } vk;

};

class VulkanDevice : public Device {
public:
    const uint32_t FRAMES_IN_FLIGHT_MAX = 2;

    VulkanDevice(const DeviceConfig& config, VulkanContext& context);
    ~VulkanDevice() override;

    bool initialize();
    void terminate();

    VertexBufferInfoHandle createVertexBufferInfo(uint8_t attributeCount,
                                                  AttributeArray attributes) override;

    VertexBufferHandle createVertexBuffer(uint32_t vertexCount, uint32_t byteCount,
                                          BufferUsage usage, VertexBufferInfoHandle vbih) override;

    TextureHandle createTexture() override;

    ShaderModuleHandle createShaderModule(std::string_view filename) override;

    SwapchainHandle createSwapchain(Window* window, uint32_t width, uint32_t height) override;

    void updateBufferData(VertexBufferHandle handle, const void* data, size_t size,
                          size_t offset) override;

    std::shared_ptr<CommandBuffer> getCommandBuffer() override;

    void beginFrame() override;

    void endFrame() override;

    VkDevice getDevice() const noexcept { return m_device; }

    VulkanContext& getContext() const noexcept { return m_context; }

    VkCommandPool getCommandPool() const noexcept { return m_commandPool; }

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

    void setDebugObjectName(void* objectHandle, VkObjectType type, const char* name);

    /** Switches the drawable swapchain image */
    VulkanResult acquireNextImage();

    /** Execute the command in the current frame context and publish the presentation */
    void submitPresent();

    VulkanSwapchain* getSwapchain() const noexcept { return m_swapchain; }

    std::shared_ptr<VulkanCommandBuffer> createCommandBuffer();

    void createFrameContexts();

    void destroyFrameContexts();

    uint32_t getCurrentFrameIndex() const noexcept { return m_currentFrameIndex; }

    FrameContext* getCurrentFrameContext();

    void advanceFrame();

    void buildFeatures();

private:
    VulkanContext& m_context;
    HandleAllocatorVK m_handleAllocator;

    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkPhysicalDeviceMemoryProperties m_memoryProperties{};
    VkPhysicalDeviceProperties m_deviceProperties{};
    VulkanSwapchain* m_swapchain = nullptr;
    std::vector<FrameContext> m_frameContext;
    uint32_t m_currentFrameIndex = 0;

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

    friend class VulkanCommandBuffer;
};

} // namespace ocf::rhi
