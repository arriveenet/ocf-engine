// SPDX-License-Identifier: MIT
#include "ocf/renderer/MaterialInstance.h"

#include "ocf/core/Engine.h"
#include "ocf/renderer/Material.h"
#include "ocf/renderer/Texture.h"
#include "renderer/DescriptorSet.h"
#include "renderer/DescriptorSetLayout.h"

namespace ocf {

MaterialInstance::MaterialInstance(Material* material)
    : m_material(material)
{
}

MaterialInstance::~MaterialInstance()
{
}

void MaterialInstance::create(Engine& engine)
{
    m_engine = &engine;
    
    if (!m_material) {
        return;
    }

    // Create descriptor set using the material's layout
    m_descriptorSet.create(engine, m_material->getDescriptorSetLayout());
}

void MaterialInstance::commit(Engine& engine)
{
    m_descriptorSet.commit(engine, m_frameIndex);
}

template <MaterialParameterType T>
void MaterialInstance::setParameter(std::string_view name, const T& value)
{
    if (!m_material) {
        return;
    }

    const auto& layout = m_material->getDescriptorSetLayout();
    const auto& blocks = layout.getBlocks();

    // Search for the parameter in all uniform blocks
    for (const auto& [blockName, blockInfo] : blocks) {
        auto it = blockInfo.members.find(std::string(name));
        if (it != blockInfo.members.end()) {
            const UniformMember& member = it->second;
            
            // Upload the value to the descriptor set's buffer
            m_descriptorSet.uploadUniformBuffer(
                blockInfo.binding,
                member.offset,
                &value,
                member.size
            );
            return;
        }
    }
}

void MaterialInstance::setParameter(std::string_view name, Texture* texture,
                                    const TextureSampler& sampler)
{
    if (!m_material || !m_engine) {
        return;
    }

    const auto& layout = m_material->getDescriptorSetLayout();
    const TextureInfo* texInfo = layout.getTextureInfo(std::string(name));
    if (!texInfo) {
        return;
    }

    m_descriptorSet.updateTextureDescriptor(*m_engine, texInfo->binding, texture, sampler);
}

template void MaterialInstance::setParameter<bool>(std::string_view, const bool&);
template void MaterialInstance::setParameter<short>(std::string_view, const short&);
template void MaterialInstance::setParameter<int>(std::string_view, const int&);
template void MaterialInstance::setParameter<math::ivec2>(std::string_view, const math::ivec2&);
template void MaterialInstance::setParameter<math::ivec3>(std::string_view, const math::ivec3&);
template void MaterialInstance::setParameter<math::ivec4>(std::string_view, const math::ivec4&);
template void MaterialInstance::setParameter<float>(std::string_view, const float&);
template void MaterialInstance::setParameter<math::vec2>(std::string_view, const math::vec2&);
template void MaterialInstance::setParameter<math::vec3>(std::string_view, const math::vec3&);
template void MaterialInstance::setParameter<math::vec4>(std::string_view, const math::vec4&);
template void MaterialInstance::setParameter<math::mat3>(std::string_view, const math::mat3&);
template void MaterialInstance::setParameter<math::mat4>(std::string_view, const math::mat4&);

} // namespace ocf
