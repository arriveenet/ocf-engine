#include "ocf/renderer/Texture.h"

#include "ocf/core/Engine.h"
#include "ocf/rhi/Device.h"

namespace  ocf {

struct Texture::BuilderDetails {
    uint32_t width;
    uint32_t height;
};


Texture::Texture(Engine& engine, const Builder& builder)
{
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
