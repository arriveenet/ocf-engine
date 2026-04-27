// SPDX-License-Identifier: MIT

#pragma once

namespace ocf::rhi {

class CommandBuffer {
public:
    virtual ~CommandBuffer() = default;

    virtual void begin() = 0;

    virtual void end() = 0;

    virtual void reset() = 0;
};

} // namespace ocf::rhi
