// SPDX-License-Identifier: MIT
#include "ocf/renderer/MaterialInstance.h"

#include "ocf/renderer/Material.h"

namespace ocf {

MaterialInstance::MaterialInstance(Material* material)
    : m_material(material)
{
}

MaterialInstance::~MaterialInstance()
{
}

void MaterialInstance::commit()
{
    
}

template <MaterialParameterType T>
void MaterialInstance::setParameter(std::string_view name, const T& value)
{
}

void MaterialInstance::setParameter(std::string_view name, Texture* texture,
                                    const TextureSampler& sampler)
{
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
