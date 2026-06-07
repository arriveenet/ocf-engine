// SPDX-License-Identifier: MIT
#pragma once

#include "renderer/DescriptorSet.h"

#include "ocf/renderer/TextureSampler.h"
#include "ocf/rhi/Handle.h"

#include <string_view>
#include <unordered_map>
#include <vector>

namespace ocf {

class Texture;
class Material;

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

    template<typename T>
    void setParameter(std::string_view name, const T& value);

    void setParameter(std::string_view name, Texture* texture, const TextureSampler& sampler);

protected:
    Material* m_material = nullptr;
    DescriptorSet m_descriptorSet;
    uint32_t m_frameIndex = 0;
};

} // namespace ocf
