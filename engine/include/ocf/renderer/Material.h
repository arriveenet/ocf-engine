// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/rhi/Handle.h"
#include "ocf/math/mat4.h"
#include "ocf/math/vec4.h"

#include <array>

namespace ocf {

class Engine;

struct SceneConstants
{
    math::mat4 mtxWorld;
    math::mat4 mtxView;
    math::mat4 mtxProj;
    math::vec4 lightDir;
    math::vec4 eyePosition;
};

class Material {
    struct BuilderDetails;

public:
    using DescriptorSetLayoutHandle = rhi::DescriptorSetLayoutHandle;
    using DescriptorSetHandle = rhi::DescriptorSetHandle;
    using BufferObjectHandle = rhi::BufferObjectHandle;

    class Builder {
    public:
        Builder();
        ~Builder();

        BuilderDetails* operator->() noexcept { return m_impl; }
        const BuilderDetails* operator->() const noexcept { return m_impl; }

        Material* build(Engine& engine);

    private:
        BuilderDetails* m_impl = nullptr;
    };

    Material(Engine& engine, const Builder& builder);
    ~Material();

    void terminate(Engine& engine);

    DescriptorSetLayoutHandle getDescriptorSetLayout() const { return m_descriptorSetLayoutHandle;}

    DescriptorSetHandle getDescriptorSet(uint32_t index) const { return m_descriptorSets[index]; }

    BufferObjectHandle getUniformBuffer(uint32_t index) const { return m_uniformBuffers[index]; }

private:
    DescriptorSetLayoutHandle m_descriptorSetLayoutHandle;
    std::array<DescriptorSetHandle, 2> m_descriptorSets;
    std::array<BufferObjectHandle, 2> m_uniformBuffers;
};

} // namespace ocf
