// SPDX-License-Identifier: MIT
#include "ocf/renderer/DescriptorSet.h"

#include "ocf/core/Engine.h"
#include "ocf/renderer/DescriptorSetLayout.h"
#include "ocf/renderer/Texture.h"
#include "ocf/renderer/TextureSampler.h"
#include "ocf/rhi/Device.h"

#include <cstring>

namespace ocf {

void DescriptorSet::create(Engine& engine, const DescriptorSetLayout& layout)
{
    Engine::Device& device = engine.getDevice();

    // Get all uniform blocks from the layout
    const auto& blocks = layout.getBlocks();

    // Initialize buffers for each binding
    for (const auto& [blockName, blockInfo] : blocks) {
        uint32_t binding = blockInfo.binding;
        m_buffers[binding].resize(blockInfo.totalSize, 0);
        m_dirtyFlags[binding] = DirtyFlags{};
    }

    // Create buffers and descriptor sets for each frame (double buffering)
    for (size_t frameIndex = 0; frameIndex < FRAME_COUNT; ++frameIndex) {
        // Create uniform buffer object for each binding
        for (const auto& [blockName, blockInfo] : blocks) {
            uint32_t binding = blockInfo.binding;
            
            m_uniformBuffers[binding][frameIndex] = device.createBufferObject(
                rhi::BufferType::Uniform,
                static_cast<uint32_t>(blockInfo.totalSize)
            );
        }

        // Create descriptor set
        m_descriptorSets[frameIndex] = device.createDescriptorSet(layout.getHandle());

        // Bind uniform buffers to descriptor sets
        for (const auto& [blockName, blockInfo] : blocks) {
            uint32_t binding = blockInfo.binding;
            device.updateDescriptorSet(
                m_descriptorSets[frameIndex],
                m_uniformBuffers[binding][frameIndex],
                0
            );
        }
    }
}

void DescriptorSet::terminate(Engine& engine)
{
    Engine::Device& device = engine.getDevice();

    // Destroy uniform buffers
    for (auto& [binding, buffers] : m_uniformBuffers) {
        for (auto& buffer : buffers) {
            device.destroyBufferObject(buffer);
        }
    }

    // Destroy descriptor sets
    for (auto& descriptorSet : m_descriptorSets) {
        device.destroyDescriptorSet(descriptorSet);
    }
}

void DescriptorSet::uploadUniformBuffer(uint32_t binding, size_t offset, const void* data, size_t size)
{
    auto it = m_buffers.find(binding);
    if (it == m_buffers.end()) {
        return;
    }

    if (offset + size > it->second.size()) {
        return;
    }

    // Copy data to internal buffer at specified offset
    std::memcpy(it->second.data() + offset, data, size);
    
    // Every frame's copy has to be updated
    m_dirtyFlags[binding].fill(true);
}

void DescriptorSet::updateTextureDescriptor(uint32_t binding, Texture* texture, const TextureSampler& sampler)
{
    if (!texture) {
        return;
    }

    // A descriptor set used by a pending command buffer must not be written, so commit()
    // writes each frame's copy when that frame is drawn
    TextureBinding& textureBinding = m_textures[binding];
    textureBinding.texture = texture->getHandle();
    textureBinding.sampler = sampler.getParams();
    textureBinding.dirty.fill(true);
}

void DescriptorSet::commit(Engine& engine, uint32_t frameIndex)
{
    Engine::Device& device = engine.getDevice();

    // Upload the buffers changed since this frame's copy was last written
    for (auto& [binding, dirty] : m_dirtyFlags) {
        if (dirty[frameIndex]) {
            auto& buffer = m_buffers[binding];
            auto& uniformBuffer = m_uniformBuffers[binding][frameIndex];
            
            device.updateBufferObject(
                uniformBuffer,
                buffer.data(),
                buffer.size(),
                0
            );
            dirty[frameIndex] = false;
        }
    }

    for (auto& [binding, textureBinding] : m_textures) {
        if (textureBinding.dirty[frameIndex]) {
            device.updateDescriptorSetTexture(m_descriptorSets[frameIndex], textureBinding.texture,
                                              textureBinding.sampler, binding);
            textureBinding.dirty[frameIndex] = false;
        }
    }
}

} // namespace ocf
