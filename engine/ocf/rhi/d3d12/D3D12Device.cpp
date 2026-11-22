// SPDX-License-Identifier: MIT
#include "ocf/rhi/d3d12/D3D12Device.h"

#include "ocf/core/Logger.h"
#include "ocf/rhi/d3d12/D3D12CommandBuffer.h"

namespace ocf::rhi {

D3D12Device::D3D12Device(const DeviceConfig& config)
    : m_handleAllocator("Handles", config.handlePoolSize)
{
}

D3D12Device::~D3D12Device()
{
    terminate();
}

bool D3D12Device::initialize()
{
    OCF_LOG_WARN("The Direct3D 12 backend is not implemented yet: nothing will be rendered");

    m_commandBuffer = std::make_shared<D3D12CommandBuffer>(*this);
    return true;
}

void D3D12Device::terminate()
{
    m_commandBuffer.reset();
}

VertexBufferInfoHandle D3D12Device::createVertexBufferInfo(uint8_t attributeCount,
                                                           AttributeArray /*attributes*/)
{
    return initHandle<RHIVertexBufferInfo>(attributeCount);
}

VertexBufferHandle D3D12Device::createVertexBuffer(uint32_t bufferSize, BufferUsage /*usage*/,
                                                   VertexBufferInfoHandle /*vbih*/)
{
    return initHandle<RHIVertexBuffer>(0u, bufferSize);
}

IndexBufferHandle D3D12Device::createIndexBuffer(ElementType elementType, uint32_t indexCount,
                                                 BufferUsage /*usage*/)
{
    const auto elementSize = static_cast<uint8_t>(getElementTypeSize(elementType));
    return initHandle<RHIIndexBuffer>(elementSize, indexCount);
}

BufferObjectHandle D3D12Device::createBufferObject(BufferType /*type*/, uint32_t byteCount)
{
    return initHandle<RHIBufferObject>(byteCount);
}

TextureHandle D3D12Device::createTexture(SamplerType /*target*/, uint8_t /*levels*/,
                                         TextureFormat /*format*/, uint32_t width,
                                         uint32_t height, uint32_t /*depth*/,
                                         TextureUsage /*usage*/)
{
    TextureHandle handle = initHandle<RHITexture>();
    RHITexture* texture = m_handleAllocator.handle_cast<RHITexture*>(handle);
    texture->width = static_cast<uint16_t>(width);
    texture->height = static_cast<uint16_t>(height);
    return handle;
}

ShaderModuleHandle D3D12Device::createShaderModule(ShaderStage stage,
                                                   std::string_view /*filename*/,
                                                   const char* entryPoint)
{
    ShaderModuleHandle handle = initHandle<RHIShaderModule>();
    RHIShaderModule* module = m_handleAllocator.handle_cast<RHIShaderModule*>(handle);
    module->stage = stage;
    module->entryPoint = entryPoint;
    return handle;
}

DescriptorSetLayoutHandle D3D12Device::createDescriptorLayoutSet(
    const DescriptorSetLayout& /*layout*/)
{
    return initHandle<RHIDescriptorSetLayout>();
}

DescriptorSetHandle D3D12Device::createDescriptorSet(DescriptorSetLayoutHandle /*dslh*/)
{
    return initHandle<RHIDescriptorSet>();
}

PipelineHandle D3D12Device::createPipeline(const PipelineState& /*pipeline*/)
{
    return initHandle<RHIPipeline>();
}

SwapchainHandle D3D12Device::createSwapchain(Window* /*window*/, uint32_t /*width*/,
                                             uint32_t /*height*/, bool /*vsync*/)
{
    return initHandle<RHISwapchain>();
}

TextureHandle D3D12Device::createDepthBuffer(uint32_t width, uint32_t height)
{
    return createTexture(SamplerType::Sampler2D, 1, TextureFormat::DEPTH32F, width, height, 1,
                         TextureUsage::DepthAttachment);
}

RenderTargetHandle D3D12Device::createRenderTarget(uint32_t width, uint32_t height,
                                                   TargetBufferInfo /*color*/,
                                                   TargetBufferInfo /*depth*/)
{
    return initHandle<RHIRenderTarget>(width, height);
}

RenderTargetHandle D3D12Device::createDefaultRenderTarget()
{
    return initHandle<RHIRenderTarget>();
}

void D3D12Device::destroyVertexBufferInfo(VertexBufferInfoHandle handle)
{
    destroyHandle(handle);
}

void D3D12Device::destroyVertexBuffer(VertexBufferHandle handle)
{
    destroyHandle(handle);
}

void D3D12Device::destroyIndexBuffer(IndexBufferHandle handle)
{
    destroyHandle(handle);
}

void D3D12Device::destroyBufferObject(BufferObjectHandle handle)
{
    destroyHandle(handle);
}

void D3D12Device::destroyTexture(TextureHandle handle)
{
    destroyHandle(handle);
}

void D3D12Device::destroyShaderModule(ShaderModuleHandle handle)
{
    destroyHandle(handle);
}

void D3D12Device::destroyDescriptorSetLayout(DescriptorSetLayoutHandle handle)
{
    destroyHandle(handle);
}

void D3D12Device::destroyDescriptorSet(DescriptorSetHandle handle)
{
    destroyHandle(handle);
}

void D3D12Device::destroyPipeline(PipelineHandle handle)
{
    destroyHandle(handle);
}

void D3D12Device::destroyRenderTarget(RenderTargetHandle handle)
{
    destroyHandle(handle);
}

void D3D12Device::updateBufferData(VertexBufferHandle, const void*, size_t, size_t) {}

void D3D12Device::updateIndexBufferData(IndexBufferHandle, const void*, size_t, size_t) {}

void D3D12Device::updateBufferObject(BufferObjectHandle, const void*, size_t, size_t) {}

void D3D12Device::updateDescriptorSet(DescriptorSetHandle, BufferObjectHandle, size_t) {}

void D3D12Device::updateDescriptorSetTexture(DescriptorSetHandle, TextureHandle,
                                             const SamplerParameters&, uint32_t)
{
}

void D3D12Device::updateTextureImage(TextureHandle, uint8_t, uint32_t, uint32_t, uint32_t,
                                     uint32_t, uint32_t, uint32_t, PixelBufferDescriptor&& data)
{
    // Not uploaded yet: dropping the data calls its release callback
    [[maybe_unused]] PixelBufferDescriptor dropped = std::move(data);
}

void D3D12Device::generateMipmaps(TextureHandle) {}

std::shared_ptr<CommandBuffer> D3D12Device::getCommandBuffer()
{
    return m_commandBuffer;
}

uint32_t D3D12Device::getCurrentFrameIndex()
{
    return 0;
}

bool D3D12Device::beginFrame()
{
    // No swapchain yet, so no frame can be rendered
    return false;
}

void D3D12Device::endFrame() {}

} // namespace ocf::rhi
