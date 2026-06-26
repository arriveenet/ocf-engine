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

void transitionImageLayout(VkCommandBuffer       cmd,
                              VkImage               image,
                              VkImageLayout         oldLayout,
                              VkImageLayout         newLayout,
                              VkPipelineStageFlags2 srcStage,
                              VkAccessFlags2        srcAccess,
                              VkPipelineStageFlags2 dstStage,
                              VkAccessFlags2        dstAccess,
                              uint32_t              baseMipLevel,
                              uint32_t              levelCount);

constexpr VulkanResourceStateInfo getResourceState(ResourceState state)
{
    switch (state) {
    case ResourceState::Undefined:
        return {
            .layout = VK_IMAGE_LAYOUT_UNDEFINED,
            .accessMask = 0,
            .stageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        };
    case ResourceState::ColorAttachment:
        return {
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .accessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        };
    case ResourceState::Present:
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

constexpr VkFormat getElementType(ElementType type)
{
    switch (type) {
    case ElementType::Byte:     return VK_FORMAT_R8_UINT;
    case ElementType::Byte2:    return VK_FORMAT_R8G8_UINT;
    case ElementType::Byte3:    return VK_FORMAT_R8G8B8_UINT;
    case ElementType::Byte4:    return VK_FORMAT_R8G8B8A8_UINT;
    case ElementType::Ubyte:    return VK_FORMAT_R8_UINT;
    case ElementType::Ubyte2:   return VK_FORMAT_R8G8_UINT;
    case ElementType::Ubyte3:   return VK_FORMAT_R8G8B8_UINT;
    case ElementType::Ubyte4:   return VK_FORMAT_R8G8B8A8_UINT;
    case ElementType::Short:    return VK_FORMAT_R16_SINT;
    case ElementType::Short2:   return VK_FORMAT_R16G16_SINT;
    case ElementType::Short3:   return VK_FORMAT_R16G16B16_SINT;
    case ElementType::Short4:   return VK_FORMAT_R16G16B16A16_SINT;
    case ElementType::Ushort:   return VK_FORMAT_R16_UINT;
    case ElementType::Ushort2:  return VK_FORMAT_R16G16_UINT;
    case ElementType::Ushort3:  return VK_FORMAT_R16G16B16_UINT;
    case ElementType::Ushort4:  return VK_FORMAT_R16G16B16A16_UINT;
    case ElementType::Int:      return VK_FORMAT_R32_SINT;
    case ElementType::Uint:     return VK_FORMAT_R32_UINT;
    case ElementType::Float:    return VK_FORMAT_R32_SFLOAT;
    case ElementType::Float2:   return VK_FORMAT_R32G32_SFLOAT;
    case ElementType::Float3:   return VK_FORMAT_R32G32B32_SFLOAT;
    case ElementType::Float4:   return VK_FORMAT_R32G32B32A32_SFLOAT;
    case ElementType::Double:   return VK_FORMAT_R64_SFLOAT;
    case ElementType::Double2:  return VK_FORMAT_R64G64_SFLOAT;
    case ElementType::Double3:  return VK_FORMAT_R64G64B64_SFLOAT;
    case ElementType::Double4:  return VK_FORMAT_R64G64B64A64_SFLOAT;
    default:
        assert(false && "Unknown ElementType");
        return VK_FORMAT_UNDEFINED;
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

constexpr VkFilter getMagFilter(SamplerMagFilter filter)
{
    switch (filter) {
    case SamplerMagFilter::Nearest: return VK_FILTER_NEAREST;
    case SamplerMagFilter::Linear:  return VK_FILTER_LINEAR;
    }

    assert(false && "Unknown MagFilter");
    return VK_FILTER_MAX_ENUM;
}

constexpr VkFilter getMinFilter(SamplerMinFilter filter)
{
    switch (filter) {
    case SamplerMinFilter::Nearest:
    case SamplerMinFilter::NearestMipmapNearest:
    case SamplerMinFilter::NearestMipmapLinear:
        return VK_FILTER_NEAREST;

    case SamplerMinFilter::Linear:
    case SamplerMinFilter::LinearMipmapNearest:
    case SamplerMinFilter::LinearMipmapLinear:
        return VK_FILTER_LINEAR;
    }

    assert(false && "Unknown MinFilter");
    return VK_FILTER_MAX_ENUM;
}

constexpr VkSamplerMipmapMode getMipmapMode(SamplerMinFilter filter)
{
    switch (filter) {
    case SamplerMinFilter::Nearest:
    case SamplerMinFilter::Linear:
    case SamplerMinFilter::NearestMipmapNearest:
    case SamplerMinFilter::LinearMipmapNearest:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;

    case SamplerMinFilter::NearestMipmapLinear:
    case SamplerMinFilter::LinearMipmapLinear:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }

    assert(false && "Unknown MipmapMode");
    return VK_SAMPLER_MIPMAP_MODE_MAX_ENUM;
}

constexpr VkSamplerAddressMode getWrapMode(SamplerWrapMode mode)
{
    switch (mode) {
    case SamplerWrapMode::ClampToEdge:      return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case SamplerWrapMode::Rpeat:            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case SamplerWrapMode::MirroredRepeat:   return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    }

    assert(false && "Unkown WrapMode");
    return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
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
