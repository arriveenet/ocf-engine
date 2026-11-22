// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/rhi/Device.h"
#include "ocf/rhi/HandleAllocator.h"

#include <memory>

namespace ocf::rhi {

class D3D12CommandBuffer;

/**
 * @brief Device of the Direct3D 12 backend.
 *
 * @note Not implemented yet. Resources only get handles, and no frame is ever rendered:
 *       beginFrame() always returns false.
 */
class D3D12Device : public Device {
public:
    static const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    explicit D3D12Device(const DeviceConfig& config);
    ~D3D12Device() override;

    bool initialize();
    void terminate();

    VertexBufferInfoHandle createVertexBufferInfo(uint8_t attributeCount,
                                                  AttributeArray attributes) override;

    VertexBufferHandle createVertexBuffer(uint32_t bufferSize, BufferUsage usage,
                                          VertexBufferInfoHandle vbih) override;

    IndexBufferHandle createIndexBuffer(ElementType elementType, uint32_t indexCount,
                                        BufferUsage usage) override;

    BufferObjectHandle createBufferObject(BufferType type, uint32_t byteCount) override;

    TextureHandle createTexture(SamplerType target, uint8_t levels, TextureFormat format,
                                uint32_t width, uint32_t height, uint32_t depth,
                                TextureUsage usage = TextureUsage::Default) override;

    ShaderModuleHandle createShaderModule(ShaderStage stage, std::string_view filename,
                                          const char* entryPoint = "main") override;

    DescriptorSetLayoutHandle createDescriptorLayoutSet(const DescriptorSetLayout& layout) override;

    DescriptorSetHandle createDescriptorSet(DescriptorSetLayoutHandle dslh) override;

    PipelineHandle createPipeline(const PipelineState& pipeline) override;

    SwapchainHandle createSwapchain(Window* window, uint32_t width, uint32_t height,
                                    bool vsync) override;

    TextureHandle createDepthBuffer(uint32_t width, uint32_t height) override;

    RenderTargetHandle createRenderTarget(uint32_t width, uint32_t height, TargetBufferInfo color,
                                          TargetBufferInfo depth) override;

    RenderTargetHandle createDefaultRenderTarget() override;

    void destroyVertexBufferInfo(VertexBufferInfoHandle handle) override;

    void destroyVertexBuffer(VertexBufferHandle handle) override;

    void destroyIndexBuffer(IndexBufferHandle handle) override;

    void destroyBufferObject(BufferObjectHandle handle) override;

    void destroyTexture(TextureHandle handle) override;

    void destroyShaderModule(ShaderModuleHandle handle) override;

    void destroyDescriptorSetLayout(DescriptorSetLayoutHandle handle) override;

    void destroyDescriptorSet(DescriptorSetHandle handle) override;

    void destroyPipeline(PipelineHandle handle) override;

    void destroyRenderTarget(RenderTargetHandle handle) override;

    void updateBufferData(VertexBufferHandle handle, const void* data, size_t size,
                          size_t offset) override;

    void updateIndexBufferData(IndexBufferHandle handle, const void* data, size_t size,
                               size_t offset) override;

    void updateBufferObject(BufferObjectHandle handle, const void* data, size_t size,
                            size_t offset) override;

    void updateDescriptorSet(DescriptorSetHandle handle, BufferObjectHandle buffer,
                             size_t offset) override;

    void updateDescriptorSetTexture(DescriptorSetHandle handle, TextureHandle texture,
                                    const SamplerParameters& sampler, uint32_t binding) override;

    void updateTextureImage(TextureHandle handle, uint8_t level, uint32_t xoffset,
                            uint32_t yoffset, uint32_t zoffset, uint32_t width, uint32_t height,
                            uint32_t depth, PixelBufferDescriptor&& data) override;

    void generateMipmaps(TextureHandle handle) override;

    std::shared_ptr<CommandBuffer> getCommandBuffer() override;

    uint32_t getCurrentFrameIndex() override;

    bool beginFrame() override;

    void endFrame() override;

private:
    template <typename D, typename... ARGS>
    Handle<D> initHandle(ARGS&&... args)
    {
        return m_handleAllocator.allocateAndConstruct<D>(std::forward<ARGS>(args)...);
    }

    template <typename D>
    void destroyHandle(Handle<D> handle) noexcept
    {
        if (handle) {
            m_handleAllocator.deallocate(handle, m_handleAllocator.handle_cast<D*>(handle));
        }
    }

    HandleAllocatorDX12 m_handleAllocator;
    std::shared_ptr<D3D12CommandBuffer> m_commandBuffer;
};

} // namespace ocf::rhi
