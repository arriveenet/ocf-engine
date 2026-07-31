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
#include "ocf/scene/View.h"
#include "ocf/scene/Camera.h"

#include <stb_image.h>

#include <cmath>
#include <cstddef>
#include <vector>

namespace ocf {

using namespace rhi;
using MinFilter = TextureSampler::MinFilter;
using MagFilter = TextureSampler::MagFilter;

static std::vector<Vertex2> vertices{};

static std::vector<uint32_t> indices{};

//static void createCubeGeometry()
//{
//    const math::vec3 A(-0.5f, 0.5f, 0.5f), B(-0.5f, -0.5f, 0.5f),
//    C(0.5f, 0.5f, 0.5f), D(0.5f, -0.5f, 0.5f),
//    E(-0.5f, 0.5f, -0.5f), F(-0.5f, -0.5f, -0.5f),
//    G(0.5f, 0.5f, -0.5f), H(0.5f, -0.5f, -0.5f);
//
//    vertices =
//    {
//        // front
//        { A, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 1.0f } },
//        { B, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
//        { C, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f } },
//        { D, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 0.0f } },
//        // back
//        { E, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, 1.0f } },
//        { F, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, 0.0f } },
//        { G, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 1.0f } },
//        { H, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f } },
//        // right
//        { C, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
//        { D, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f } },
//        { G, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 1.0f } },
//        { H, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
//        // left
//        { E, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
//        { F, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
//        { A, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 1.0f } },
//        { B, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
//        // top
//        { E, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
//        { A, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 1.0f } },
//        { G, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 1.0f } },
//        { C, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
//        // bottom
//        { B, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
//        { F, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
//        { D, { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f } },
//        { H, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
//    };
//
//    indices = {
//        0, 1, 2, 2, 1, 3,       // front
//        6, 7, 4, 4, 7, 5,       // back
//        8, 9, 10, 10, 9, 11,    // right
//        12, 13, 14, 14, 13, 15, // left
//        16, 17, 18, 18, 17, 19, // top
//        20, 21, 22, 22, 21, 23, // bottom
//    };
//}
//
//static void createSphareGeometry()
//{
//    const int stackCount = 32;
//    const int sliceCount = 48;
//    constexpr auto PI = math::pi<float>();
//    const auto sliceStep = PI * 2.0f / sliceCount;
//    const auto stackStep = PI / stackCount;
//
//    for (int stack = 0; stack <= stackCount; ++stack) {
//        auto stackAngle = (float)PI / 2 - stack * stackStep;
//
//        for (int slice = 0; slice <= sliceCount; ++slice) {
//            auto sliceAngle = slice * sliceStep;
//
//            auto x = std::cosf(stackAngle) * std::cosf(sliceAngle);
//            auto y = std::sinf(stackAngle);
//            auto z = std::cosf(stackAngle) * std::sinf(sliceAngle);
//
//            Vertex v;
//            v.position = math::vec3(x, y, z);
//            v.normal = normalize(v.position);
//            v.color = math::vec3(0.7f, 0.85f, 0.9f);
//            vertices.push_back(v);
//        }
//    }
//
//    for (int stack = 0; stack < stackCount; ++stack) {
//        uint32_t k1 = stack * (sliceCount + 1);
//        uint32_t k2 = k1 + sliceCount + 1;
//
//        for (int slice = 0; slice < sliceCount; ++slice, ++k1, ++k2) {
//            if (stack != 0) {
//                indices.insert(indices.end(), { k1, k1 + 1, k2 });
//            }
//            if (stack != (stackCount - 1)) {
//                indices.insert(indices.end(), { k1 + 1, k2 + 1, k2 });
//            }
//        }
//    }
//}

static void createPlaneGeometry()
{
    vertices = {
        {{-0.5f, -0.5f, 0.5f}, {0.0f, 24.0f}},
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {24.0f, 24.0f}},
        {{0.5f, 0.5f, 0.5f}, {24.0f, 0.0f}},
    };
    indices = {
        0, 3, 1, 0, 2, 3,
    };
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
    m_materialInstance->terminate(m_engine);
    m_texture->terminate(m_engine);

    delete m_vertexBuffer;
    delete m_indexBuffer;
    delete m_material;
    delete m_texture;
}

