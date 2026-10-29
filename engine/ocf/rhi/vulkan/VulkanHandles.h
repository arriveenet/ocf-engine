// SPDX-License-Identifier: MIT

#pragma once

#include "ocf/rhi/Device.h"

#include "ocf/rhi/vulkan/VulkanBuffer.h"
#include "ocf/rhi/vulkan/VulkanPipelineCache.h"
#include "ocf/rhi/vulkan/resource/ImageResource.h"

#include <memory>

namespace  ocf::rhi {

struct VulkanVertexBufferInfo : public RHIVertexBufferInfo {
    AttributeArray attributes;

    VulkanVertexBufferInfo() noexcept = default;
    VulkanVertexBufferInfo(uint8_t attributeCount, AttributeArray attributes)
    : RHIVertexBufferInfo(attributeCount)
    , attributes(attributes)
    {
    }
};

struct VulkanBufferObject : public RHIBufferObject {
    std::shared_ptr<IBufferResource> buffer;

    VulkanBufferObject() noexcept = default;
};

struct VulkanDescriptorSetLayout : public RHIDescriptorSetLayout {
    struct VK {
        VkDescriptorSetLayout id = VK_NULL_HANDLE;
    } vk;

    DescriptorSetLayout layout;
};

struct VulkanDescriptorSet : public RHIDescriptorSet {
    struct VK {
        VkDescriptorSet id = VK_NULL_HANDLE;
    } vk;
};

struct VulkanTexture : public RHITexture {
    std::shared_ptr<IImageResource> image;
    TextureUsage usage = TextureUsage::Default;
};

/** Owns a VkShaderModule, shared by the shader module handle and the pipelines built from it */
class VulkanShaderModuleObject {
public:
    VulkanShaderModuleObject(VkDevice device, VkShaderModule module, uint32_t id)
        : m_device(device)
        , m_module(module)
        , m_id(id)
    {
    }

    ~VulkanShaderModuleObject()
    {
        if (m_module != VK_NULL_HANDLE) {
            vkDestroyShaderModule(m_device, m_module, nullptr);
        }
    }

    VulkanShaderModuleObject(const VulkanShaderModuleObject&) = delete;
    VulkanShaderModuleObject& operator=(const VulkanShaderModuleObject&) = delete;

    VkShaderModule get() const noexcept { return m_module; }

    /** Identifies the module in pipeline cache keys. Never reused, unlike the Vulkan handle. */
    uint32_t getId() const noexcept { return m_id; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkShaderModule m_module = VK_NULL_HANDLE;
    uint32_t m_id = 0;
};

struct VulkanShaderModule : public RHIShaderModule {
    std::shared_ptr<VulkanShaderModuleObject> module;
};

/**
 * A pipeline description. The VkPipeline depends on the attachment formats of the render target,
 * so it is looked up in the device's VulkanPipelineCache by the full state when bound.
 */
struct VulkanPipeline : public RHIPipeline {
    /** Everything needed to build a VkPipeline, apart from the attachment formats */
    struct Description {
        std::shared_ptr<VulkanShaderModuleObject> vertexShader;
        std::shared_ptr<VulkanShaderModuleObject> fragmentShader;
        const char* vertexEntryPoint = "main";
        const char* fragmentEntryPoint = "main";
        VulkanPipelineCache::PipelineKey key{}; ///< The attachment formats are set when bound
    };

    struct VK {
        VkPipelineLayout layout = VK_NULL_HANDLE;
    } vk;

    // On the heap, as the key does not fit in the handle allocator's buckets
    std::unique_ptr<Description> description;
};

struct VulkanRenderTarget : public RHIRenderTarget {
    /** Draws into the current swapchain image and the depth buffer of the device */
    bool isDefault = false;
    std::shared_ptr<IImageResource> color;
    std::shared_ptr<IImageResource> depth;
    bool colorSampleable = false; ///< The color image becomes shader readable after a pass
    bool depthSampleable = false; ///< The depth image becomes shader readable after a pass

    VulkanRenderTarget() noexcept = default;
    VulkanRenderTarget(uint32_t width, uint32_t height)
        : RHIRenderTarget(width, height)
    {
    }
};

} // namespace ocf::rhi
