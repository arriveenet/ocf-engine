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

struct RHIResourceBase {};

struct RHIVertexBuffer : public RHIResourceBase {};

struct RHITexture : public RHIResourceBase {
    uint16_t width;
    uint16_t height;
};

struct RHIShaderModule : public RHIResourceBase {
    ShaderStage stage;
    const char* entryPoint;
};

struct RHISwapchain : public RHIResourceBase {};

class Device {
public:
    struct DeviceConfig {
        size_t handlePoolSize = 0;
    };

    Device();
    virtual ~Device();

    virtual TextureHandle createTexture() = 0;

    virtual ShaderModuleHandle createShaderModule(std::string_view filename) = 0;

    virtual SwapchainHandle createSwapchain(Window* window, uint32_t width, uint32_t height) = 0;

    virtual std::shared_ptr<CommandBuffer> getCommandBuffer() = 0;

    virtual void beginFrame() = 0;

    virtual void endFrame() = 0;

protected:
};

} // namespace ocf::rhi
