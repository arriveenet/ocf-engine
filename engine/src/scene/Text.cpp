// SPDX-License-Identifier: MIT
#include "ocf/scene/Text.h"

#include "ocf/core/Engine.h"
#include "ocf/core/Logger.h"
#include "ocf/core/TextUtility.h"
#include "ocf/platform/FileSystem.h"
#include "ocf/renderer/IndexBuffer.h"
#include "ocf/renderer/Material.h"
#include "ocf/renderer/MaterialInstance.h"
#include "ocf/renderer/Renderer.h"
#include "ocf/renderer/Texture.h"
#include "ocf/renderer/TextureSampler.h"
#include "ocf/renderer/VertexBuffer.h"
#include "ocf/resource/Font.h"
#include "ocf/resource/FontAtlas.h"
#include "ocf/rhi/Device.h"
#include "ocf/rhi/PipelineState.h"

#include <cstdint>

namespace ocf {

namespace {

constexpr uint8_t VERTEX_STRIDE = sizeof(Vertex2);
static_assert(sizeof(Vertex2) == sizeof(float) * 5, "Vertex2 must be tightly packed");

} // namespace

Text::Text(Engine& engine, const Ref<Font>& font)
    : m_engine(engine)
    , m_font(font)
{
    auto& device = m_engine.getDevice();
    FileSystem* fileSystem = FileSystem::getInstance();
    m_vertexShader = device.createShaderModule(
        rhi::ShaderStage::Vertex, fileSystem->getAssetFullPath("shaders/text.vert.spv"));
    m_fragmentShader = device.createShaderModule(
        rhi::ShaderStage::Fragment, fileSystem->getAssetFullPath("shaders/text.frag.spv"));

    m_material = Material::Builder()
                     .uniformBlock(0, "TextParameters", 16)
                     .uniformMember("TextParameters", "color", rhi::UniformType::Float4, 0, 16)
                     .texture(1, "fontTexture")
                     .build(engine);
}

Text::~Text()
{
    releaseGeometry();

    auto& device = m_engine.getDevice();
    if (m_pipeline) {
        device.destroyPipeline(m_pipeline);
    }
    device.destroyShaderModule(m_vertexShader);
    device.destroyShaderModule(m_fragmentShader);

    m_material->terminate(m_engine);
    delete m_material;
}

void Text::update(float /*deltaTime*/)
{
    // The filter belongs to the shared Font, so it can change without going through Text
    if (m_font.ptr() != nullptr && m_font->getTextureFilter() != m_appliedFilter) {
        m_dirty = true;
    }

    if (m_dirty) {
        rebuild();
        m_dirty = false;
    }
}

void Text::setFont(const Ref<Font>& font)
{
    if (m_font != font) {
        m_font = font;
        m_dirty = true;
    }
}

void Text::setString(std::string_view utf8)
{
    if (m_string != utf8) {
        m_string = utf8;
        m_dirty = true;
    }
}

void Text::setColor(const math::vec4& color)
{
    m_color = color;

    for (Page& page : m_pages) {
        if (page.materialInstance != nullptr) {
            page.materialInstance->setParameter("color", m_color);
            page.materialInstance->commit(m_engine);
        }
    }
}

void Text::setAlignment(TextAlignment alignment)
{
    if (m_alignment != alignment) {
        m_alignment = alignment;
        m_dirty = true;
    }
}

void Text::setAnchor(const math::vec2& anchor)
{
    m_anchor = anchor;
    m_dirty = true;
}

void Text::setPixelsPerUnit(float pixelsPerUnit)
{
    if (pixelsPerUnit > 0.0f && m_pixelsPerUnit != pixelsPerUnit) {
        m_pixelsPerUnit = pixelsPerUnit;
        m_dirty = true;
    }
}

void Text::rebuild()
{
    releaseGeometry();
    m_contentSize = math::vec2(0.0f, 0.0f);

    if (m_font.ptr() == nullptr || m_string.empty()) {
        return;
    }

    FontAtlas* atlas = m_font->getFontAtlas();
    if (atlas == nullptr) {
        OCF_LOG_WARN("Text: font '{}' has no atlas", m_font->getFontName());
        return;
    }

    m_appliedFilter = m_font->getTextureFilter();

    const std::u32string utf32Text = TextUtility::utf8ToUtf32(m_string);
    const TextLayoutResult layout = TextLayout::layout(*m_font.ptr(), utf32Text, m_alignment);
    m_contentSize = layout.size;

    if (layout.quads.empty()) {
        return;
    }

    // Group the quads by atlas page, since each page is a separate texture and draw call
    const size_t pageCount = atlas->getPageCount();
    std::vector<std::vector<const GlyphQuad*>> quadsPerPage(pageCount);
    for (const GlyphQuad& quad : layout.quads) {
        if (quad.page < 0 || static_cast<size_t>(quad.page) >= pageCount) {
            OCF_LOG_WARN("Text: glyph refers to missing atlas page {}", quad.page);
            continue;
        }
        quadsPerPage[quad.page].push_back(&quad);
    }

    if (m_pages.size() < pageCount) {
        m_pages.resize(pageCount);
    }

    // Layout space is in pixels with y down; local space is in units with y up
    const float scale = 1.0f / m_pixelsPerUnit;
    const math::vec2 origin(m_anchor.x * layout.size.x, m_anchor.y * layout.size.y);
    const math::vec3 center((0.5f - m_anchor.x) * layout.size.x * scale,
                            -(0.5f - m_anchor.y) * layout.size.y * scale, 0.0f);

    std::vector<Vertex2> vertices;
    std::vector<uint32_t> indices;

    for (size_t pageIndex = 0; pageIndex < pageCount; pageIndex++) {
        const auto& quads = quadsPerPage[pageIndex];
        if (quads.empty()) {
            continue;
        }

        Texture* texture = atlas->getTexture(static_cast<unsigned int>(pageIndex));
        const float invTexWidth = 1.0f / static_cast<float>(texture->getWidth());
        const float invTexHeight = 1.0f / static_cast<float>(texture->getHeight());

        vertices.clear();
        indices.clear();
        vertices.reserve(quads.size() * 4);
        indices.reserve(quads.size() * 6);

        for (const GlyphQuad* quad : quads) {
            const float left = (quad->position.x - origin.x) * scale;
            const float right = (quad->position.x + quad->size.x - origin.x) * scale;
            const float top = -(quad->position.y - origin.y) * scale;
            const float bottom = -(quad->position.y + quad->size.y - origin.y) * scale;

            const float u0 = quad->texRect.getMinX() * invTexWidth;
            const float u1 = quad->texRect.getMaxX() * invTexWidth;
            const float v0 = quad->texRect.getMinY() * invTexHeight;
            const float v1 = quad->texRect.getMaxY() * invTexHeight;

            const uint32_t base = static_cast<uint32_t>(vertices.size());
            vertices.push_back({math::vec3(left, top, 0.0f), math::vec2(u0, v0)});
            vertices.push_back({math::vec3(left, bottom, 0.0f), math::vec2(u0, v1)});
            vertices.push_back({math::vec3(right, bottom, 0.0f), math::vec2(u1, v1)});
            vertices.push_back({math::vec3(right, top, 0.0f), math::vec2(u1, v0)});

            indices.insert(indices.end(),
                           {base, base + 1, base + 2, base, base + 2, base + 3});
        }

        VertexBuffer* vertexBuffer =
            VertexBuffer::Builder()
                .attribute(VertexAttribute::Position, VertexBuffer::AttributeType::Float3,
                           VERTEX_STRIDE, offsetof(Vertex2, position))
                .attribute(VertexAttribute::TexCoord0, VertexBuffer::AttributeType::Float2,
                           VERTEX_STRIDE, offsetof(Vertex2, texCoord))
                .bufferCount(1)
                .vertexCount(static_cast<uint32_t>(vertices.size()))
                .build(m_engine);
        vertexBuffer->setBufferData(m_engine, vertices.data(),
                                    vertices.size() * sizeof(Vertex2), 0);

        IndexBuffer* indexBuffer = IndexBuffer::Builder()
                                       .indexType(IndexBuffer::IndexType::Uint)
                                       .indexCount(static_cast<uint32_t>(indices.size()))
                                       .build(m_engine);
        indexBuffer->setBufferData(m_engine, indices.data(), indices.size() * sizeof(uint32_t),
                                   0);

        if (!m_pipeline) {
            createPipeline(vertexBuffer);
        }

        Page& page = m_pages[pageIndex];
        page.vertexBuffer = vertexBuffer;
        page.indexBuffer = indexBuffer;

        MaterialInstance* materialInstance = preparePageMaterial(page, texture, m_appliedFilter);

        m_renderables.push_back(new Renderable(vertexBuffer, indexBuffer, materialInstance,
                                               m_pipeline, AlphaMode::Blend, center));
    }
}

void Text::releaseGeometry()
{
    for (Renderable* renderable : m_renderables) {
        delete renderable;
    }
    m_renderables.clear();

    // The device defers destruction until in-flight frames are done with the buffers
    for (Page& page : m_pages) {
        if (page.vertexBuffer != nullptr) {
            page.vertexBuffer->terminate(m_engine);
            delete page.vertexBuffer;
            page.vertexBuffer = nullptr;
        }
        if (page.indexBuffer != nullptr) {
            page.indexBuffer->terminate(m_engine);
            delete page.indexBuffer;
            page.indexBuffer = nullptr;
        }
    }
}

MaterialInstance* Text::preparePageMaterial(Page& page, Texture* texture, TextureFilter filter)
{
    const bool isNewInstance = page.materialInstance == nullptr;
    if (isNewInstance) {
        page.materialInstance = m_material->createInstance();
        page.materialInstance->setParameter("color", m_color);
        page.materialInstance->commit(m_engine);
    }

    // Only touch the descriptor when the texture or filter changes (e.g. after setFont())
    if (isNewInstance || page.texture != texture || page.filter != filter) {
        const bool nearest = filter == TextureFilter::Nearest;
        const TextureSampler sampler(
            nearest ? TextureSampler::MinFilter::Nearest : TextureSampler::MinFilter::Linear,
            nearest ? TextureSampler::MagFilter::Nearest : TextureSampler::MagFilter::Linear,
            TextureSampler::WrapMode::ClampToEdge);
        page.materialInstance->setParameter("fontTexture", texture, sampler);
        page.texture = texture;
        page.filter = filter;
    }

    return page.materialInstance;
}

void Text::createPipeline(VertexBuffer* vertexBuffer)
{
    Material* uboMaterial = m_engine.getRenderer().getUBOMaterial();

    rhi::PipelineState pipeline;
    pipeline.vertexShader = m_vertexShader;
    pipeline.fragmentShader = m_fragmentShader;
    pipeline.vertexBufferInfo = vertexBuffer->getVertexBufferInfoHandle();
    pipeline.pipelineLayout.setLayout[0] = uboMaterial->getDescriptorSetLayout().getHandle();
    pipeline.pipelineLayout.setLayout[1] = m_material->getDescriptorSetLayout().getHandle();

    // Straight alpha blending; no depth writes so overlapping glyphs and text behind text
    // blend correctly. Both faces are drawn so the text stays visible from behind.
    rhi::RasterState& raster = pipeline.rasterState;
    raster.bits.culling = rhi::CullingMode::None;
    raster.bits.blendFunctionSrcColor = rhi::BlendFunction::SrcAlpha;
    raster.bits.blendFunctionDstColor = rhi::BlendFunction::OneMinusSrcAlpha;
    raster.bits.blendEquationColor = rhi::BlendEquation::Add;
    raster.bits.blendFunctionSrcAlpha = rhi::BlendFunction::One;
    raster.bits.blendFunctionDstAlpha = rhi::BlendFunction::OneMinusSrcAlpha;
    raster.bits.blendEquationAlpha = rhi::BlendEquation::Add;
    raster.bits.depthWriteEnable = false;

    m_pipeline = m_engine.getDevice().createPipeline(pipeline);
}

} // namespace ocf
