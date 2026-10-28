// SPDX-License-Identifier: MIT

#pragma once

#include "ocf/rhi/Device.h"

#include "ocf/rhi/vulkan/VulkanBuffer.h"
#include "ocf/rhi/vulkan/resource/ImageResource.h"

#include <memory>
#include <vector>

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
    VulkanShaderModuleObject(VkDevice device, VkShaderModule module)
        : m_device(device)
        , m_module(module)
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

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkShaderModule m_module = VK_NULL_HANDLE;
};

struct VulkanShaderModule : public RHIShaderModule {
    std::shared_ptr<VulkanShaderModuleObject> module;
};

/**
 * A pipeline description. The VkPipeline depends on the attachment formats of the render target,
 * so one is built for each render target format it is drawn into, when it is first bound.
 */
struct VulkanPipeline : public RHIPipeline {
    /** Everything needed to build a VkPipeline, apart from the attachment formats */
    struct Description {
        std::shared_ptr<VulkanShaderModuleObject> vertexShader;
        std::shared_ptr<VulkanShaderModuleObject> fragmentShader;
        const char* vertexEntryPoint = "main";
        const char* fragmentEntryPoint = "main";
        std::vector<VkVertexInputBindingDescription> bindings;
        std::vector<VkVertexInputAttributeDescription> attributes;
        RasterState rasterState;
    };

    struct Variant {
        VkFormat colorFormat = VK_FORMAT_UNDEFINED;
        VkFormat depthFormat = VK_FORMAT_UNDEFINED;
        VkPipeline pipeline = VK_NULL_HANDLE;
    };

    struct VK {
        VkPipelineLayout layout = VK_NULL_HANDLE;
    } vk;

    Description description;
    std::vector<Variant> variants;
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
