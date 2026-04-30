// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/math/vec4.h"

#include <array>
#include <cstdint>
#include <variant>

namespace ocf::rhi {

static constexpr size_t VERTEX_ATTRIBUTE_COUNT_MAX = 16;
static constexpr size_t SAMPLER_COUNT_MAX = 62;

enum class PrimitiveType : uint8_t {
	Points,
	Lines,
    LineStrip,
    Triangles,
    TriangleStrip,
};


enum class ElementType : uint8_t {
    BYTE,
    BYTE2,
    BYTE3,
    BYTE4,
    UNSIGNED_BYTE,
    UNSIGNED_BYTE2,
    UNSIGNED_BYTE3,
    UNSIGNED_BYTE4,
    SHORT,
    SHORT2,
    SHORT3,
    SHORT4,
    UNSIGNED_SHORT,
    UNSIGNED_SHORT2,
    UNSIGNED_SHORT3,
    UNSIGNED_SHORT4,
    INT,
    UNSIGNED_INT,
    FLOAT,
    FLOAT2,
    FLOAT3,
    FLOAT4,
    DOUBLE,
    DOUBLE2,
    DOUBLE3,
    DOUBLE4
};

enum class BufferUsage : uint8_t {
    Static,
    Dynamic,
};

struct Attribute {
    static constexpr uint8_t BUFFER_UNUSED = 0xFF;

    uint32_t offset = 0;
    uint8_t stride = 0;
    uint8_t buffer = BUFFER_UNUSED;
    ElementType type = ElementType::FLOAT;
    uint8_t flags = 0;
};

using AttributeArray = std::array<Attribute, VERTEX_ATTRIBUTE_COUNT_MAX>;

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
