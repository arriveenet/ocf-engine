// SPDX-License-Identifier: MIT

#include "ocf/renderer/Renderer.h"

#include "ocf/core/Engine.h"
#include "ocf/math/constants.h"
#include "ocf/math/matrix_transform.h"
#include "ocf/platform/FileSystem.h"
#include "ocf/renderer/IndexBuffer.h"
#include "ocf/renderer/Material.h"
#include "ocf/renderer/MaterialInstance.h"
#include "ocf/renderer/Texture.h"
#include "ocf/renderer/TextureSampler.h"
#include "ocf/renderer/VertexBuffer.h"
#include "ocf/rhi/CommandBuffer.h"
#include "ocf/rhi/Device.h"

#include <stb_image.h>

#include <cmath>
#include <cstddef>
#include <vector>

namespace ocf {

using namespace rhi;
using MinFilter = TextureSampler::MinFilter;
using MagFilter = TextureSampler::MagFilter;

static std::vector<Vertex> vertices{};

static std::vector<uint32_t> indices{};

static void createCubeGeometry()
{
    const math::vec3 A(-0.5f, 0.5f, 0.5f), B(-0.5f, -0.5f, 0.5f),
    C(0.5f, 0.5f, 0.5f), D(0.5f, -0.5f, 0.5f),
    E(-0.5f, 0.5f, -0.5f), F(-0.5f, -0.5f, -0.5f),
    G(0.5f, 0.5f, -0.5f), H(0.5f, -0.5f, -0.5f);

    vertices =
    {
        // front
        { A, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 1.0f } },
        { B, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
        { C, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f } },
        { D, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 0.0f } },
        // back
        { E, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, 1.0f } },
        { F, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, 0.0f } },
        { G, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 1.0f } },
        { H, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f } },
        // right
        { C, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
        { D, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f } },
        { G, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 1.0f } },
        { H, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
        // left
        { E, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        { F, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
        { A, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 1.0f } },
        { B, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
        // top
        { E, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        { A, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 1.0f } },
        { G, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 1.0f } },
        { C, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
        // bottom
        { B, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
        { F, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
        { D, { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f } },
        { H, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
    };

    indices = {
        0, 1, 2, 2, 1, 3,       // front
        6, 7, 4, 4, 7, 5,       // back
        8, 9, 10, 10, 9, 11,    // right
        12, 13, 14, 14, 13, 15, // left
        16, 17, 18, 18, 17, 19, // top
        20, 21, 22, 22, 21, 23, // bottom
    };
}

static void createSphareGeometry()
{
    const int stackCount = 32;
    const int sliceCount = 48;
    constexpr auto PI = math::pi<float>();
    const auto sliceStep = PI * 2.0f / sliceCount;
    const auto stackStep = PI / stackCount;

    for (int stack = 0; stack <= stackCount; ++stack) {
        auto stackAngle = (float)PI / 2 - stack * stackStep;

        for (int slice = 0; slice <= sliceCount; ++slice) {
            auto sliceAngle = slice * sliceStep;

            auto x = std::cosf(stackAngle) * std::cosf(sliceAngle);
            auto y = std::sinf(stackAngle);
            auto z = std::cosf(stackAngle) * std::sinf(sliceAngle);

            Vertex v;
            v.position = math::vec3(x, y, z);
            v.normal = normalize(v.position);
            v.color = math::vec3(0.7f, 0.85f, 0.9f);
            vertices.push_back(v);
        }
    }

    for (int stack = 0; stack < stackCount; ++stack) {
        uint32_t k1 = stack * (sliceCount + 1);
        uint32_t k2 = k1 + sliceCount + 1;

        for (int slice = 0; slice < sliceCount; ++slice, ++k1, ++k2) {
            if (stack != 0) {
                indices.insert(indices.end(), { k1, k1 + 1, k2 });
            }
            if (stack != (stackCount - 1)) {
                indices.insert(indices.end(), { k1 + 1, k2 + 1, k2 });
            }
        }
    }
}

Renderer::Renderer(Engine& engine, rhi::Device* device)
    : m_engine(engine)
    , m_device(device)
{
}

Renderer::~Renderer()
{
    m_device->destroyPipeline(m_pipelineHandle);
    m_vertexBuffer->terminate(m_engine);
    m_indexBuffer->terminate(m_engine);
    m_material->terminate(m_engine);

    delete m_vertexBuffer;
    delete m_indexBuffer;
    delete m_material;
    delete m_texture;
}

bool Renderer::init()
{
    createCubeGeometry();
    //createSphareGeometry();

    const size_t vertexBufferSize = sizeof(Vertex) * vertices.size();
    m_vertexBuffer = VertexBuffer::Builder()
                         .attribute(VertexAttribute::Position, VertexBuffer::AttributeType::Float3,
                                    sizeof(Vertex), offsetof(Vertex, position))
                         .attribute(VertexAttribute::Normal, VertexBuffer::AttributeType::Float3,
                                    sizeof(Vertex), offsetof(Vertex, normal))
                         .attribute(VertexAttribute::Color, VertexBuffer::AttributeType::Float3,
                                    sizeof(Vertex), offsetof(Vertex, color))
                         .bufferCount(1)
                         .vertexCount(uint32_t(vertices.size()))
                         .build(m_engine);
    m_vertexBuffer->setBufferData(m_engine, vertices.data(), vertexBufferSize, 0);

    const size_t indexBufferSize = sizeof(uint32_t) * indices.size();
    m_indexBuffer = IndexBuffer::Builder()
                        .indexType(IndexBuffer::IndexType::Uint)
                        .indexCount(uint32_t(indices.size()))
                        .build(m_engine);
    m_indexBuffer->setBufferData(m_engine, indices.data(), indexBufferSize, 0);

    auto texPath = FileSystem::getInstance()->getAssetFullPath("textures/test-texture.png");
    int w, h, n;
    unsigned char* data = stbi_load(texPath.c_str(), &w, &h, &n, 4);
    assert(data != nullptr);

    Texture::PixelBufferDescriptor buffer(
    data, size_t(w * h * 4), Texture::Format::RGBA, Texture::Type::Ubyte,
    (Texture::PixelBufferDescriptor::Callback)&stbi_image_free);

    m_texture = Texture::Builder()
                    .width(uint32_t(w))
                    .height(uint32_t(h))
                    .sampler(Texture::Sampler::Sampler2D)
                    .format(Texture::InternalFormat::RGBA8)
                    .build(m_engine);
    m_texture->setImage(m_engine, 0, std::move(buffer));
    TextureSampler sampler(MinFilter::Linear, MagFilter::Linear);

    auto vsPath = FileSystem::getInstance()->getAssetFullPath("shaders/cube.vert.spv");
    auto fsPath = FileSystem::getInstance()->getAssetFullPath("shaders/cube.frag.spv");

    ShaderModuleHandle vs = m_device->createShaderModule(ShaderStage::Vertex, vsPath);
    ShaderModuleHandle fs = m_device->createShaderModule(ShaderStage::Fragment, fsPath);

    m_material = Material::Builder()
                    .uniformBlock(0, "SceneContents", 224)
                    .uniformMember("SceneContents", "matWorld",    UniformType::Mat4, 0, 64)
                    .uniformMember("SceneContents", "matView",     UniformType::Mat4, 64, 64)
                    .uniformMember("SceneContents", "matProj",     UniformType::Mat4, 128, 64)
                    .uniformMember("SceneContents", "lightDir",    UniformType::Float4, 192, 16)
                    .uniformMember("SceneContents", "eyePosition", UniformType::Float4, 208, 16)
                    .build(m_engine);

    m_materialInstance = m_material->createInstance();

    PipelineState pipeline;
    pipeline.vertexShader = vs;
    pipeline.fragmentShader = fs;
    pipeline.vertexBufferInfo = m_vertexBuffer->getVertexBufferInfoHandle();
    pipeline.layout = m_material->getDescriptorSetLayout().getHandle();

    m_pipelineHandle = m_device->createPipeline(pipeline);

    return true;
}

void Renderer::beginFrame()
{
    m_device->beginFrame();
}

void Renderer::endFrame()
{
    m_device->endFrame();
}

void Renderer::render()
{
    const uint32_t frameIndex = m_device->getCurrentFrameIndex();
    m_materialInstance->setFrameIndex(frameIndex);

    math::vec3 eyePos = math::vec3(2, 1, 4);
    static float angle = 0.0f;
    angle += 3.0f * 0.016666f;

    math::mat4 matWorld = math::rotateY(math::radians(angle));
    math::mat4 matView = math::lookAt(eyePos, math::vec3(0, 0, 0), math::vec3(0, 1, 0));
    math::mat4 matProj = math::perspective(math::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    math::vec4 lightDir = math::normalize(math::vec4(0.5f, 2.0f, 1.0f, 0.0f));
    math::vec4 eyePosition = math::vec4(eyePos, 0);

    m_materialInstance->setParameter("matWorld", matWorld);
    m_materialInstance->setParameter("matView", matView);
    m_materialInstance->setParameter("matProj", matProj);
    m_materialInstance->setParameter("lightDir", lightDir);
    m_materialInstance->setParameter("eyePosition", eyePosition);

    m_materialInstance->commit(m_engine);

    auto commandBuffer = m_device->getCommandBuffer();
    commandBuffer->begin();
    // Change to color layout
    commandBuffer->transitionLayout(rhi::ResourceState::Undefined,
                                    rhi::ResourceState::ColorAttachment);

    rhi::RenderingInfo info;
    info.clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    commandBuffer->beginRendering(info);

    commandBuffer->bindPipeline(m_pipelineHandle);
    commandBuffer->bindDescriptorSets(m_pipelineHandle,
                                      m_materialInstance->getDescriptorSetHandle());
    commandBuffer->bindVertexBuffers(0, 1, m_vertexBuffer->getHandle());
    commandBuffer->bindIndexBuffer(m_indexBuffer->getHandle(), 0);
    commandBuffer->drawIndexed(uint32_t(indices.size()), 1, 0, 0, 0);

    commandBuffer->endRendering();

    // Change to present layout
    commandBuffer->transitionLayout(rhi::ResourceState::ColorAttachment,
                                    rhi::ResourceState::Present);
    commandBuffer->end();
}

} // namespace ocf

