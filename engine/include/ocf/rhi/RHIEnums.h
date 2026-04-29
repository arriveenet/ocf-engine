// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/math/vec4.h"

#include <cstdint>
#include <variant>

namespace ocf::rhi {

enum class PrimitiveType : uint8_t {
	Points,
	Lines,
    LineStrip,
    Triangles,
    TriangleStrip,
};

enum class ShaderStage : uint8_t {
    Vertex,
    Fragment,
    Compute,
};

enum class ResourceState : uint8_t {
    Undefined,
    ColorAttachment,
    Present,
};

struct PipelineState {
};



struct ClearValue {
    std::variant<math::vec4> color;
};

struct RenderingInfo {
};

} // namespace ocf::rhi