bool Renderer::init()
{
    m_depthTexture = m_device->createDepthBuffer(m_engine.getWindowSize().x, m_engine.getWindowSize().y);
    //createCubeGeometry();
    //createSphareGeometry();
    createPlaneGeometry();

    const size_t vertexBufferSize = sizeof(Vertex2) * vertices.size();
    m_vertexBuffer = VertexBuffer::Builder()
                         .attribute(VertexAttribute::Position, VertexBuffer::AttributeType::Float3,
                                    sizeof(Vertex2), offsetof(Vertex2, position))
                         .attribute(VertexAttribute::TexCoord0, VertexBuffer::AttributeType::Float2,
                                    sizeof(Vertex2), offsetof(Vertex2, texCoord))
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
        [](void* buffer, size_t, void*) { stbi_image_free(buffer); });

    m_texture = Texture::Builder()
                    .width(uint32_t(w))
                    .height(uint32_t(h))
                    .levels(0xff)
                    .sampler(Texture::Sampler::Sampler2D)
                    .format(Texture::InternalFormat::RGBA8)
                    .build(m_engine);
    m_texture->setImage(m_engine, 0, std::move(buffer));
    m_texture->generateMipmaps(m_engine);
    TextureSampler sampler(MinFilter::Linear, MagFilter::Linear, SamplerWrapMode::Repeat);
    sampler.setAnisotropy(16.0f);

    auto vsPath = FileSystem::getInstance()->getAssetFullPath("shaders/texture.vert.spv");
    auto fsPath = FileSystem::getInstance()->getAssetFullPath("shaders/texture.frag.spv");

    ShaderModuleHandle vs = m_device->createShaderModule(ShaderStage::Vertex, vsPath);
    ShaderModuleHandle fs = m_device->createShaderModule(ShaderStage::Fragment, fsPath);

    m_material = Material::Builder()
                    .uniformBlock(0, "SceneContents", 224)
                    .uniformMember("SceneContents", "matWorld",    UniformType::Mat4, 0, 64)
                    .uniformMember("SceneContents", "matView",     UniformType::Mat4, 64, 64)
                    .uniformMember("SceneContents", "matProj",     UniformType::Mat4, 128, 64)
                    .texture(1, "gTex")
                    .build(m_engine);

    m_materialInstance = m_material->createInstance();

    m_materialInstance->setParameter("gTex", m_texture, sampler);

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

void Renderer::render(const View* view)
{
    if (view == nullptr || view->getScene() == nullptr || !view->hasCamera()) {
        return;
    }

    Scene* scene = view->getScene();

    m_renderQueue.clear();

    // Collect renderable objects from the scene
    RenderCommand renderCommand;
    renderCommand.vertexBuffer = m_vertexBuffer;
    renderCommand.indexBuffer = m_indexBuffer;
    renderCommand.pipelineHandle = m_pipelineHandle;
    renderCommand.materialInstance = m_materialInstance.get();
    renderCommand.indexCount = uint32_t(indices.size());
    m_renderQueue.addCommand(renderCommand);

    const uint32_t frameIndex = m_device->getCurrentFrameIndex();

    auto commandBuffer = m_device->getCommandBuffer();
    commandBuffer->begin();
    // Change to color layout
    commandBuffer->transitionLayout(rhi::ResourceState::Undefined,
                                    rhi::ResourceState::ColorAttachment);
    commandBuffer->transitionLayout(m_depthTexture, rhi::ResourceState::Undefined,
                                    rhi::ResourceState::DepthStencilAttachment);

    rhi::RenderingInfo info;
    info.clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    commandBuffer->beginRendering(info);

    for (auto& cmd : m_renderQueue.getRenderCommands()) {
        cmd.materialInstance->setFrameIndex(frameIndex);

        cmd.materialInstance->setParameter("matWorld", cmd.matWorld);
        cmd.materialInstance->setParameter("matView", view->getCamera()->getView());
        cmd.materialInstance->setParameter("matProj", view->getCamera()->getProjection());

        cmd.materialInstance->commit(m_engine);

        // Bind pipeline, descriptor sets, vertex/index buffers, and draw
        commandBuffer->bindPipeline(cmd.pipelineHandle);
        commandBuffer->bindDescriptorSets(cmd.pipelineHandle, cmd.materialInstance->getDescriptorSetHandle());

        // Draw Indexed
        if (cmd.indexBuffer != nullptr) {
            commandBuffer->bindVertexBuffers(0, 1, cmd.vertexBuffer->getHandle());
            commandBuffer->bindIndexBuffer(cmd.indexBuffer->getHandle(), 0);
            commandBuffer->drawIndexed(cmd.indexCount, 1, cmd.indexOffset, 0, 0);
        }
        // Draw Arrays
        else {
            commandBuffer->bindVertexBuffers(0, 1, cmd.vertexBuffer->getHandle());
            commandBuffer->draw(cmd.vertexCount, 1, cmd.vertexOffset, 0);
        }
    }

    commandBuffer->endRendering();
    // Change to present layout
    commandBuffer->transitionLayout(rhi::ResourceState::ColorAttachment,
                                    rhi::ResourceState::Present);
    commandBuffer->end();
}

} // namespace ocf

