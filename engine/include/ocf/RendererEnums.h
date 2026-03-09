#pragma once
#include <array>
#include <stdint.h>
#include <string>
#include <unordered_map>

namespace ocf {

static constexpr size_t VERTEX_ATTRIBUTE_COUNT_MAX = 16;
static constexpr size_t SAMPLER_COUNT_MAX = 62;

enum class PrimitiveType : uint8_t {
    POINTS = 0,
    LINES = 1,
    LINE_STRIP = 3,
    TRIANGLES = 4,
    TRIANGLE_STRIP = 5
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
    STATIC,
    DYNAMIC,
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

enum class SamplerType : uint8_t {
    SAMPLER_2D,
    SAMPLER_2D_ARRAY,
    SAMPLER_CUBEMAP,
    SAMPLER_CUBEMAP_ARRAY,
    SAMPLER_3D,
};

enum class PixelDataFormat : uint8_t {
    R,
    R_INTEGER,
    RG,
    RG_INTEGER,
    RGB,
    RGB_INTEGER,
    RGBA,
    RGBA_INTEGER,
    DEPTH_COMPONENT,
    DEPTH_STENCIL
};

enum class PixelDataType : uint8_t {
    UNSIGNED_BYTE,
    BYTE,
    UNSIGNED_SHORT,
    SHORT,
    UNSIGNED_INT,
    INT,
    FLOAT,
    HALF_FLOAT,
    UNSIGNED_INT_24_8
};

enum class TextureFormat : uint16_t {
    R8,
    RG8,
    RGB8,
    RGBA8,
};

/**
 * @brief Sampler wrap mode
 */
enum class SamplerWrapMode : uint8_t { CLAMP_TO_EDGE, REPEAT, MIRRORED_REPEAT };

/**
 * @brief Sampler minification filter
 */
enum class SamplerMinFilter : uint8_t {
    NEAREST = 0,
    LINEAR = 1,
    NEAREST_MIPMAP_NEAREST = 2,
    LINEAR_MIPMAP_NEAREST = 3,
    NEAREST_MIPMAP_LINEAR = 4,
    LINEAR_MIPMAP_LINEAR = 5
};

/**
 * @brief Sampler magnification filter
 */
enum class SamplerMagFilter : uint8_t {
    NEAREST = 0, //!< No filtering. Nearest neighor is used.
    LINEAR = 1   //!< Box filtering. Weighted average of the 4 nearest texels is used.
};

/**
 * @brief Sampler parameters
 */
struct SamplerParameters {
    SamplerMagFilter filterMag : 1;
    SamplerMinFilter filterMin : 3;
    SamplerWrapMode wrapS : 2;
    SamplerWrapMode wrapT : 2;
    SamplerWrapMode wrapR : 2;

    uint8_t padding : 6;
};

static_assert(sizeof(SamplerParameters) == 2, "SamplerParameters size must be 2 bytes");

enum class ShaderStage : uint8_t { VERTEX, FRAGMENT };

struct UniformInfo {
    int32_t count = 0;
    int32_t location = -1;
    uint32_t type = 0;
    uint32_t size = 0;
    uint32_t offset = 0;
};

using UniformInfoMap = std::unordered_map<std::string, UniformInfo>;

/**
 * @brief Face culling mode
 */
enum class CullingMode : uint8_t { NONE, FRONT, BACK, FRONT_AND_BACK };

/**
 * @brief Blend function
 */
enum class BlendFunction : uint8_t {
    ZERO,
    ONE,
    SRC_COLOR,
    ONE_MINUS_SRC_COLOR,
    DST_COLOR,
    ONE_MINUS_DST_COLOR,
    SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA,
    DST_ALPHA,
    ONE_MINUS_DST_ALPHA,
};

enum class SamplerCompareFunc : uint8_t {
    NEVER,
    LESS,
    LEQUAL,
    EQUAL,
    GREATER,
    NOTEQUAL,
    GEQUAL,
    ALWAYS
};

/**
 * @brief Raster state descriptor
 */
struct RasterState {
    using CullingMode = CullingMode;
    using DepthFunc = SamplerCompareFunc;
    using BlendFunction = BlendFunction;

    CullingMode culling = CullingMode::NONE;
    BlendFunction blendSrc = BlendFunction::ONE;
    BlendFunction blendDst = BlendFunction::ZERO;
    DepthFunc depthFunc = DepthFunc::LESS;

    bool hasBlending() const noexcept
    {
        return !(blendSrc == BlendFunction::ONE && blendDst == BlendFunction::ZERO);
    }
};

enum class VertexAttribute : uint8_t {
    POSITION = 0,
    NORMAL = 1,
    COLOR = 2,
    TEXCOORD0 = 3,
    TEXCOORD1 = 4,
    BONE_INDICES = 5,
    BONE_WEIGHTS = 6,
    CUSTOM0 = 7,
    CUSTOM1 = 8,
    CUSTOM2 = 9,
    CUSTOM3 = 10,
    CUSTOM4 = 11,
    CUSTOM5 = 12,
    CUSTOM6 = 13,
    CUSTOM7 = 14,
    CUSTOM8 = 15
};

} // namespace ocf::backend
