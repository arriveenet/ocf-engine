// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/math/vec2.h"
#include "ocf/math/vec4.h"
#include "ocf/renderer/TextureSampler.h"
#include "ocf/renderer/VertexTypes.h"
#include "ocf/rhi/Handle.h"
#include "ocf/scene/Component.h"

#include <array>
#include <cstdint>
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
 * @brief Base for components that draw textured quads in the node's local 2D space.
 *
 * Content is built in layout pixels (origin at the top left, y down) and placed in local
 * space scaled by 1 / pixelsPerUnit with y pointing up, so that the anchor point of the
 * content lies at the node origin. Geometry is rebuilt lazily in update() after a property
 * changes.
 *
 * Derived classes implement rebuild() with appendQuad() and addGeometry(), and get their
 * material instances from createMaterialInstance() so that setColor() reaches them.
 */
class OCF_API Drawable2D : public Component {
public:
    static constexpr float DEFAULT_PIXELS_PER_UNIT = 100.0f;

    ~Drawable2D() override;

    void update(float deltaTime) override;

    /** @brief Set the color multiplied with the texture. */
    void setColor(const math::vec4& color);
    const math::vec4& getColor() const noexcept { return m_color; }

    /**
     * @brief Set the point of the content placed at the node origin.
     *
     * (0, 0) is the top-left corner and (1, 1) the bottom-right corner of the content.
     */
    void setAnchor(const math::vec2& anchor);
    const math::vec2& getAnchor() const noexcept { return m_anchor; }

    /** @brief Set how many layout pixels fit in one world unit. */
    void setPixelsPerUnit(float pixelsPerUnit);
    float getPixelsPerUnit() const noexcept { return m_pixelsPerUnit; }

    /** @brief Size of the content in pixels. Valid after the next rebuild. */
    const math::vec2& getContentSize() const noexcept { return m_contentSize; }

protected:
    /**
     * @param fragmentShaderPath Asset path of a fragment shader that reads the color from
     *        set 1 binding 0 and the texture from set 1 binding 1.
     */
    Drawable2D(Engine& engine, std::string_view fragmentShaderPath);

    /** @brief Build the geometry. Called from update() after the old one was released. */
    virtual void rebuild() = 0;

    void markDirty() noexcept { m_dirty = true; }

    void setContentSize(const math::vec2& size) noexcept { m_contentSize = size; }

    /**
     * @brief Create a material instance with the current color.
     *
     * The instance is owned by the material and lives as long as this component, since a
     * Material cannot release a single instance.
     *
     * @param opacity Multiplied with the alpha of the color for this instance only.
     */
    MaterialInstance* createMaterialInstance(float opacity = 1.0f);

    /** @brief Change the opacity of an instance made by createMaterialInstance(). */
    void setMaterialOpacity(MaterialInstance* materialInstance, float opacity);

    /** @brief Bind a texture to a material instance with a clamping sampler. */
    static void bindTexture(MaterialInstance* materialInstance, Texture* texture,
                            TextureFilter filter);

    /** @brief Convert a point in layout pixels to local space. Uses the content size. */
    math::vec2 toLocal(const math::vec2& pixel) const;

    /** @brief Append a quad given in layout pixels, with uv0 at its top-left corner. */
    void appendQuad(std::vector<VertexPositionTexCoord>& vertices,
                    std::vector<uint32_t>& indices, const math::vec2& position,
                    const math::vec2& size, const math::vec2& uv0, const math::vec2& uv1) const;

    /**
     * @brief Append a quad given in layout pixels with a texture coordinate per corner.
     *
     * @param uvs Texture coordinates of the top-left, bottom-left, bottom-right and top-right
     *        corners, so that the texture can also be rotated.
     */
    void appendQuad(std::vector<VertexPositionTexCoord>& vertices,
                    std::vector<uint32_t>& indices, const math::vec2& position,
                    const math::vec2& size, const std::array<math::vec2, 4>& uvs) const;

    /** @brief Upload the geometry and add a renderable that draws it with the instance. */
    void addGeometry(const std::vector<VertexPositionTexCoord>& vertices,
                     const std::vector<uint32_t>& indices, MaterialInstance* materialInstance);

    Engine& m_engine;

private:
    struct Geometry {
        VertexBuffer* vertexBuffer = nullptr;
        IndexBuffer* indexBuffer = nullptr;
    };

    struct MaterialEntry {
        MaterialInstance* materialInstance = nullptr;
        float opacity = 1.0f;
    };

    void releaseGeometry();
    void createPipeline(VertexBuffer* vertexBuffer);
    void applyColor(const MaterialEntry& entry);

    math::vec4 m_color = math::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    math::vec2 m_anchor = math::vec2(0.5f, 0.5f);
    float m_pixelsPerUnit = DEFAULT_PIXELS_PER_UNIT;
    math::vec2 m_contentSize = math::vec2(0.0f, 0.0f);
    bool m_dirty = true;

    rhi::ShaderModuleHandle m_vertexShader;
    rhi::ShaderModuleHandle m_fragmentShader;
    rhi::PipelineHandle m_pipeline;
    Material* m_material = nullptr;

    std::vector<MaterialEntry> m_materialInstances;
    std::vector<Geometry> m_geometries;
};

} // namespace ocf
