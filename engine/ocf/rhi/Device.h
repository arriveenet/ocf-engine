// SPDX-License-Identifier: MIT

#pragma once

#include "ocf/core/compiler.h"
#include "Handle.h"
#include "ocf/rhi/Handle.h"
#include "ocf/rhi/PipelineState.h"
#include "ocf/rhi/PixelBufferDescriptor.h"
#include "ocf/rhi/RHIEnums.h"

#include <cstdint>
#include <memory>
#include <string_view>

namespace ocf {
class Window;
}

namespace ocf::rhi {

class Context;
class CommandBuffer;
class Swapchain;

struct RHIResourceBase {};

struct RHIVertexBufferInfo : public RHIResourceBase {
    uint8_t attributeCount = 0;
    bool padding[3] = {};

    RHIVertexBufferInfo() noexcept = default;
    RHIVertexBufferInfo(uint8_t attributeCount)
        : attributeCount(attributeCount)
    {
    }
};

struct RHIVertexBuffer : public RHIResourceBase {
    uint32_t vertexCount = 0;
    uint32_t byteCount = 0;
    uint8_t bufferObjectVertion = 0xff;
    bool padding[3] = {};

    RHIVertexBuffer() noexcept = default;
    RHIVertexBuffer(uint32_t vertexCount, uint32_t byteCount)
        : vertexCount(vertexCount)
        , byteCount(byteCount)
    {
    }
};

struct RHIIndexBuffer : public RHIResourceBase {
    uint32_t count : 27;
    uint32_t elementSize : 5;

    RHIIndexBuffer() noexcept
        : count{}
        , elementSize{}
    {
    }
    RHIIndexBuffer(uint8_t elementSize, uint32_t indexCount)
        : count(indexCount)
        , elementSize(elementSize)
    {
        assert(elementSize > 0 && elementSize <= 16);
        assert(indexCount < (1u << 27));
    }
};

struct RHIBufferObject : public RHIResourceBase {
    uint32_t byteCount;

    RHIBufferObject() noexcept = default;
    RHIBufferObject(uint32_t byteCount) :byteCount(byteCount) {}
};


struct RHITexture : public RHIResourceBase {
    uint16_t width;
    uint16_t height;
};

struct RHIShaderModule : public RHIResourceBase {
    ShaderStage stage;
    const char* entryPoint;
};

struct RHIDescriptorSetLayout : public RHIResourceBase {
};

struct RHIDescriptorSet : public RHIResourceBase {
};

struct RHIPipeline : public RHIResourceBase {
};

struct RHISwapchain : public RHIResourceBase {
};

struct RHIRenderTarget : public RHIResourceBase {
    uint32_t width = 0;
    uint32_t height = 0;

    RHIRenderTarget() noexcept = default;
    RHIRenderTarget(uint32_t width, uint32_t height)
        : width(width)
        , height(height)
    {
    }
};

/** @brief A texture attached to a render target. A null handle leaves the attachment unused. */
struct TargetBufferInfo {
    TextureHandle handle;
    uint8_t level = 0; ///< Mipmap level rendered to. Only level 0 is supported for now.
};

class OCF_API Device {
public:
    struct DeviceConfig {
        size_t handlePoolSize = 0;
    };

    static size_t getElementTypeSize(ElementType type);

    Device();
    virtual ~Device();

    virtual VertexBufferInfoHandle createVertexBufferInfo(uint8_t attributeCount,
                                                          AttributeArray attributes) = 0;

    virtual VertexBufferHandle createVertexBuffer(uint32_t bufferSize, BufferUsage usage,
                                                  VertexBufferInfoHandle vbih) = 0;

    virtual IndexBufferHandle createIndexBuffer(ElementType elementType, uint32_t indexCount,
                                                BufferUsage usage) = 0;

    virtual BufferObjectHandle createBufferObject(BufferType type, uint32_t byteCount) = 0;

