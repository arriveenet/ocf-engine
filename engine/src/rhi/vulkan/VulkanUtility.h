// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/Result.h"
#include "ocf/rhi/RHIEnums.h"

#include <vulkan/vulkan.h>

#include <optional>
#include <vulkan/vulkan_core.h>

namespace ocf {
namespace rhi {

struct VulkanError {
    VkResult result;
    const char* message;
    const char* file;
    size_t line;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
};

struct VulkanResourceStateInfo {
    VkImageLayout layout;
    VkAccessFlags accessMask;
    VkPipelineStageFlags stageFlags;
};

using VulkanResult = ocf::Result<void, VulkanError>;

namespace VulkanUtility {

void assertVkError(VkResult ret, const char* function, size_t line) noexcept;

#ifdef NDEBUG
#   define VK_CHECK_RESULT(ret)
#else
#   define VK_CHECK_RESULT(ret) { VulkanUtility::assertVkError(ret, __func__, __LINE__); }
#endif

#define VK_ERROR(result, msg)                                                                      \
    VulkanError                                                                                    \
    {                                                                                              \
        result, msg, __FILE__, __LINE__                                                            \
    }

void logError(const VulkanError& error);

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

constexpr VulkanResourceStateInfo getResourceState(ResourceState state)
{
    switch (state) {
    case ocf::rhi::ResourceState::Undefined:
        return {
            .layout = VK_IMAGE_LAYOUT_UNDEFINED,
            .accessMask = 0,
            .stageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        };
    case ocf::rhi::ResourceState::ColorAttachment:
        return {
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .accessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        };
    case ocf::rhi::ResourceState::Present:
        return {
            .layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .accessMask = 0,
            .stageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        };
    default:
        return {
            .layout = VK_IMAGE_LAYOUT_UNDEFINED,
            .accessMask = 0,
            .stageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        };
    }
}

constexpr VkDescriptorType getDescriptorType(DescriptorType type)
{
    switch (type) {
    case DescriptorType::Sampler:               return VK_DESCRIPTOR_TYPE_SAMPLER;
    case DescriptorType::CombinedImageSampler:  return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case DescriptorType::StorageImage:          return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case DescriptorType::UniformBuffer:         return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case DescriptorType::StorageBuffer:         return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    default:
        assert(false && "Unknown DescriptorType");
        return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
}

constexpr VkShaderStageFlags getShaderStageFlagBits(ShaderStageFlags flags)
{
    VkShaderStageFlags vkFlags = 0;

    if (flags & ShaderStageFlags::Vertex) {
        vkFlags |= VK_SHADER_STAGE_VERTEX_BIT;
    }
    if (flags & ShaderStageFlags::Fragment) {
        vkFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    if (flags & ShaderStageFlags::Compute) {
        vkFlags |= VK_SHADER_STAGE_COMPUTE_BIT;
    }

    return vkFlags;
}

constexpr VkCompareOp getDepthFunc(RasterState::DepthFunc func)
{
    using DepthFunc = SamplerCompareFunc;

    switch (func) {
    case DepthFunc::Never:          return VK_COMPARE_OP_NEVER;
    case DepthFunc::Less:           return VK_COMPARE_OP_LESS;
    case DepthFunc::LessEqual:      return VK_COMPARE_OP_LESS_OR_EQUAL;
    case DepthFunc::Equal:          return VK_COMPARE_OP_EQUAL;
    case DepthFunc::Greater:        return VK_COMPARE_OP_GREATER;
    case DepthFunc::NotEqual:       return VK_COMPARE_OP_NOT_EQUAL;
    case DepthFunc::GreaterEqual:   return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case DepthFunc::Always:         return VK_COMPARE_OP_ALWAYS;
    default:
        assert(false && "Unknown CompareOp");
        return VK_COMPARE_OP_MAX_ENUM;
    }
}

constexpr VkCullModeFlags getCullMode(CullingMode mode)
{
    switch (mode) {
    case CullingMode::None:         return VK_CULL_MODE_NONE;
    case CullingMode::Front:        return VK_CULL_MODE_FRONT_BIT;
    case CullingMode::Back:         return VK_CULL_MODE_BACK_BIT;
    case CullingMode::FrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;
    default:
        assert(false && "Unknown CullingMode");
        return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
    }
}

constexpr const char* getPhysicalDeviceTypeString(VkPhysicalDeviceType type)
{
    switch (type) {
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:             return "Other";
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:    return "Integrated GPU";
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:      return "Discrete GPU";
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:       return "Virtual GPU";
    case VK_PHYSICAL_DEVICE_TYPE_CPU:               return "CPU";
    default:                                        return "Unknown";
    }
}

} // namespace VulkanUtility

} // namespace rhi
} // namespace ocf
