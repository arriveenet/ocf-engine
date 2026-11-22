// SPDX-License-Identifier: MIT
#include "ocf/rhi/d3d12/D3D12CommandBuffer.h"

#include "ocf/rhi/d3d12/D3D12Device.h"

namespace ocf::rhi {

D3D12CommandBuffer::D3D12CommandBuffer(D3D12Device& device)
    : m_device(device)
{
}

D3D12CommandBuffer::~D3D12CommandBuffer() = default;

void D3D12CommandBuffer::begin() {}

void D3D12CommandBuffer::end() {}

void D3D12CommandBuffer::reset() {}

void D3D12CommandBuffer::beginRendering(RenderTargetHandle, const RenderingInfo&) {}

void D3D12CommandBuffer::endRendering() {}

void D3D12CommandBuffer::bindPipeline(PipelineHandle) {}

void D3D12CommandBuffer::bindDescriptorSets(PipelineHandle, DescriptorSetHandle, uint32_t,
                                            uint32_t)
{
}

void D3D12CommandBuffer::pushConstants(PipelineHandle, ShaderStageFlags, uint32_t, uint32_t,
                                       const void*)
{
}

void D3D12CommandBuffer::bindVertexBuffers(uint32_t, uint32_t, VertexBufferHandle) {}

void D3D12CommandBuffer::bindIndexBuffer(IndexBufferHandle, uint32_t) {}

void D3D12CommandBuffer::draw(uint32_t, uint32_t, uint32_t, uint32_t) {}

void D3D12CommandBuffer::drawIndexed(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {}

void D3D12CommandBuffer::transitionLayout(ResourceState, ResourceState) {}

void D3D12CommandBuffer::transitionLayout(TextureHandle, ResourceState, ResourceState) {}

} // namespace ocf::rhi
