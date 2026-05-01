#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace ocf::rhi {

class VulkanPipelineBuilder {
public:
    VulkanPipelineBuilder();

    // Adds a shader stage to the pipeline
    VulkanPipelineBuilder& addShaderStage(VkShaderStageFlagBits stage, VkShaderModule module,
                                      const char* entry);

    // Sets the vertex input state
    VulkanPipelineBuilder& setVertexInput(const VkVertexInputBindingDescription* bindings,
                                      uint32_t bindingCount,
                                      const VkVertexInputAttributeDescription* attributes,
                                      uint32_t attributeCount);

    // Sets the viewport and scissor
    VulkanPipelineBuilder& setViewport(VkExtent2D extent);
    VulkanPipelineBuilder& setViewport(VkViewport& viewport, VkRect2D scissor);


    // Sets the color blend attachment state
    void setColorBlendAttachmentState(const VkPipelineColorBlendAttachmentState& state);

    // Sets the rasterization state
    void setRasterizationState(const VkPipelineRasterizationStateCreateInfo& state);

    // Sets the depth stencil state
    void setDepthStencilState(const VkPipelineDepthStencilStateCreateInfo& state);

    // Sets the pipeline layout
    VulkanPipelineBuilder& setPipelineLayout(VkPipelineLayout layout);

    // Sets the render pass and subpass
    VulkanPipelineBuilder& useRenderPass(VkRenderPass renderPass, uint32_t subpass);

    // Sets dynamic rendering with specified formats
    VulkanPipelineBuilder& useDynamicRendering(VkFormat colorFormat,
                                           VkFormat depthFormat = VK_FORMAT_UNDEFINED);

    // Builds and returns the graphics pipeline
    VkPipeline build(VkDevice device);

private:
    std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;

    VkPipelineVertexInputStateCreateInfo m_vertexInputState{};
    std::vector<VkVertexInputBindingDescription> m_bindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> m_attributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo m_inputAssemblyState{};
    VkPipelineViewportStateCreateInfo m_viewportState{};
    VkViewport m_viewport{};
    VkRect2D m_scissor{};

    VkPipelineRasterizationStateCreateInfo m_rasterizationState{};
    VkPipelineMultisampleStateCreateInfo m_multisampleState{};
    VkPipelineColorBlendAttachmentState m_colorBlendAttachment{};
    VkPipelineColorBlendStateCreateInfo m_colorBlendState{};
    VkPipelineDepthStencilStateCreateInfo m_depthStencilState{};

    bool m_tessellationEnabled = false;
    VkPipelineTessellationStateCreateInfo m_tessellationState{};

    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkFormat m_colorFormat = VK_FORMAT_UNDEFINED;
    VkFormat m_depthFormat = VK_FORMAT_UNDEFINED;

    bool m_useRenderPass = false;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    uint32_t m_subpass = 0;
};

} // namespace ocf::rhi
