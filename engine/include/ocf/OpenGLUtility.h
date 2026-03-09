#pragma once
#include "glad/gl.h"
#include "ocf/RendererEnums.h"
#include <ostream>
#include <string_view>
#include <utility>

namespace ocf {

namespace OpenGLUtility {

const char* getGLError(GLenum error) noexcept;
GLenum checkGLError(std::ostream& out, const char* function, size_t line) noexcept;
void assertGLError(std::ostream& out, const char* function, size_t line) noexcept;

#ifdef NDEBUG
#define CHECK_GL_ERROR(out)
#else
#define CHECK_GL_ERROR(out)                                                                        \
    {                                                                                              \
        OpenGLUtility::assertGLError(out, __func__, __LINE__);                                     \
    }
#define CHECK_GL_ERROR_NON_FATAL(out)                                                              \
    {                                                                                              \
        OpenGLUtility::checkGLError(out, __func__, __LINE__);                                      \
    }
#endif

GLenum toGLPrimitive(PrimitiveType primitiveType);
//GLenum toGLFormat(PixelFormat format);
GLsizei getGLDataTypeSize(GLenum type);

constexpr GLuint getComponentCount(const ElementType type)
{
    switch (type) {
    case ElementType::BYTE:
    case ElementType::UNSIGNED_BYTE:
    case ElementType::SHORT:
    case ElementType::UNSIGNED_SHORT:
    case ElementType::INT:
    case ElementType::UNSIGNED_INT:
    case ElementType::FLOAT:
    case ElementType::DOUBLE:
        return 1;
    case ElementType::BYTE2:
    case ElementType::UNSIGNED_BYTE2:
    case ElementType::SHORT2:
    case ElementType::UNSIGNED_SHORT2:
    case ElementType::FLOAT2:
    case ElementType::DOUBLE2:
        return 2;
    case ElementType::BYTE3:
    case ElementType::UNSIGNED_BYTE3:
    case ElementType::SHORT3:
    case ElementType::UNSIGNED_SHORT3:
    case ElementType::FLOAT3:
    case ElementType::DOUBLE3:
        return 3;
    case ElementType::BYTE4:
    case ElementType::UNSIGNED_BYTE4:
    case ElementType::SHORT4:
    case ElementType::UNSIGNED_SHORT4:
    case ElementType::FLOAT4:
    case ElementType::DOUBLE4:
        return 4;
    }

    return 1;
}

constexpr GLuint getComponentType(const ElementType type)
{
    switch (type) {
    case ElementType::BYTE:
    case ElementType::BYTE2:
    case ElementType::BYTE3:
    case ElementType::BYTE4:
        return GL_BYTE;
    case ElementType::UNSIGNED_BYTE:
    case ElementType::UNSIGNED_BYTE2:
    case ElementType::UNSIGNED_BYTE3:
    case ElementType::UNSIGNED_BYTE4:
        return GL_UNSIGNED_BYTE;
    case ElementType::SHORT:
    case ElementType::SHORT2:
    case ElementType::SHORT3:
    case ElementType::SHORT4:
        return GL_SHORT;
    case ElementType::UNSIGNED_SHORT:
    case ElementType::UNSIGNED_SHORT2:
    case ElementType::UNSIGNED_SHORT3:
    case ElementType::UNSIGNED_SHORT4:
        return GL_UNSIGNED_SHORT;
    case ElementType::INT:
        return GL_INT;
    case ElementType::UNSIGNED_INT:
        return GL_UNSIGNED_INT;
    case ElementType::FLOAT:
    case ElementType::FLOAT2:
    case ElementType::FLOAT3:
    case ElementType::FLOAT4:
        return GL_FLOAT;
    case ElementType::DOUBLE:
    case ElementType::DOUBLE2:
    case ElementType::DOUBLE3:
    case ElementType::DOUBLE4:
        return GL_DOUBLE;
    }
    return GL_INT;
}

constexpr GLenum getInternalFormat(const TextureFormat format)
{
    switch (format) {
    case ocf::TextureFormat::R8:
        return GL_R8;
    case ocf::TextureFormat::RG8:
        return GL_RG8;
    case ocf::TextureFormat::RGB8:
        return GL_RGB8;
    case ocf::TextureFormat::RGBA8:
        return GL_RGBA8;
    default:
        return GL_NONE;
    }
}

constexpr GLenum getFormat(PixelDataFormat format) noexcept
{
    switch (format) {
    case PixelDataFormat::RGB:
        return GL_RGB;
    case PixelDataFormat::RGBA:
        return GL_RGBA;
    case PixelDataFormat::R:
        return GL_RED;
    case PixelDataFormat::R_INTEGER:
        return GL_RED_INTEGER;
    case PixelDataFormat::RG:
        return GL_RG;
    case PixelDataFormat::RGB_INTEGER:
        return GL_RGB_INTEGER;
    case PixelDataFormat::RGBA_INTEGER:
        return GL_RGBA_INTEGER;

    default:
        return GL_NONE;
    }
}

constexpr GLenum getType(PixelDataType type) noexcept
{
    switch (type) {
    case PixelDataType::UNSIGNED_BYTE:
        return GL_UNSIGNED_BYTE;
    case PixelDataType::BYTE:
        return GL_BYTE;
    case PixelDataType::UNSIGNED_SHORT:
        return GL_UNSIGNED_SHORT;
    case PixelDataType::SHORT:
        return GL_SHORT;
    case PixelDataType::UNSIGNED_INT:
        return GL_UNSIGNED_INT;
    case PixelDataType::INT:
        return GL_INT;
    case PixelDataType::FLOAT:
        return GL_FLOAT;
    case PixelDataType::HALF_FLOAT:
        return GL_HALF_FLOAT;
    case PixelDataType::UNSIGNED_INT_24_8:
        return GL_UNSIGNED_INT_24_8;

    default:
        return GL_NONE;
    }
}

constexpr GLenum getTextureTarget(const SamplerType target)
{
    switch (target) {
    case SamplerType::SAMPLER_2D:
        return GL_TEXTURE_2D;
    case SamplerType::SAMPLER_2D_ARRAY:
        return GL_TEXTURE_2D_ARRAY;
    case SamplerType::SAMPLER_CUBEMAP:
        return GL_TEXTURE_CUBE_MAP;
    default:
        return GL_NONE;
    }
}

constexpr GLenum getWrapMode(const SamplerWrapMode mode)
{
    switch (mode) {
    case SamplerWrapMode::REPEAT:
        return GL_REPEAT;
    case SamplerWrapMode::CLAMP_TO_EDGE:
        return GL_CLAMP_TO_EDGE;
    case SamplerWrapMode::MIRRORED_REPEAT:
        return GL_MIRRORED_REPEAT;
    default:
        return GL_NONE;
    }
}

constexpr GLenum getTextureFilter(SamplerMinFilter filter)
{
    switch (filter) {
    case SamplerMinFilter::NEAREST:
        return GL_NEAREST;
    case SamplerMinFilter::LINEAR:
        return GL_LINEAR;
    case SamplerMinFilter::NEAREST_MIPMAP_NEAREST:
        return GL_NEAREST_MIPMAP_NEAREST;
    case SamplerMinFilter::LINEAR_MIPMAP_NEAREST:
        return GL_LINEAR_MIPMAP_NEAREST;
    case SamplerMinFilter::NEAREST_MIPMAP_LINEAR:
        return GL_NEAREST_MIPMAP_LINEAR;
    case SamplerMinFilter::LINEAR_MIPMAP_LINEAR:
        return GL_LINEAR_MIPMAP_LINEAR;
    default:
        return GL_NONE;
    }
}

constexpr GLenum getTextureFilter(SamplerMagFilter filter)
{
    return GL_NEAREST + GLenum(filter);
}

constexpr std::pair<GLenum, GLenum> textureFormatToFormatAndType(const TextureFormat format)
{
    switch (format) {
    case ocf::TextureFormat::R8:
        return {GL_RED, GL_UNSIGNED_BYTE};
    case ocf::TextureFormat::RG8:
        return {GL_RG, GL_UNSIGNED_BYTE};
    case ocf::TextureFormat::RGB8:
        return {GL_RGB, GL_UNSIGNED_BYTE};
    case ocf::TextureFormat::RGBA8:
        return {GL_RGBA, GL_UNSIGNED_BYTE};
    default:
        return {GL_NONE, GL_NONE};
    }
}

constexpr GLenum getBufferUsage(const BufferUsage usage)
{
    switch (usage) {
    case BufferUsage::STATIC:
        return GL_STATIC_DRAW;
    case BufferUsage::DYNAMIC:
        return GL_DYNAMIC_DRAW;
    default:
        return GL_NONE;
    }
}

constexpr GLenum getCullingMode(const CullingMode mode)
{
    switch (mode) {
    case CullingMode::FRONT:
        return GL_FRONT;
    case CullingMode::BACK:
        return GL_BACK;
    case CullingMode::FRONT_AND_BACK:
        return GL_FRONT_AND_BACK;
    default:
        return GL_NONE;
    }
}

constexpr GLenum getBlendFunctionMode(const BlendFunction func)
{
    switch (func) {
    case BlendFunction::ZERO:
        return GL_ZERO;
    case BlendFunction::ONE:
        return GL_ONE;
    case BlendFunction::SRC_COLOR:
        return GL_SRC_COLOR;
    case BlendFunction::ONE_MINUS_SRC_COLOR:
        return GL_ONE_MINUS_SRC_COLOR;
    case BlendFunction::DST_COLOR:
        return GL_DST_COLOR;
    case BlendFunction::ONE_MINUS_DST_COLOR:
        return GL_ONE_MINUS_DST_COLOR;
    case BlendFunction::SRC_ALPHA:
        return GL_SRC_ALPHA;
    case BlendFunction::ONE_MINUS_SRC_ALPHA:
        return GL_ONE_MINUS_SRC_ALPHA;
    case BlendFunction::DST_ALPHA:
        return GL_DST_ALPHA;
    case BlendFunction::ONE_MINUS_DST_ALPHA:
        return GL_ONE_MINUS_DST_ALPHA;
    default:
        return GL_NONE;
    }
}

constexpr GLenum getCompareFunc(SamplerCompareFunc func) noexcept
{
    switch (func) {
    case SamplerCompareFunc::NEVER:
        return GL_NEVER;
    case SamplerCompareFunc::LESS:
        return GL_LESS;
    case SamplerCompareFunc::LEQUAL:
        return GL_LEQUAL;
    case SamplerCompareFunc::EQUAL:
        return GL_EQUAL;
    case SamplerCompareFunc::GREATER:
        return GL_GREATER;
    case SamplerCompareFunc::NOTEQUAL:
        return GL_NOTEQUAL;
    case SamplerCompareFunc::GEQUAL:
        return GL_GEQUAL;
    case SamplerCompareFunc::ALWAYS:
        return GL_ALWAYS;
    default:
        return GL_NONE;
    }
}

constexpr GLenum getDepthFunc(SamplerCompareFunc func) noexcept
{
    return getCompareFunc(func);
}

constexpr GLenum getShaderStage(const ShaderStage type)
{
    switch (type) {
    case ShaderStage::VERTEX:
        return GL_VERTEX_SHADER;
    case ShaderStage::FRAGMENT:
        return GL_FRAGMENT_SHADER;
    default:
        return GL_NONE;
    }
}

GLuint loadShader(ShaderStage stage, std::string_view source);

GLuint compileProgram(GLuint vertexShader, GLuint fragmentShader);

} // namespace OpenGLUtility
} // namespace ocf
