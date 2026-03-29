// SPDX - License - Identifier : MIT

#pragma once

#include "ocf/rhi/Handle.h"

namespace ocf {
namespace rhi {

class Context;

struct RHIBase {};

struct RHITexture : public RHIBase {
};

struct RHIVertexBuffer : public RHIBase {
};

class Device {
public:
    Device();
    virtual ~Device();

    virtual TextureHandle createTexture() = 0;

protected:
};

} // namespace rhi
} // namespace ocf
