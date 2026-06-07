// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/math/mat4.h"
#include "ocf/math/vec4.h"
#include "ocf/rhi/Handle.h"
#include "renderer/DescriptorSetLayout.h"

#include <array>
#include <memory>
#include <ocf/rhi/RHIEnums.h>
#include <string_view>

namespace ocf {

class Engine;
class MaterialInstance;

class Material {
    struct BuilderDetails;

public:
    using UniformType = rhi::UniformType;

    class Builder {
    public:
        Builder();
        ~Builder();

        BuilderDetails* operator->() noexcept { return m_impl; }
        const BuilderDetails* operator->() const noexcept { return m_impl; }

        Builder& uniformBlock(uint32_t binding, std::string_view blockName, size_t totalSize);

        Builder& uniformMember(std::string_view blockName, std::string_view memberName,
                               UniformType type, size_t offset, size_t size);

        Material* build(Engine& engine);

    private:
        BuilderDetails* m_impl = nullptr;
    };

    Material(Engine& engine, const Builder& builder);
    ~Material();

    void terminate(Engine& engine);

    std::shared_ptr<MaterialInstance> createInstance();

    const ocf::DescriptorSetLayout& getDescriptorSetLayout() const { return m_descriptorSetLayout; }

private:
    ocf::DescriptorSetLayout m_descriptorSetLayout;
};

} // namespace ocf
