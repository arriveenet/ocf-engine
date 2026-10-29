// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/compiler.h"
#include "ocf/rhi/Handle.h"
#include "ocf/rhi/RHIEnums.h"

#include <array>
#include <unordered_map>
#include <vector>

namespace ocf {

class Engine;
class DescriptorSetLayout;
class Texture;
class TextureSampler;

/**
 * @brief A descriptor set and its uniform buffers, one copy per frame in flight.
 *
 * Changes are only recorded until commit() writes them to the copy of a frame, since the
 * copies of other frames may still be in use by the GPU.
 */
class OCF_API DescriptorSet {
public:
    using DescriptorSetHandle = rhi::DescriptorSetHandle;
    using BufferObjectHandle = rhi::BufferObjectHandle;
    using TextureHandle = rhi::TextureHandle;

    static constexpr size_t FRAME_COUNT = 2;

    DescriptorSet() = default;

    void create(Engine& engine, const DescriptorSetLayout& layout);

    void terminate(Engine& engine);

    void uploadUniformBuffer(uint32_t binding, size_t offset, const void* data, size_t size);

    void updateTextureDescriptor(uint32_t binding, Texture* texture, const TextureSampler& sampler);

    /**
     * @brief Write the changes to the copy of a frame.
     *
     * Call only when the GPU is done with that copy, i.e. after the frame's fence was waited.
     */
    void commit(Engine& engine, uint32_t frameIndex);

    DescriptorSetHandle getDescriptorSetHandle(size_t index) const { return m_descriptorSets.at(index); }

private:
    using DirtyFlags = std::array<bool, FRAME_COUNT>; // Per frame: not written to its copy yet

    struct TextureBinding {
        TextureHandle texture;
        rhi::SamplerParameters sampler;
        DirtyFlags dirty{};
    };

    std::array<DescriptorSetHandle, FRAME_COUNT> m_descriptorSets;

    // Map: binding -> array of BufferObjectHandle for each frame
    std::unordered_map<uint32_t, std::array<BufferObjectHandle, FRAME_COUNT>> m_uniformBuffers;

    std::unordered_map<uint32_t, std::vector<uint8_t>> m_buffers;
    std::unordered_map<uint32_t, DirtyFlags> m_dirtyFlags;
    std::unordered_map<uint32_t, TextureBinding> m_textures;
};

} // namespace ocf
