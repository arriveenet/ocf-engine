// SPDX-License-Identifier: MIT
#include "ocf/scene/2d/Drawable2D.h"

#include "ocf/core/Engine.h"
#include "ocf/platform/FileSystem.h"
#include "ocf/renderer/IndexBuffer.h"
#include "ocf/renderer/Material.h"
#include "ocf/renderer/MaterialInstance.h"
#include "ocf/renderer/Renderer.h"
#include "ocf/renderer/VertexBuffer.h"
#include "ocf/rhi/Device.h"
#include "ocf/rhi/PipelineState.h"

#include <cstddef>

namespace ocf {

namespace {

constexpr uint8_t VERTEX_STRIDE = sizeof(VertexPositionTexCoord);

} // namespace

Drawable2D::Drawable2D(Engine& engine, std::string_view fragmentShaderPath)
    : m_engine(engine)
{
    auto& device = m_engine.getDevice();
    FileSystem* fileSystem = FileSystem::getInstance();
    m_vertexShader = device.createShaderModule(
        rhi::ShaderStage::Vertex, fileSystem->getAssetFullPath("shaders/drawable2d.vert.spv"));
    m_fragmentShader = device.createShaderModule(
        rhi::ShaderStage::Fragment, fileSystem->getAssetFullPath(fragmentShaderPath));

    m_material = Material::Builder()
                     .uniformBlock(0, "Drawable2DParameters", 16)
                     .uniformMember("Drawable2DParameters", "color", rhi::UniformType::Float4, 0,
                                    16)
                     .texture(1, "mainTexture")
                     .build(engine);
}

Drawable2D::~Drawable2D()
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

void Drawable2D::update(float /*deltaTime*/)
{
    if (m_dirty) {
        releaseGeometry();
        m_contentSize = math::vec2(0.0f, 0.0f);
        rebuild();
        m_dirty = false;
    }
}

void Drawable2D::setColor(const math::vec4& color)
{
    m_color = color;

    for (MaterialInstance* materialInstance : m_materialInstances) {
        materialInstance->setParameter("color", m_color);
        materialInstance->commit(m_engine);
    }
}

void Drawable2D::setAnchor(const math::vec2& anchor)
{
    if (m_anchor != anchor) {
        m_anchor = anchor;
        m_dirty = true;
    }
}

void Drawable2D::setPixelsPerUnit(float pixelsPerUnit)
{
    if (pixelsPerUnit > 0.0f && m_pixelsPerUnit != pixelsPerUnit) {
        m_pixelsPerUnit = pixelsPerUnit;
        m_dirty = true;
    }
}

MaterialInstance* Drawable2D::createMaterialInstance()
{
    MaterialInstance* materialInstance = m_material->createInstance();
    materialInstance->setParameter("color", m_color);
    materialInstance->commit(m_engine);
    m_materialInstances.push_back(materialInstance);
    return materialInstance;
}

void Drawable2D::bindTexture(MaterialInstance* materialInstance, Texture* texture,
                             TextureFilter filter)
{
    const bool nearest = filter == TextureFilter::Nearest;
    const TextureSampler sampler(
        nearest ? TextureSampler::MinFilter::Nearest : TextureSampler::MinFilter::Linear,
        nearest ? TextureSampler::MagFilter::Nearest : TextureSampler::MagFilter::Linear,
        TextureSampler::WrapMode::ClampToEdge);
    materialInstance->setParameter("mainTexture", texture, sampler);
}

math::vec2 Drawable2D::toLocal(const math::vec2& pixel) const
{
    // Layout space is in pixels with y down; local space is in units with y up
    const float scale = 1.0f / m_pixelsPerUnit;
    const math::vec2 origin(m_anchor.x * m_contentSize.x, m_anchor.y * m_contentSize.y);
    return math::vec2((pixel.x - origin.x) * scale, -(pixel.y - origin.y) * scale);
}

void Drawable2D::appendQuad(std::vector<VertexPositionTexCoord>& vertices,
                            std::vector<uint32_t>& indices, const math::vec2& position,
                            const math::vec2& size, const math::vec2& uv0,
                            const math::vec2& uv1) const
{
    const math::vec2 topLeft = toLocal(position);
    const math::vec2 bottomRight = toLocal(position + size);

    const uint32_t base = static_cast<uint32_t>(vertices.size());
    vertices.push_back({math::vec3(topLeft.x, topLeft.y, 0.0f), math::vec2(uv0.x, uv0.y)});
    vertices.push_back({math::vec3(topLeft.x, bottomRight.y, 0.0f), math::vec2(uv0.x, uv1.y)});
    vertices.push_back(
        {math::vec3(bottomRight.x, bottomRight.y, 0.0f), math::vec2(uv1.x, uv1.y)});
    vertices.push_back({math::vec3(bottomRight.x, topLeft.y, 0.0f), math::vec2(uv1.x, uv0.y)});

    indices.insert(indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
}

void Drawable2D::addGeometry(const std::vector<VertexPositionTexCoord>& vertices,
                             const std::vector<uint32_t>& indices,
                             MaterialInstance* materialInstance)
{
    if (vertices.empty() || indices.empty()) {
        return;
    }

    VertexBuffer* vertexBuffer =
        VertexBuffer::Builder()
            .attribute(VertexAttribute::Position, VertexBuffer::AttributeType::Float3,
                       VERTEX_STRIDE, offsetof(VertexPositionTexCoord, position))
            .attribute(VertexAttribute::TexCoord0, VertexBuffer::AttributeType::Float2,
                       VERTEX_STRIDE, offsetof(VertexPositionTexCoord, texCoord))
            .bufferCount(1)
            .vertexCount(static_cast<uint32_t>(vertices.size()))
            .build(m_engine);
    vertexBuffer->setBufferData(m_engine, vertices.data(),
                                vertices.size() * sizeof(VertexPositionTexCoord), 0);

    IndexBuffer* indexBuffer = IndexBuffer::Builder()
                                   .indexType(IndexBuffer::IndexType::Uint)
                                   .indexCount(static_cast<uint32_t>(indices.size()))
                                   .build(m_engine);
    indexBuffer->setBufferData(m_engine, indices.data(), indices.size() * sizeof(uint32_t), 0);

    if (!m_pipeline) {
        createPipeline(vertexBuffer);
    }

    m_geometries.push_back({vertexBuffer, indexBuffer});

    const math::vec2 center = toLocal(m_contentSize * 0.5f);
    m_renderables.push_back(new Renderable(vertexBuffer, indexBuffer, materialInstance,
                                           m_pipeline, AlphaMode::Blend,
                                           math::vec3(center.x, center.y, 0.0f)));
}

void Drawable2D::releaseGeometry()
{
    for (Renderable* renderable : m_renderables) {
        delete renderable;
    }
    m_renderables.clear();

    // The device defers destruction until in-flight frames are done with the buffers
    for (Geometry& geometry : m_geometries) {
        geometry.vertexBuffer->terminate(m_engine);
        delete geometry.vertexBuffer;
        geometry.indexBuffer->terminate(m_engine);
        delete geometry.indexBuffer;
    }
    m_geometries.clear();
}

void Drawable2D::createPipeline(VertexBuffer* vertexBuffer)
{
    Material* uboMaterial = m_engine.getRenderer().getUBOMaterial();

    rhi::PipelineState pipeline;
    pipeline.vertexShader = m_vertexShader;
    pipeline.fragmentShader = m_fragmentShader;
    pipeline.vertexBufferInfo = vertexBuffer->getVertexBufferInfoHandle();
    pipeline.pipelineLayout.setLayout[0] = uboMaterial->getDescriptorSetLayout().getHandle();
    pipeline.pipelineLayout.setLayout[1] = m_material->getDescriptorSetLayout().getHandle();
    pipeline.pipelineLayout.pushConstant = Renderer::OBJECT_PUSH_CONSTANT_RANGE;

    // Straight alpha blending; no depth writes so overlapping quads blend correctly. Both
    // faces are drawn so the content stays visible from behind.
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
