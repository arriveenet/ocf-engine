// SPDX-License-Identifier: MIT

#pragma once

#include "ocf/rhi/RHIEnums.h"

namespace ocf::rhi {

class CommandBuffer {
public:
    virtual ~CommandBuffer() = default;

    virtual void begin() = 0;

    virtual void end() = 0;

    virtual void reset() = 0;

    virtual void beginRendering(const RenderingInfo& info) = 0;

    virtual void endRendering() = 0;

    virtual void transitionLayout(ResourceState oldState, ResourceState newState) = 0;
};

} // namespace ocf::rhi
