// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>

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

struct PipelineState {
};

} // namespace ocf::rhi
