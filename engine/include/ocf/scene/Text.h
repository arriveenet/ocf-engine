// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/Reference.h"
#include "ocf/math/vec2.h"
#include "ocf/math/vec4.h"
#include "ocf/resource/Font.h"
#include "ocf/resource/TextLayout.h"
#include "ocf/rhi/Handle.h"
#include "ocf/scene/Component.h"

#include <string>
#include <string_view>
#include <vector>

namespace ocf {

class Engine;
class Material;
class MaterialInstance;
class Texture;
class VertexBuffer;
class IndexBuffer;

/**
 * @brief Component that draws a string with a Font as textured quads.
 *
 * The text is laid out in pixels and placed in the node's local space, scaled by
 * 1 / pixelsPerUnit with y pointing up. Geometry is rebuilt lazily in update() after a
 * property changes.
 */
class Text : public Component {
public:
    static constexpr float DEFAULT_PIXELS_PER_UNIT = 100.0f;

    Text(Engine& engine, const Ref<Font>& font);
    ~Text() override;

    void update(float deltaTime) override;

    void setFont(const Ref<Font>& font);
    const Ref<Font>& getFont() const noexcept { return m_font; }

    /** @brief Set the string to draw, encoded as UTF-8. */
    void setString(std::string_view utf8);
    const std::string& getString() const noexcept { return m_string; }

    void setColor(const math::vec4& color);
    const math::vec4& getColor() const noexcept { return m_color; }

    void setAlignment(TextAlignment alignment);
    TextAlignment getAlignment() const noexcept { return m_alignment; }

    /**
     * @brief Set the point of the text block placed at the node origin.
     *
     * (0, 0) is the top-left corner and (1, 1) the bottom-right corner of the block.
     */
    void setAnchor(const math::vec2& anchor);
    const math::vec2& getAnchor() const noexcept { return m_anchor; }

    /** @brief Set how many layout pixels fit in one world unit. */
    void setPixelsPerUnit(float pixelsPerUnit);
    float getPixelsPerUnit() const noexcept { return m_pixelsPerUnit; }

    /** @brief Size of the laid out text block in pixels. Valid after the next rebuild. */
    const math::vec2& getContentSize() const noexcept { return m_contentSize; }

private:
    // Draw data for one atlas page. Material instances are owned by m_material and reused
    // across rebuilds, since a Material cannot release a single instance.
    struct Page {
        MaterialInstance* materialInstance = nullptr;
        Texture* texture = nullptr;
        TextureFilter filter = TextureFilter::Linear;
        VertexBuffer* vertexBuffer = nullptr;
        IndexBuffer* indexBuffer = nullptr;
    };

    void rebuild();
    void releaseGeometry();
    MaterialInstance* preparePageMaterial(Page& page, Texture* texture, TextureFilter filter);
    void createPipeline(VertexBuffer* vertexBuffer);

    Engine& m_engine;
    Ref<Font> m_font;
    std::string m_string;
    math::vec4 m_color = math::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    TextAlignment m_alignment = TextAlignment::Left;
    math::vec2 m_anchor = math::vec2(0.5f, 0.5f);
    float m_pixelsPerUnit = DEFAULT_PIXELS_PER_UNIT;
    math::vec2 m_contentSize = math::vec2(0.0f, 0.0f);
    bool m_dirty = true;
    TextureFilter m_appliedFilter = TextureFilter::Linear; // Font filter used by the last rebuild

    rhi::ShaderModuleHandle m_vertexShader;
    rhi::ShaderModuleHandle m_fragmentShader;
    rhi::PipelineHandle m_pipeline;
    Material* m_material = nullptr;

    std::vector<Page> m_pages; // Indexed by atlas page
};

} // namespace ocf
