#include "ocf/Texture.h"
#include "ocf/OpenGLUtility.h"
#include <glad/gl.h>
#include <iostream>

namespace ocf {

struct Texture::BuilderDetails {
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    uint8_t levels = 1;
    SamplerType samplerType = SamplerType::SAMPLER_2D;
    TextureFormat format = TextureFormat::RGBA8;
};

Texture::Builder::Builder()
{
    m_impl = new BuilderDetails();
}

Texture::Builder::~Builder()
{
    delete m_impl;
}

Texture::Builder& Texture::Builder::width(uint32_t width) noexcept
{
    m_impl->width = width;
    return *this;
}

Texture::Builder& Texture::Builder::height(uint32_t height) noexcept
{
    m_impl->height = height;
    return *this;
}

Texture::Builder& Texture::Builder::depth(uint32_t depth) noexcept
{
    m_impl->depth = depth;
    return *this;
}

Texture::Builder& Texture::Builder::levels(uint8_t levels) noexcept
{
    m_impl->levels = levels;
    return *this;
}

Texture::Builder& Texture::Builder::sampler(SamplerType sampler) noexcept
{
    m_impl->samplerType = sampler;
    return *this;
}

Texture::Builder& Texture::Builder::format(TextureFormat format) noexcept
{
    m_impl->format = format;
    return *this;
}

Texture* Texture::Builder::build()
{
    Texture* texture = new Texture(*this);
    return texture;
}

Texture::Texture(const Builder& builder)
    : m_width(builder->width)
    , m_height(builder->height)
    , m_depth(builder->depth)
    , m_levels(builder->levels)
    , m_samplerType(builder->samplerType)
    , m_format(builder->format)
{
    const GLenum target = OpenGLUtility::getTextureTarget(m_samplerType);
    const GLenum internalFormat = OpenGLUtility::getInternalFormat(m_format);
    const auto [baseFormat, baseType] = OpenGLUtility::textureFormatToFormatAndType(m_format);

    glGenTextures(1, &m_handle);
    glBindTexture(target, m_handle);
    for (uint8_t level = 0; level < m_levels; ++level) {
        const uint32_t levelWidth = (m_width >> level) > 0 ? (m_width >> level) : 1;
        const uint32_t levelHeight = (m_height >> level) > 0 ? (m_height >> level) : 1;
        glTexImage2D(target, level, internalFormat, levelWidth, levelHeight, 0, baseFormat,
                     baseType, nullptr);
    }
    glBindTexture(target, 0);

    CHECK_GL_ERROR(std::cerr);
}

Texture::~Texture()
{
    glDeleteTextures(1, &m_handle);
    m_handle = 0;
}

void Texture::setImage(uint32_t level, PixelDataFormat format, PixelDataType type,
                       const void* data) noexcept
{
    const GLenum target = OpenGLUtility::getTextureTarget(m_samplerType);
    const GLenum glFormat = OpenGLUtility::getFormat(format);
    const GLenum glType = OpenGLUtility::getType(type);

    const uint32_t levelWidth = m_width >> level;
    const uint32_t levelHeight = m_height >> level;

    glBindTexture(target, m_handle);
    glTexSubImage2D(target, level, 0, 0, levelWidth > 0 ? levelWidth : 1,
                    levelHeight > 0 ? levelHeight : 1, glFormat, glType, data);
    glBindTexture(target, 0);

    CHECK_GL_ERROR(std::cerr);
}

void Texture::generateMipmaps() noexcept
{
    const GLenum target = OpenGLUtility::getTextureTarget(m_samplerType);
    glBindTexture(target, m_handle);
    glGenerateMipmap(target);
    glBindTexture(target, 0);

    CHECK_GL_ERROR(std::cerr);
}

} // namespace ocf
