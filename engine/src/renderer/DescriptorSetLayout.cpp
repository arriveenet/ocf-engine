// SPDX-License-Identifier: MIT
#include "DescriptorSetLayout.h"

#include "ocf/core/Engine.h"
#include "ocf/rhi/Device.h"

namespace ocf {

void DescriptorSetLayout::create(Engine& engine)
{
    Engine::Device& device = engine.getDevice();

    // Create descriptor set layout
    rhi::DescriptorSetLayout descriptorSetLayout;

    for (auto& block : m_blocks) {
        auto& blockInfo = block.second;

        rhi::DescriptorLayoutBinding binding{
            .binding = blockInfo.binding,
            .type = rhi::DescriptorType::UniformBuffer,
            .shaderStageFlags = rhi::ShaderStageFlags::Vertex | rhi::ShaderStageFlags::Fragment,
        };

        descriptorSetLayout.descriptors.push_back(binding);
    }

    m_handle = device.createDescriptorLayoutSet(descriptorSetLayout);
}

void DescriptorSetLayout::terminate(Engine& engine)
{
    engine.getDevice().destroyDescriptorSetLayout(m_handle);
}

} // namespace ocf
