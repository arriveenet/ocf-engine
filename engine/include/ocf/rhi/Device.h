// SPDX-License-Identifier: MIT

#pragma once

#include "ocf/rhi/Handle.h"
#include "ocf/rhi/RHIEnums.h"

#include <cstdint>
#include <string_view>

namespace ocf {
class Window;
}

namespace ocf::rhi {

class Context;

struct RHIResourceBase {};

struct RHIVertexBuffer : public RHIResourceBase {};

struct RHITexture : public RHIResourceBase {
    uint16_t width;
    uint16_t height;
};

struct RHIShader : public RHIResourceBase {
    ShaderStage stage;
    const char* entryPoint;
};

struct RHISwapchain : public RHIResourceBase {};

class Device {
public:
    Device();
    virtual ~Device();

    virtual TextureHandle createTexture() = 0;

    virtual ShaderHandle createShader(std::string_view filename) = 0;

    virtual SwapchainHandle createSwapchain(Window* window, uint32_t width, uint32_t height) = 0;

protected:
};

} // namespace ocf::rhi
