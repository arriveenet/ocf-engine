// SPDX-License-Identifier: MIT

#pragma once

#include "ocf/rhi/Handle.h"
#include "ocf/rhi/RHIEnums.h"

#include <cstdint>
#include <string_view>
#include <memory>

namespace ocf {
class Window;
}

namespace ocf::rhi {

class Context;
class CommandBuffer;
class Swapchain;

struct RHIResourceBase {};

struct RHIVertexBufferInfo : public RHIResourceBase {
    uint8_t attributeCount;
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

struct RHITexture : public RHIResourceBase {
    uint16_t width;
    uint16_t height;
};

struct RHIShaderModule : public RHIResourceBase {
    ShaderStage stage;
    const char* entryPoint;
};

struct RHISwapchain : public RHIResourceBase {
};

class Device {
public:
    struct DeviceConfig {
        size_t handlePoolSize = 0;
    };

    Device();
    virtual ~Device();

    virtual VertexBufferInfoHandle createVertexBufferInfo(uint8_t attributeCount,
                                                          AttributeArray attributes) = 0;

    virtual VertexBufferHandle createVertexBuffer(uint32_t vertexCount, uint32_t byteCount,
                                                  BufferUsage usage,
                                                  VertexBufferInfoHandle vbih) = 0;

    virtual TextureHandle createTexture() = 0;

    virtual ShaderModuleHandle createShaderModule(ShaderStage stage, std::string_view filename,
                                                  const char* entryPoint = "main") = 0;

    virtual SwapchainHandle createSwapchain(Window* window, uint32_t width, uint32_t height) = 0;

    virtual void destroyVertexBuffer(VertexBufferHandle handle) = 0;

    virtual void updateBufferData(VertexBufferHandle handle, const void* data, size_t size,
                                  size_t offset) = 0;

    virtual std::shared_ptr<CommandBuffer> getCommandBuffer() = 0;

    virtual void beginFrame() = 0;

    virtual void endFrame() = 0;

protected:
};

} // namespace ocf::rhi
