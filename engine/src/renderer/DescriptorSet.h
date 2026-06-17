// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/rhi/Handle.h"

#include <array>
#include <unordered_map>
#include <vector>

namespace ocf {

class Engine;
class DescriptorSetLayout;
class Texture;
class TextureSampler;

class DescriptorSet {
public:
    using DescriptorSetHandle = rhi::DescriptorSetHandle;
    using BufferObjectHandle = rhi::BufferObjectHandle;
    using TextureHandle = rhi::TextureHandle;

    DescriptorSet() = default;

    void create(Engine& engine, const DescriptorSetLayout& layout);

    void uploadUniformBuffer(uint32_t binding, size_t offset, const void* data, size_t size);

    void updateTextureDescriptor(Engine& engine, uint32_t binding, Texture* texture, const TextureSampler& sampler);

    void commit(Engine& engine, uint32_t frameIndex);

    DescriptorSetHandle getDescriptorSetHandle(size_t index) const { return m_descriptorSets.at(index); }

private:
    std::array<DescriptorSetHandle, 2> m_descriptorSets;
    
    // Map: binding -> array of BufferObjectHandle for each frame
    std::unordered_map<uint32_t, std::array<BufferObjectHandle, 2>> m_uniformBuffers;

    std::unordered_map<uint32_t, std::vector<uint8_t>> m_buffers;
    std::unordered_map<uint32_t, bool> m_dirtyFlags;
};

} // namespace ocf
