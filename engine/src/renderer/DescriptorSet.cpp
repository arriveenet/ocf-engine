// SPDX-License-Identifier: MIT
#include "DescriptorSet.h"

#include "DescriptorSetLayout.h"

#include "ocf/core/Engine.h"
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
        m_dirtyFlags[binding] = false;
    }

    // Create buffers and descriptor sets for each frame (double buffering)
    for (size_t frameIndex = 0; frameIndex < 2; ++frameIndex) {
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
    
    // Mark as dirty
    m_dirtyFlags[binding] = true;
}

void DescriptorSet::updateTextureDescriptor(Engine& engine, uint32_t binding, Texture* texture, const TextureSampler& sampler)
{ 
    if (!texture) {
        return;
    }

    Engine::Device& device = engine.getDevice();
    device.updateDescriptorSetTexture(m_descriptorSets[0], texture->getHandle(), sampler.getParams(), binding);
    device.updateDescriptorSetTexture(m_descriptorSets[1], texture->getHandle(), sampler.getParams(), binding);
}

void DescriptorSet::commit(Engine& engine, uint32_t frameIndex)
{
    Engine::Device& device = engine.getDevice();

    // Upload all dirty buffers to GPU for the current frame
    for (auto& [binding, isDirty] : m_dirtyFlags) {
        if (isDirty) {
            auto& buffer = m_buffers[binding];
            auto& uniformBuffer = m_uniformBuffers[binding][frameIndex];
            
            device.updateBufferObject(
                uniformBuffer,
                buffer.data(),
                buffer.size(),
                0
            );
            isDirty = false;
        }
    }
}

} // namespace ocf
