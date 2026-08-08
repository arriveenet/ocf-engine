#pragma once
#include <string>
#include <unordered_map>
#include <stdint.h>

namespace ocf {

class Engine;
class Texture;

class TextureManager {
public:
    TextureManager(Engine& engine);
    ~TextureManager();

    void clear();

    Texture* addImage(std::string_view filePath);

    Texture* getTextureForKey(std::string_view textureKeyName) const;

    Texture* getWhiteTexture();

    Texture* getWhiteTexture(std::string_view key, uint8_t luma);

private:
    Engine& m_engine;
    std::unordered_map<std::string, Texture*> m_textures;
};

} // namespace ocf
