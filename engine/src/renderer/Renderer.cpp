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
#include "ocf/scene/Scene.h"
#include "ocf/scene/Node.h"

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
    m_material->terminate(m_engine);
    m_materialInstance->terminate(m_engine);

    delete m_material;
}

bool Renderer::init()
{
    m_depthTexture = m_device->createDepthBuffer(m_engine.getWindowSize().x, m_engine.getWindowSize().y);

    m_material = Material::Builder()
                    .uniformBlock(0, "UBO", 224)
                    .uniformMember("UBO", "projection", rhi::UniformType::Mat4, 0, 64)
                    .uniformMember("UBO", "view", rhi::UniformType::Mat4, 64, 64)
                    .uniformMember("UBO", "model", rhi::UniformType::Mat4, 128, 64)
                    .uniformMember("UBO", "lightDirection", rhi::UniformType::Float4, 192, 16)
                    .uniformMember("UBO", "eyePosition", rhi::UniformType::Float3, 208, 12)
                    .uniformMember("UBO", "exposure", rhi::UniformType::Float, 220, 4)
                    .build(m_engine);

    m_materialInstance = m_material->createInstance();

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
    scene->traverseNodes(scene->getRoot(), [this](Node* node) {
        for (const auto& component : node->getComponents()) {
            auto renderables = component->getRenderables();
            for (const auto& renderable : renderables) {
                RenderCommand cmd;
                cmd.vertexBuffer = renderable->getVertexBuffer();
                cmd.indexBuffer = renderable->getIndexBuffer();
                cmd.materialInstance = renderable->getMaterialInstance();
                cmd.pipelineHandle = renderable->getPipelineHandle();
                cmd.matWorld = node->getTransform().getWorldMatrix();

                if (cmd.indexBuffer != nullptr) {
                    cmd.indexCount = cmd.indexBuffer->getIndexCount();
                    cmd.indexOffset = 0;
                } else {
                    cmd.vertexCount = cmd.vertexBuffer->getVertexCount();
                    cmd.vertexOffset = 0;
                }

                m_renderQueue.addCommand(cmd);
            }
        }
    });

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
        m_materialInstance->setFrameIndex(frameIndex);

        m_materialInstance->setParameter("model", cmd.matWorld);
        m_materialInstance->setParameter("view", view->getCamera()->getView());
        m_materialInstance->setParameter("projection", view->getCamera()->getProjection());
        m_materialInstance->setParameter("eyePosition", view->getCamera()->getPosition());
        m_materialInstance->setParameter("lightDirection", math::vec3(0.0f, -1.0f, 0.0f));
        m_materialInstance->setParameter("exposure", 1.0f);

        m_materialInstance->commit(m_engine);

        // Bind pipeline, descriptor sets, vertex/index buffers, and draw
        commandBuffer->bindPipeline(cmd.pipelineHandle);
        commandBuffer->bindDescriptorSets(cmd.pipelineHandle,
                                          m_materialInstance->getDescriptorSetHandle(), 0, 1);
        commandBuffer->bindDescriptorSets(cmd.pipelineHandle,
                                          cmd.materialInstance->getDescriptorSetHandle(), 1, 1);

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

