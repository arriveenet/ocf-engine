// SPDX-License-Identifier: MIT
#include "ocf/renderer/Material.h"

#include "ocf/core/Engine.h"
#include "ocf/rhi/Device.h"
#include "ocf/rhi/RHIEnums.h"

namespace ocf {

using namespace rhi;

struct Material::BuilderDetails {
};

Material::Builder::Builder()
{
    m_impl = new BuilderDetails();
}

Material::Builder::~Builder()
{
    delete m_impl;
}

Material* Material::Builder::build(Engine& engine)
{
    Material* material = new Material(engine, *this);
    return material;
}

Material::Material(Engine& engine, const Builder& builder)
{
    Engine::Device& device = engine.getDevice();

    // Create descriptor set layout
    DescriptorSetLayout descriptorSetLayout;
    DescriptorLayoutBinding bindings[2] = {
        {
            .binding = 0,
            .type = DescriptorType::UniformBuffer,
            .shaderStageFlags = ShaderStageFlags::Vertex | ShaderStageFlags::Fragment
        },
    {
            .binding = 1,
            .type = DescriptorType::CombinedImageSampler,
            .shaderStageFlags = ShaderStageFlags::Fragment,
        }
    };
    descriptorSetLayout.descriptors.insert(descriptorSetLayout.descriptors.end(),
        std::begin(bindings), std::end(bindings));

    m_descriptorSetLayoutHandle = device.createDescriptorLayoutSet(descriptorSetLayout);

    // Create UniformBuffer
    for (uint32_t i = 0; i < m_uniformBuffers.size(); i++) {
        m_uniformBuffers[i] = device.createBufferObject(BufferType::Uniform, sizeof(SceneConstants));
    }

    // Create DescriptorSet
    for (uint32_t i = 0; i< m_descriptorSets.size(); i++) {
        m_descriptorSets[i] = device.createDescriptorSet(m_descriptorSetLayoutHandle);

        device.updateDescriptorSet(m_descriptorSets[i], m_uniformBuffers[i], 0);
    }

}

Material::~Material()
{
}

void Material::terminate(Engine& engine)
{
    auto& device = engine.getDevice();

    for (uint32_t i = 0; i < m_uniformBuffers.size(); i++) {
        device.destroyBufferObject(m_uniformBuffers[i]);
    }

    for (uint32_t i = 0; i < m_descriptorSets.size(); i++) {
        device.destroyDescriptorSet(m_descriptorSets[i]);
    }

    device.destroyDescriptorSetLayout(m_descriptorSetLayoutHandle);
}

} // namespace ocf
