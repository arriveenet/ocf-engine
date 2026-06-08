// SPDX-License-Identifier: MIT
#pragma once

#include "renderer/DescriptorSet.h"

#include "ocf/math/mat3.h"
#include "ocf/math/mat4.h"
#include "ocf/math/vec2.h"
#include "ocf/math/vec3.h"
#include "ocf/math/vec4.h"
#include "ocf/renderer/TextureSampler.h"
#include "ocf/rhi/Handle.h"

#include <concepts>
#include <string_view>

namespace ocf {

class Texture;
class Material;

template<typename T>
concept MaterialParameterType =
    std::same_as<T, bool> ||
    std::same_as<T, short> ||
    std::same_as<T, int> ||
    std::same_as<T, math::ivec2> ||
    std::same_as<T, math::ivec3> ||
    std::same_as<T, math::ivec4> ||
    std::same_as<T, float> ||
    std::same_as<T, math::vec2> ||
    std::same_as<T, math::vec3> ||
    std::same_as<T, math::vec4> ||
    std::same_as<T, math::mat3> ||
    std::same_as<T, math::mat4> ;

class MaterialInstance {
public:
    MaterialInstance(Material* material);
    virtual ~MaterialInstance();

    void commit();

    void setFrameIndex(uint32_t index) { m_frameIndex = index; }

    rhi::DescriptorSetHandle getDescriptorSetHandle()
    {
        return m_descriptorSet.getDescriptorSetHandle(m_frameIndex);
    }

    template <MaterialParameterType T>
    void setParameter(std::string_view name, const T& value);

    void setParameter(std::string_view name, Texture* texture, const TextureSampler& sampler);

protected:
    Material* m_material = nullptr;
    DescriptorSet m_descriptorSet;
    uint32_t m_frameIndex = 0;
};

} // namespace ocf
