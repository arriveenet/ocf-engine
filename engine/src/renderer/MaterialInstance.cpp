// SPDX-License-Identifier: MIT
#include "ocf/renderer/MaterialInstance.h"

#include "ocf/math/mat4.h"
#include "ocf/math/vec4.h"
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

template <typename T>
void MaterialInstance::setParameter(std::string_view name, const T& value)
{
    const auto* routingInfo = layout->getParameterInfo(name);
    if (!routingInfo) {
        return;
    }

    if (sizeof(T) <= routingInfo->size) {
        uint32_t binding = routingInfo->binding;

        std::memcpy(m_buffers[binding].data() + routingInfo->offset, &value, sizeof(T));
        m_dirtyFlags[binding] = true;
    }
}

void MaterialInstance::setParameter(std::string_view name, Texture* texture,
                                    const TextureSampler& sampler)
{
}

template void MaterialInstance::setParameter<math::mat4>(std::string_view, const math::mat4&);
template void MaterialInstance::setParameter<math::vec4>(std::string_view, const math::vec4&);

} // namespace ocf
