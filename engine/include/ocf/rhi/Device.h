// SPDX-License-Identifier: MIT

#pragma once

#include "Handle.h"
#include "ocf/rhi/Handle.h"
#include "ocf/rhi/PipelineState.h"
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

class Device {
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

    virtual TextureHandle createTexture() = 0;

    virtual ShaderModuleHandle createShaderModule(ShaderStage stage, std::string_view filename,
                                                  const char* entryPoint = "main") = 0;

    virtual DescriptorSetLayoutHandle createDescriptorLayoutSet(const DescriptorSetLayout& layout) = 0;

    virtual DescriptorSetHandle createDescriptorSet(DescriptorSetLayoutHandle dslh) = 0;

    virtual PipelineHandle createPipeline(const PipelineState& pipeline) = 0;

    virtual SwapchainHandle createSwapchain(Window* window, uint32_t width, uint32_t height) = 0;

    virtual void createDepthBuffer(uint32_t width, uint32_t height) = 0;

    virtual void destroyVertexBufferInfo(VertexBufferInfoHandle handle) = 0;

    virtual void destroyVertexBuffer(VertexBufferHandle handle) = 0;

    virtual void destroyIndexBuffer(IndexBufferHandle handle) = 0;

    virtual void destroyBufferObject(BufferObjectHandle handle) = 0;

    virtual void destroyDescriptorSetLayout(DescriptorSetLayoutHandle handle) = 0;

    virtual void destroyDescriptorSet(DescriptorSetHandle handle) = 0;

    virtual void destroyPipeline(PipelineHandle handle) = 0;

    virtual void updateBufferData(VertexBufferHandle handle, const void* data, size_t size,
                                  size_t offset) = 0;

    virtual void updateIndexBufferData(IndexBufferHandle handle, const void* data, size_t size,
                                       size_t offset) = 0;

    virtual void updateBufferObject(BufferObjectHandle handle, const void* data, size_t size,
                                    size_t offset) = 0;

    virtual void updateDescriptorSet(DescriptorSetHandle handle, BufferObjectHandle buffer,
                                     size_t offset) = 0;

    virtual std::shared_ptr<CommandBuffer> getCommandBuffer() = 0;

    // @TODO
    virtual uint32_t getCurrentFrameIndex() = 0;

    virtual void beginFrame() = 0;

    virtual void endFrame() = 0;

protected:
};

} // namespace ocf::rhi
