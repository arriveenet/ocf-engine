#pragma once

#include "ocf/resource/Resource.h"

#include "ocf/math/vec2.h"
#include "ocf/math/Rect.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ocf {

class Engine;
class Texture;
class MaxRectsBinPack;

class FontAtlas : public Resource {
public:
    static constexpr int DEFAULT_TEXTURE_WIDTH = 512;
    static constexpr int DEFAULT_TEXTURE_HEIGHT = 512;

    FontAtlas(Engine& engine);
    FontAtlas(Engine& engine, int width, int height);
    virtual ~FontAtlas();

    void addNewPage();

    int getCurrentPage() const { return m_currentPage; }

    bool insert(math::Rect& outRect, const uint8_t* bitmap, int width, int height);

    void updateTexture();

    size_t getPageCount() const { return m_atlasTextures.size(); }

    void releaseTextures();

    void setTexure(unsigned int slot, Texture* texture);
    Texture* getTexture(unsigned int  slot) const;

private:
    Engine& m_engine;
    int m_width = DEFAULT_TEXTURE_WIDTH;
    int m_height = DEFAULT_TEXTURE_HEIGHT;
    int m_currentPage = -1;
    bool m_dirty = false;
    std::vector<uint8_t> m_currentPageData;
    std::unordered_map<unsigned int, Texture*> m_atlasTextures;
    std::unordered_set<Texture*> m_ownedTextures;
    std::unique_ptr<MaxRectsBinPack> m_binPack;
};

} // namespace ocf
