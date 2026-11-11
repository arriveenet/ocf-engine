// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/Reference.h"
#include "ocf/core/compiler.h"
#include "ocf/math/Rect.h"
#include "ocf/math/vec2.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ocf {

class Texture;

/**
 * @brief One image packed into a sprite sheet.
 *
 * Packers such as TexturePacker can trim the transparent border of an image and store it
 * rotated by 90 degrees clockwise. The frame keeps what is needed to draw it as if it were the
 * original image: the content size of a sprite showing it is sourceSize, and the packed part
 * is drawn at offset within it.
 */
struct OCF_API SpriteFrame {
    std::string name;

    /**
     * @brief Region in the texture, in texels with the origin at the top left.
     *
     * The size is the size of the trimmed image as it is drawn. When the frame is rotated, it
     * covers size.y by size.x texels in the texture.
     */
    math::Rect textureRect;

    /** @brief Whether the image is stored rotated by 90 degrees clockwise. */
    bool rotated = false;

    /** @brief Position of the trimmed image in the original image, in pixels. */
    math::vec2 offset = math::vec2(0.0f, 0.0f);

    /** @brief Size of the original image before trimming, in pixels. */
    math::vec2 sourceSize = math::vec2(0.0f, 0.0f);

    /** @brief Pivot set in the packer, with (0, 0) at the top left and (1, 1) at the bottom right. */
    math::vec2 pivot = math::vec2(0.5f, 0.5f);

    /** @brief A frame showing a region of a texture, without trimming or rotation. */
    static SpriteFrame fromRect(const math::Rect& rect);

    /** @brief Region the frame covers in the texture, with rotation applied. */
    math::Rect getPackedRect() const;
};

/** @brief Geometry of a frame in the layout pixels of a sprite. */
struct OCF_API SpriteQuad {
    math::vec2 position = math::vec2(0.0f, 0.0f);
    math::vec2 size = math::vec2(0.0f, 0.0f);
    /** Texture coordinates of the top-left, bottom-left, bottom-right and top-right corners. */
    std::array<math::vec2, 4> uvs{};
};

/**
 * @brief Compute where a frame is drawn in a sprite of size frame.sourceSize.
 *
 * Flipping mirrors the whole original image, so the trimmed part moves along with it.
 *
 * @param textureSize Size of the texture in texels. Must be positive.
 */
OCF_API SpriteQuad computeSpriteQuad(const SpriteFrame& frame, const math::vec2& textureSize,
                                     bool flipX, bool flipY);

/**
 * @brief Named frames packed into one texture, as made by TexturePacker and similar tools.
 *
 * The texture is not owned by the sprite sheet.
 */
class OCF_API SpriteSheet : public RefCounted {
public:
    SpriteSheet();
    ~SpriteSheet();

    void setName(std::string name) { m_name = std::move(name); }
    const std::string& getName() const noexcept { return m_name; }

    /**
     * @brief Set the image the frames are packed into.
     *
     * @param path Asset path of the image, resolved to a texture by the loader.
     */
    void setImage(std::string path, uint32_t width, uint32_t height);
    const std::string& getImagePath() const noexcept { return m_imagePath; }
    uint32_t getImageWidth() const noexcept { return m_imageWidth; }
    uint32_t getImageHeight() const noexcept { return m_imageHeight; }

    void setTexture(Texture* texture) noexcept { m_texture = texture; }
    Texture* getTexture() const noexcept { return m_texture; }

    /** @brief Add a frame. A frame with the same name is replaced. */
    void addFrame(SpriteFrame frame);

    /** @brief The frame with the name, or nullptr. */
    const SpriteFrame* getFrame(std::string_view name) const;

    /** @brief Frames in the order they were added. */
    const std::vector<SpriteFrame>& getFrames() const noexcept { return m_frames; }
    size_t getFrameCount() const noexcept { return m_frames.size(); }

    /** @brief Frames whose name starts with the prefix, sorted by name. Meant for animations. */
    std::vector<const SpriteFrame*> getFramesWithPrefix(std::string_view prefix) const;

private:
    std::string m_name;
    std::string m_imagePath;
    uint32_t m_imageWidth = 0;
    uint32_t m_imageHeight = 0;
    Texture* m_texture = nullptr;

    std::vector<SpriteFrame> m_frames;
    std::unordered_map<std::string, size_t> m_frameIndices;
};

} // namespace ocf
