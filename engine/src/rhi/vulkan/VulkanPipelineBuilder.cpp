// SPDX-License-Identifier: MIT
#include "VulkanPipelineBuilder.h"

namespace ocf::rhi {

VulkanPipelineBuilder::VulkanPipelineBuilder()
{
    m_vertexInputState = VkPipelineVertexInputStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr,
    };

    m_inputAssemblyState = VkPipelineInputAssemblyStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    m_rasterizationState = VkPipelineRasterizationStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    m_multisampleState = VkPipelineMultisampleStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    const auto colorWriteAll = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                               VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    m_colorBlendAttachment = VkPipelineColorBlendAttachmentState{
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = colorWriteAll,
    };
    m_colorBlendState = VkPipelineColorBlendStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &m_colorBlendAttachment,
        .blendConstants = {},
    };
}

VulkanPipelineBuilder& VulkanPipelineBuilder::addShaderStage(VkShaderStageFlagBits stage,
                                                     VkShaderModule module, const char* entry)
{
    VkPipelineShaderStageCreateInfo shaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = stage,
        .module = module,
        .pName = entry,
    };
    m_shaderStages.push_back(shaderStageInfo);

    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::setVertexInput(
    const VkVertexInputBindingDescription* bindings, uint32_t bindingCount,
    const VkVertexInputAttributeDescription* attributes, uint32_t attributeCount)
{
    m_bindingDescriptions.resize(bindingCount);
    for (uint32_t i = 0; i < bindingCount; ++i) {
        m_bindingDescriptions[i] = bindings[i];
    }
    m_attributeDescriptions.resize(attributeCount);
    for (uint32_t i = 0; i < attributeCount; ++i) {
        m_attributeDescriptions[i] = attributes[i];
    }
    m_vertexInputState = VkPipelineVertexInputStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = bindingCount,
        .pVertexBindingDescriptions = m_bindingDescriptions.data(),
        .vertexAttributeDescriptionCount = attributeCount,
        .pVertexAttributeDescriptions = m_attributeDescriptions.data(),
    };

    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::setViewport(VkExtent2D extent)
{
    m_viewport = VkViewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(extent.width),
        .height = static_cast<float>(extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    // Vertical inversion by VK_KHR_Maintenance1
    m_viewport.y = float(extent.height);
    m_viewport.height = -float(extent.height);

    m_scissor = VkRect2D{
        .offset = {0, 0},
        .extent = extent,
    };

    m_viewportState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                       .viewportCount = 1,
                       .pViewports = &m_viewport,
                       .scissorCount = 1,
                       .pScissors = &m_scissor};

    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::setViewport(VkViewport& viewport, VkRect2D scissor)
{
    m_viewport = viewport;
    m_scissor = scissor;

    m_viewportState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                       .viewportCount = 1,
                       .pViewports = &m_viewport,
                       .scissorCount = 1,
                       .pScissors = &m_scissor};

    return *this;
}

void VulkanPipelineBuilder::setColorBlendAttachmentState(
    const VkPipelineColorBlendAttachmentState& state)
{
    m_colorBlendAttachment = state;
}

void VulkanPipelineBuilder::setRasterizationState(const VkPipelineRasterizationStateCreateInfo& state)
{
    m_rasterizationState = state;
}

void VulkanPipelineBuilder::setDepthStencilState(const VkPipelineDepthStencilStateCreateInfo& state)
{
    m_depthStencilState = state;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::setPipelineLayout(VkPipelineLayout layout)
{
    m_pipelineLayout = layout;

    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::useRenderPass(VkRenderPass renderPass, uint32_t subpass)
{
    m_useRenderPass = true;
    m_renderPass = renderPass;
    m_subpass = subpass;

    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::useDynamicRendering(VkFormat colorFormat,
                                                          VkFormat depthFormat)
{
    m_useRenderPass = false;
    m_colorFormat = colorFormat;
    m_depthFormat = depthFormat;

    return *this;
}

VkPipeline VulkanPipelineBuilder::build(VkDevice device)
{
    VkGraphicsPipelineCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(m_shaderStages.size()),
        .pStages = m_shaderStages.data(),
        .pVertexInputState = &m_vertexInputState,
        .pInputAssemblyState = &m_inputAssemblyState,
        .pViewportState = &m_viewportState,
        .pRasterizationState = &m_rasterizationState,
        .pMultisampleState = &m_multisampleState,
        .pDepthStencilState = &m_depthStencilState,
        .pColorBlendState = &m_colorBlendState,
        .layout = m_pipelineLayout,
    };

    VkPipelineRenderingCreateInfo renderingInfo{};
    if (m_useRenderPass) {
        pipelineInfo.renderPass = m_renderPass;
        pipelineInfo.subpass = m_subpass;
    }
    else {
        renderingInfo = VkPipelineRenderingCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &m_colorFormat,
            .depthAttachmentFormat = m_depthFormat,
        };
        pipelineInfo.pNext = &renderingInfo;
        pipelineInfo.renderPass = VK_NULL_HANDLE;
        pipelineInfo.subpass = 0;
    }

    if (m_tessellationEnabled) {
        pipelineInfo.pTessellationState = &m_tessellationState;
    }

    VkPipeline pipeline = VK_NULL_HANDLE;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) !=
        VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    return pipeline;
}

} // namespace ocf::rhi
