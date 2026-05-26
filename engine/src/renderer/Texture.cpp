#include "ocf/renderer/Texture.h"

#include "ocf/core/Engine.h"
#include "ocf/rhi/Device.h"

namespace  ocf {

struct Texture::BuilderDetails {
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    InternalFormat format = InternalFormat::RGBA8;
    Sampler sampler = Sampler::Sampler2D;
};


Texture::Builder::Builder()
{
    m_impl = new BuilderDetails();
}

Texture::Builder::~Builder()
{
    delete m_impl;
}

Texture::Builder& Texture::Builder::width(uint32_t width)
{
    m_impl->width = width;
    return *this;
}

Texture::Builder& Texture::Builder::height(uint32_t height)
{
    m_impl->height = height;
    return *this;
}

Texture::Builder& Texture::Builder::depth(uint32_t depth)
{
    m_impl->depth = depth;
    return *this;
}

Texture::Builder& Texture::Builder::format(InternalFormat format)
{
    m_impl->format = format;
    return *this;
}

Texture::Builder& Texture::Builder::sampler(Sampler sampler)
{
    m_impl->sampler = sampler;
    return *this;
}

Texture* Texture::Builder::build(Engine& engine)
{
    Texture* texture = new Texture(engine, *this);
    return texture;
}


Texture::Texture(Engine& engine, const Builder& builder)
    : m_width(builder->width)
    , m_height(builder->height)
    , m_depth(builder->depth)
    , m_format(builder->format)
    , m_sampler(builder->sampler)
{
    Engine::Device& device = engine.getDevice();
    m_handle = device.createTexture(m_sampler, m_levelCount, m_format, m_width, m_height, m_depth);
}

void Texture::terminate(Engine& engine)
{
}

size_t Texture::getWidth(size_t level) const
{
    return valueForLevel(static_cast<uint8_t>(level), m_width);
}

size_t Texture::getHeight(size_t level) const
{
    return valueForLevel(static_cast<uint8_t>(level), m_height);
}

size_t Texture::getDepth(size_t level) const
{
    return valueForLevel(static_cast<uint8_t>(level), m_depth);
}

void Texture::setImage(Engine& engine, size_t level, uint32_t xoffset, uint32_t yoffset,
                       uint32_t zoffset,
                       uint32_t width, uint32_t height, uint32_t depth,
                       PixelBufferDescriptor&& buffer) const
{
    engine.getDevice().updateTextureImage(m_handle, static_cast<uint8_t>(level), xoffset, yoffset,
                                          zoffset, width, height, depth, std::move(buffer));
}


} // namespace ocf
