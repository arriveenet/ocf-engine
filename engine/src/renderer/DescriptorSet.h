// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/rhi/Handle.h"

#include <array>

namespace ocf {

class DescriptorSet {
public:
    using DescriptorSetHandle = rhi::DescriptorSetHandle;
    using BufferObjectHandle = rhi::BufferObjectHandle;

    DescriptorSet() = default;

    void create();

    void uploadUniformBuffer(uint32_t binding, const void* data, size_t size);

    DescriptorSetHandle getDescriptorSetHandle(size_t index) const { return m_descriptorSets.at(index); }

    BufferObjectHandle getBufferObjectHandle(size_t index) const
    {
        return m_uniformBuffers.at(index);
    }

private:
    std::array<DescriptorSetHandle, 2> m_descriptorSets;
    std::array<BufferObjectHandle, 2> m_uniformBuffers;

    std::unordered_map<uint32_t, std::vector<uint8_t>> m_buffers;
    std::unordered_map<uint32_t, bool> m_dirtyFlags;
};

} // namespace ocf