    virtual TextureHandle createTexture(SamplerType target, uint8_t levels, TextureFormat format,
                                        uint32_t width, uint32_t height, uint32_t depth,
                                        TextureUsage usage = TextureUsage::Default) = 0;

    virtual ShaderModuleHandle createShaderModule(ShaderStage stage, std::string_view filename,
                                                  const char* entryPoint = "main") = 0;

    virtual DescriptorSetLayoutHandle createDescriptorLayoutSet(const DescriptorSetLayout& layout) = 0;

    virtual DescriptorSetHandle createDescriptorSet(DescriptorSetLayoutHandle dslh) = 0;

    virtual PipelineHandle createPipeline(const PipelineState& pipeline) = 0;

    virtual SwapchainHandle createSwapchain(Window* window, uint32_t width, uint32_t height,
                                            bool vsync) = 0;

    virtual TextureHandle createDepthBuffer(uint32_t width, uint32_t height) = 0;

    /**
     * @brief Create a render target that draws into textures.
     *
     * The textures must have the matching attachment usage and are not owned by the render target.
     * Either attachment may be left unused, e.g. a depth-only target for a shadow map.
     */
    virtual RenderTargetHandle createRenderTarget(uint32_t width, uint32_t height,
                                                  TargetBufferInfo color,
                                                  TargetBufferInfo depth) = 0;

    /** @brief Create the render target that draws into the swapchain and its depth buffer. */
    virtual RenderTargetHandle createDefaultRenderTarget() = 0;

    virtual void destroyVertexBufferInfo(VertexBufferInfoHandle handle) = 0;

    virtual void destroyVertexBuffer(VertexBufferHandle handle) = 0;

    virtual void destroyIndexBuffer(IndexBufferHandle handle) = 0;

    virtual void destroyBufferObject(BufferObjectHandle handle) = 0;

    virtual void destroyTexture(TextureHandle handle) = 0;

    virtual void destroyShaderModule(ShaderModuleHandle handle) = 0;

    virtual void destroyDescriptorSetLayout(DescriptorSetLayoutHandle handle) = 0;

    virtual void destroyDescriptorSet(DescriptorSetHandle handle) = 0;

    virtual void destroyPipeline(PipelineHandle handle) = 0;

    virtual void destroyRenderTarget(RenderTargetHandle handle) = 0;

    virtual void updateBufferData(VertexBufferHandle handle, const void* data, size_t size,
                                  size_t offset) = 0;

    virtual void updateIndexBufferData(IndexBufferHandle handle, const void* data, size_t size,
                                       size_t offset) = 0;

    virtual void updateBufferObject(BufferObjectHandle handle, const void* data, size_t size,
                                    size_t offset) = 0;

    virtual void updateDescriptorSet(DescriptorSetHandle handle, BufferObjectHandle buffer,
                                     size_t offset) = 0;

    virtual void updateDescriptorSetTexture(DescriptorSetHandle handle, TextureHandle texture,
                                            const SamplerParameters& sampler, uint32_t binding) = 0;

    virtual void updateTextureImage(TextureHandle handle, uint8_t level, uint32_t xoffset,
                                    uint32_t yoffset, uint32_t zoffset, uint32_t width,
                                    uint32_t height, uint32_t depth,
                                    PixelBufferDescriptor&& data) = 0;

    virtual void generateMipmaps(TextureHandle handle) = 0;

    virtual std::shared_ptr<CommandBuffer> getCommandBuffer() = 0;

    // @TODO
    virtual uint32_t getCurrentFrameIndex() = 0;

    /**
     * @brief Acquire the swapchain image of the frame.
     *
     * The swapchain follows the window by itself: it is recreated with the depth buffer when
     * the surface size changes or the swapchain is out of date. Their handles stay the same.
     *
     * @return false if the frame cannot be rendered (no swapchain image was acquired)
     */
    virtual bool beginFrame() = 0;

    virtual void endFrame() = 0;

protected:
};

} // namespace ocf::rhi
