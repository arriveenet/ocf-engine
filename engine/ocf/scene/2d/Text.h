// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/Reference.h"
#include "ocf/resource/Font.h"
#include "ocf/resource/TextLayout.h"
#include "ocf/scene/2d/Drawable2D.h"

#include <string>
#include <string_view>
#include <vector>

namespace ocf {

class Engine;
class MaterialInstance;
class Texture;

/**
 * @brief Component that draws a string with a Font as textured quads.
 *
 * The text is laid out in pixels and placed in the node's local space as described in
 * Drawable2D. The content size is the size of the laid out text block.
 */
class OCF_API Text : public Drawable2D {
public:
    Text(Engine& engine, const Ref<Font>& font);
    ~Text() override;

    void update(float deltaTime) override;

    void setFont(const Ref<Font>& font);
    const Ref<Font>& getFont() const noexcept { return m_font; }

    /** @brief Set the string to draw, encoded as UTF-8. */
    void setString(std::string_view utf8);
    const std::string& getString() const noexcept { return m_string; }

    void setAlignment(TextAlignment alignment);
    TextAlignment getAlignment() const noexcept { return m_alignment; }

protected:
    void rebuild() override;

private:
    // Material binding for one atlas page, reused across rebuilds
    struct Page {
        MaterialInstance* materialInstance = nullptr;
        Texture* texture = nullptr;
        TextureFilter filter = TextureFilter::Linear;
    };

    MaterialInstance* preparePageMaterial(Page& page, Texture* texture, TextureFilter filter);

    Ref<Font> m_font;
    std::string m_string;
    TextAlignment m_alignment = TextAlignment::Left;
    TextureFilter m_appliedFilter = TextureFilter::Linear; // Font filter used by the last rebuild

    std::vector<Page> m_pages; // Indexed by atlas page
};

} // namespace ocf
