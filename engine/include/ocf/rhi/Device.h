// SPDX - License - Identifier : MIT

#pragma once

#include "ocf/rhi/Handle.h"

namespace ocf {
namespace rhi {

class Context;

class IWindow;

struct RHIBase {};

struct RHITexture : public RHIBase {
};

struct RHIVertexBuffer : public RHIBase {
};

struct RHISwapchain : public RHIBase {
};

class Device {
public:
    Device();
    virtual ~Device();

    virtual TextureHandle createTexture() = 0;

    virtual SwapchainHandle createSwapchain(uint32_t width, uint32_t height) = 0;

protected:
};

} // namespace rhi
} // namespace ocf
