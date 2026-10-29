// SPDX-License-Identifier: MIT
#include "ocf/rhi/vulkan/VulkanPipelineCache.h"

#include "ocf/rhi/vulkan/VulkanPipelineBuilder.h"
#include "ocf/rhi/vulkan/VulkanUtility.h"

#include "ocf/core/Logger.h"

#include <xxhash.h>

#include <cstring>

namespace ocf::rhi {

size_t VulkanPipelineCache::KeyHasher::operator()(const PipelineKey& key) const noexcept
{
    return static_cast<size_t>(XXH3_64bits(&key, sizeof(key)));
}

bool VulkanPipelineCache::KeyEqual::operator()(const PipelineKey& lhs,
                                               const PipelineKey& rhs) const noexcept
{
    return std::memcmp(&lhs, &rhs, sizeof(PipelineKey)) == 0;
}

VulkanPipelineCache::VulkanPipelineCache(VkDevice device)
    : m_device(device)
{
}

VulkanPipelineCache::~VulkanPipelineCache()
{
    clear();
}

VkPipeline VulkanPipelineCache::getPipeline(const PipelineKey& key, const PipelineObjects& objects,
                                            uint64_t frameNumber)
{
    auto iter = m_pipelines.find(key);
    if (iter != m_pipelines.end()) {
        iter->second.lastUsed = frameNumber;
        return iter->second.pipeline;
    }

    // A failed build is cached too, so that it is not retried on every draw
    VkPipeline pipeline = buildPipeline(key, objects);
    if (pipeline == VK_NULL_HANDLE) {
        OCF_LOG_ERROR("Failed to create pipeline");
    }

    m_pipelines.emplace(key, Entry{.pipeline = pipeline, .lastUsed = frameNumber});
    return pipeline;
}

void VulkanPipelineCache::gc(uint64_t currentFrameNumber, uint64_t completedFrameNumber)
{
    for (auto iter = m_pipelines.begin(); iter != m_pipelines.end();) {
        const Entry& entry = iter->second;
        const bool expired = entry.lastUsed + MAX_PIPELINE_AGE < currentFrameNumber;
        if (expired && entry.lastUsed <= completedFrameNumber) {
            vkDestroyPipeline(m_device, entry.pipeline, nullptr);
            iter = m_pipelines.erase(iter);
        }
        else {
            ++iter;
        }
    }
}

void VulkanPipelineCache::clear()
{
    for (auto& [key, entry] : m_pipelines) {
        vkDestroyPipeline(m_device, entry.pipeline, nullptr);
    }

    m_pipelines.clear();
}

VkPipeline VulkanPipelineCache::buildPipeline(const PipelineKey& key,
                                              const PipelineObjects& objects) const
{
    RasterState rasterState;
    rasterState.value = key.rasterState;

    // Depth buffer settings
    VkPipelineDepthStencilStateCreateInfo depthStencilState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = rasterState.bits.depthWriteEnable ? VK_TRUE : VK_FALSE,
        .depthCompareOp = VulkanUtility::getDepthFunc(rasterState.bits.depthFunc),
    };

    // Culling settings
    VkPipelineRasterizationStateCreateInfo rasterizerState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VulkanUtility::getCullMode(rasterState.bits.culling),
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f,
    };

    // Color blending settings
    VkPipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = rasterState.hasBlending() ? VK_TRUE : VK_FALSE,
        .srcColorBlendFactor = VulkanUtility::getBlendFunction(rasterState.bits.blendFunctionSrcColor),
        .dstColorBlendFactor = VulkanUtility::getBlendFunction(rasterState.bits.blendFunctionDstColor),
        .colorBlendOp = VulkanUtility::getBlendEquation(rasterState.bits.blendEquationColor),
        .srcAlphaBlendFactor = VulkanUtility::getBlendFunction(rasterState.bits.blendFunctionSrcAlpha),
        .dstAlphaBlendFactor = VulkanUtility::getBlendFunction(rasterState.bits.blendFunctionDstAlpha),
        .alphaBlendOp = VulkanUtility::getBlendEquation(rasterState.bits.blendEquationAlpha),
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    // Vertex input settings. All attributes are read from a single interleaved buffer.
    VkVertexInputBindingDescription bindingDescription{
        .binding = 0,
        .stride = key.vertexStride,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    VkVertexInputAttributeDescription attributeDescriptions[VERTEX_ATTRIBUTE_COUNT_MAX] = {};
    for (uint32_t i = 0; i < key.attributeCount; i++) {
        attributeDescriptions[i] = VkVertexInputAttributeDescription{
            .location = key.attributes[i].location,
            .binding = 0,
            .format = key.attributes[i].format,
            .offset = key.attributes[i].offset,
        };
    }

    // Build pipeline
    VulkanPipelineBuilder builder{};
    builder.addShaderStage(VK_SHADER_STAGE_VERTEX_BIT, objects.vertexShader,
                           objects.vertexEntryPoint);
    builder.addShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, objects.fragmentShader,
                           objects.fragmentEntryPoint);
    builder.setVertexInput(&bindingDescription, 1, attributeDescriptions, key.attributeCount);
    builder.setPrimitiveTopology(key.topology);
    builder.useDynamicViewport();
    builder.setPipelineLayout(objects.layout);
    builder.setDepthStencilState(depthStencilState);
    builder.setRasterizationState(rasterizerState);
    builder.setColorBlendAttachmentState(colorBlendAttachment);
    builder.useDynamicRendering(key.colorFormat, key.depthFormat);
    return builder.build(m_device);
}

} // namespace ocf::rhi
