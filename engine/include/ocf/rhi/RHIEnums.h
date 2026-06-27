// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/math/vec4.h"

#include <array>
#include <cstdint>
#include <type_traits>
#include <vector>

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
    Byte,
    Byte2,
    Byte3,
    Byte4,
    Ubyte,
    Ubyte2,
    Ubyte3,
    Ubyte4,
    Short,
    Short2,
    Short3,
    Short4,
    Ushort,
    Ushort2,
    Ushort3,
    Ushort4,
    Int,
    Uint,
    Float,
    Float2,
    Float3,
    Float4,
    Double,
    Double2,
    Double3,
    Double4
};

enum class BufferType : uint8_t {
    Vertex,
    Index,
    Uniform,
};

enum class BufferUsage : uint8_t {
    Static,
    Dynamic,
};

enum class SamplerType : uint8_t {
    Sampler2D,
    Sampler2DArray,
    SamplerCubemap,
    SamplerCubemapArray,
    Sampler3D,
};

enum class TextureFormat : uint16_t {
    R8,
    RG8,
    RGB8,
    RGBA8,
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
    Ubyte,
    Byte,
    Ushort,
    Short,
    Uint,
    Int,
    Float,
};

/**
 * @brief Sampler wrap mode
 */
enum class SamplerWrapMode : uint8_t {
    ClampToEdge,
    Repeat,
    MirroredRepeat,
};

/**
 * @brief Sampler minification filter
 */
enum class SamplerMinFilter : uint8_t {
    Nearest = 0,
    Linear = 1,
    NearestMipmapNearest = 2,
    LinearMipmapNearest = 3,
    NearestMipmapLinear = 4,
    LinearMipmapLinear = 5,
};

/**
 * @brief Sampler magnification filter
 */
enum class SamplerMagFilter : uint8_t {
    Nearest = 0,  //!< No filtering. Nearest neighor is used.
    Linear  = 1   //!< Box filtering. Weighted average of the 4 nearest texels is used.
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
    uint8_t anisotropyLog2 : 3;
    uint8_t padding : 3;
};

static_assert(sizeof(SamplerParameters) == 2, "SamplerParameters size must be 2 bytes");

struct Attribute {
    static constexpr uint8_t BUFFER_UNUSED = 0xFF;

    uint32_t offset = 0;
    uint8_t stride = 0;
    uint8_t buffer = BUFFER_UNUSED;
    ElementType type = ElementType::Float;
    uint8_t flags = 0;
};

using AttributeArray = std::array<Attribute, VERTEX_ATTRIBUTE_COUNT_MAX>;

enum class ShaderStage : uint32_t {
    Vertex      = 1 << 0,
    Fragment    = 1 << 1,
    Compute     = 1 << 2,
    AllStage    = 0x7FFFFFFF,
};
using ShaderStageFlags = ShaderStage;

constexpr ShaderStageFlags operator|(ShaderStageFlags lhs, ShaderStageFlags rhs)
{
    return static_cast<ShaderStageFlags>(
        static_cast<std::underlying_type_t<ShaderStageFlags>>(lhs) |
        static_cast<std::underlying_type_t<ShaderStageFlags>>(rhs));
}

constexpr bool operator&(ShaderStageFlags lhs, ShaderStage rhs)
{
    return static_cast<std::underlying_type_t<ShaderStageFlags>>(lhs) &
           static_cast<std::underlying_type_t<ShaderStageFlags>>(rhs);
}

enum class UniformType : uint8_t {
    Bool,
    Int,
    Int2,
    Int3,
    Int4,
    Float,
    Float2,
    Float3,
    Float4,
    Mat3,
    Mat4,
};

enum class DescriptorType : uint8_t {
    Sampler,
    CombinedImageSampler,
    StorageImage,
    UniformBuffer,
    StorageBuffer,
};

enum class ResourceState : uint8_t {
    Undefined,
    ColorAttachment,
    Present,
};

struct RenderingInfo {
    math::vec4 clearColor;
};

/**
 * @brief Face culling mode
 */
enum class CullingMode : uint8_t {
    None,
    Front,
    Back,
    FrontAndBack
};

/**
 * @brief Blend function
 */
enum class BlendFunction : uint8_t {
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
};

enum class SamplerCompareFunc : uint8_t {
    Never,
    Less,
    LessEqual,
    Equal,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,
};

/**
 * @brief Raster state descriptor
 */
struct RasterState {
    using CullingMode = rhi::CullingMode;
    using DepthFunc = rhi::SamplerCompareFunc;
    using BlendFunction = rhi::BlendFunction;

    CullingMode culling = CullingMode::None;
    BlendFunction blendSrc = BlendFunction::One;
    BlendFunction blendDst = BlendFunction::Zero;
    DepthFunc depthFunc = DepthFunc::Less;

    bool hasBlending() const noexcept
    {
        return !(blendSrc == BlendFunction::One && blendDst == BlendFunction::Zero);
    }
};

struct DescriptorLayoutBinding {
    uint32_t binding;
    DescriptorType type;
    ShaderStageFlags shaderStageFlags;
};

struct DescriptorSetLayout {
    std::vector<DescriptorLayoutBinding> descriptors;
};

} // namespace ocf::rhi
