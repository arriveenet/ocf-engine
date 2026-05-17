#include "ocf/renderer/Texture.h"

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

void Texture::setImage(uint32_t width, uint32_t height)
{
}

} // namespace ocf
