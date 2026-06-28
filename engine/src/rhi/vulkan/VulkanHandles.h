// SPDX-License-Identifier: MIT

#pragma once

#include "ocf/rhi/Device.h"

#include "VulkanBuffer.h"
#include "resource/ImageResource.h"

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
};

struct VulkanShaderModule : public RHIShaderModule {
    struct VK {
        VkShaderModule id = VK_NULL_HANDLE;
    } vk;
};

struct VulkanPipeline : public RHIPipeline {

    struct VK {
        VkPipelineLayout layout = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;
    } vk;
};

} // namespace ocf::rhi
