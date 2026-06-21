// SPDX-License-Identifier: MIT
#include "ocf/renderer/Material.h"

#include "renderer/DescriptorSet.h"

#include "ocf/core/Engine.h"
#include "ocf/renderer/MaterialInstance.h"
#include "ocf/rhi/Device.h"
#include "ocf/rhi/RHIEnums.h"

#include <utility>
#include <vector>

namespace ocf {

struct Material::BuilderDetails {
   DescriptorSetLayout descriptorSetLayout;
};

Material::Builder::Builder()
{
    m_impl = new BuilderDetails();
}

Material::Builder::~Builder()
{
    delete m_impl;
}

Material::Builder& Material::Builder::uniformBlock(uint32_t binding, std::string_view blockName,
                                         size_t totalSize)
{
    m_impl->descriptorSetLayout.addUniformBlock(binding, blockName.data(), totalSize);
    return *this;
}

Material::Builder& Material::Builder::uniformMember(std::string_view blockName,
                                                    std::string_view memberName,
                                                    UniformType type, size_t offset, size_t size)
{
    m_impl->descriptorSetLayout.addUniformMember(blockName.data(), memberName.data(), type,
                                                 offset, size);
    return *this;
}

Material::Builder& Material::Builder::texture(uint32_t binding, std::string_view name,
                                              SamplerType samplerType,
                                              rhi::ShaderStageFlags stage)
{
    m_impl->descriptorSetLayout.addTexture(binding, name.data(), samplerType, stage);
    return *this;
}

Material* Material::Builder::build(Engine& engine)
{
    Material* material = new Material(engine, *this);
    return material;
}

Material::Material(Engine& engine, const Builder& builder)
    : m_engine(&engine)
{
    Engine::Device& device = engine.getDevice();

    m_descriptorSetLayout = builder->descriptorSetLayout;
    m_descriptorSetLayout.create(engine);
}

Material::~Material()
{
}

void Material::terminate(Engine& engine)
{
    m_descriptorSetLayout.terminate(engine);
}

std::shared_ptr<MaterialInstance> Material::createInstance()
{
    auto instance = std::make_shared<MaterialInstance>(this);
    instance->create(*m_engine);
    return instance;
}

} // namespace ocf
