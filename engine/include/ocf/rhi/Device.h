// SPDX-License-Identifier: MIT

#pragma once

#include "ocf/rhi/Handle.h"

namespace ocf {
class Window;
}

namespace ocf::rhi {

class Context;

struct RHIResourceBase {};

struct RHITexture : public RHIResourceBase {};

struct RHIVertexBuffer : public RHIResourceBase {};

struct RHISwapchain : public RHIResourceBase {};

class Device {
public:
    Device();
    virtual ~Device();

    virtual TextureHandle createTexture() = 0;

    virtual SwapchainHandle createSwapchain(Window* window, uint32_t width, uint32_t height) = 0;

protected:
};

} // namespace ocf::rhi
