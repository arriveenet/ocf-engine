#include "ocf/resource/TextureManager.h"

#include "ocf/core/Engine.h"
#include "ocf/core/Logger.h"
#include "ocf/platform/FileSystem.h"
#include "ocf/renderer/Texture.h"
#include "ocf/rhi/Device.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace ocf {

TextureManager::TextureManager(Engine& engine)
    : m_engine(engine)
{
}

TextureManager::~TextureManager()
{
    clear();
}

void TextureManager::clear()
{
    auto& device = m_engine.getDevice();
    for (auto& [key, texture] : m_textures) {
        device.destroyTexture(texture->getHandle());
        delete texture;
    }
    m_textures.clear();
}

Texture* TextureManager::addImage(std::string_view filePath)
{
    Texture* texture = nullptr;

    std::string fullPath = FileSystem::getInstance()->getAssetFullPath(filePath.data());
    if (fullPath.empty()) {
        return nullptr;
    }

    auto iter = m_textures.find(fullPath);
    if (iter != m_textures.end()) {
        texture = iter->second;
    }

    if (!texture) {
        int width = 0, height = 0, channels = 0;
        unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (data != nullptr) {
            Texture::Format format = Texture::Format::RGBA;
            Texture::InternalFormat internalFormat = Texture::InternalFormat::RGBA8;

            Texture::PixelBufferDescriptor buffer(
                data, size_t(width * height * 4), format, Texture::Type::Ubyte,
                (Texture::PixelBufferDescriptor::Callback)&stbi_image_free);

            texture = Texture::Builder()
                          .width(uint32_t(width))
                          .height(uint32_t(height))
                          .levels(0)
                          .sampler(Texture::Sampler::Sampler2D)
                          .format(internalFormat)
                          .build(m_engine);
            texture->setImage(m_engine, 0, std::move(buffer));

            m_textures.emplace(fullPath, texture);
        }
        else {

            OCF_LOG_ERROR("Could not load texture file: %s", fullPath.c_str());
        }
    }

    return texture;
}

Texture* TextureManager::getTextureForKey(std::string_view textureKeyName) const
{
    auto iter = m_textures.find(textureKeyName.data());

    if (iter != m_textures.end()) {
        auto key = FileSystem::getInstance()->getAssetFullPath(textureKeyName.data());
        iter = m_textures.find(key);
    }

    if (iter != m_textures.end()) {
        return iter->second;
    }

    return nullptr;
}

Texture* TextureManager::getWhiteTexture()
{
    const std::string key = "/white-texture";
    return getWhiteTexture(key, 0xff);
}

Texture* TextureManager::getWhiteTexture(std::string_view key, uint8_t luma)
{
    Texture* texture = getTextureForKey(key);
    if (texture != nullptr) {
        return texture;
    }

    uint8_t texls[] = {luma, luma, luma, 0xff, luma, luma, luma, 0xff,
                       luma, luma, luma, 0xff, luma, luma, luma, 0xff};

    Texture::PixelBufferDescriptor buffer(texls, sizeof(texls), Texture::Format::RGBA,
                                          Texture::Type::Ubyte, nullptr);
    texture = Texture::Builder()
                  .width(2)
                  .height(2)
                  .levels(0)
                  .sampler(Texture::Sampler::Sampler2D)
                  .format(Texture::InternalFormat::RGBA8)
                  .build(m_engine);
    texture->setImage(m_engine, 0, std::move(buffer));

    m_textures.emplace(key, texture);

    return texture;
}

} // namespace ocf
