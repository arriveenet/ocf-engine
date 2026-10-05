// SPDX-License-Identifier: MIT

#include "ocf/renderer/Renderer.h"

#include "ocf/core/Engine.h"
#include "ocf/math/constants.h"
#include "ocf/math/geometric.h"
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

Renderer::Renderer(Engine& engine, rhi::Device* device)
    : m_engine(engine)
    , m_device(device)
{
}

Renderer::~Renderer()
{
    m_material->terminate(m_engine);
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

    const math::vec3 cameraPosition = view->getCamera()->getPosition();

    // Collect renderable objects from the scene
    scene->traverseNodes(scene->getRoot(), [this, &cameraPosition](Node* node) {
        for (const auto& component : node->getComponents()) {
            auto renderables = component->getRenderables();
            for (const auto& renderable : renderables) {
                RenderCommand cmd;
                cmd.vertexBuffer = renderable->getVertexBuffer();
                cmd.indexBuffer = renderable->getIndexBuffer();
                cmd.materialInstance = renderable->getMaterialInstance();
                cmd.pipelineHandle = renderable->getPipelineHandle();
                cmd.matWorld = node->getTransform().getWorldMatrix();
                cmd.alphaMode = renderable->getAlphaMode();

                const math::vec3& center = renderable->getCenter();
                const math::vec4 worldCenter = cmd.matWorld * math::vec4(center.x, center.y, center.z, 1.0f);
                const math::vec3 toCamera = math::vec3(worldCenter.x, worldCenter.y, worldCenter.z) - cameraPosition;
                cmd.distanceToCamera = math::dot(toCamera, toCamera);

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

    m_renderQueue.sort();

    const uint32_t frameIndex = m_device->getCurrentFrameIndex();

    auto commandBuffer = m_device->getCommandBuffer();
    commandBuffer->begin();
    // Change to color layout
    commandBuffer->transitionLayout(rhi::ResourceState::Undefined,
                                    rhi::ResourceState::ColorAttachment);
    commandBuffer->transitionLayout(m_depthTexture, rhi::ResourceState::Undefined,
                                    rhi::ResourceState::DepthStencilAttachment);

    rhi::RenderingInfo info;
    info.clearColor = {0.0f, 0.0f, 1.0f, 1.0f};
    commandBuffer->beginRendering(info);

    for (auto& cmd : m_renderQueue.getRenderCommands()) {
        m_materialInstance->setFrameIndex(frameIndex);

        m_materialInstance->setParameter("model", cmd.matWorld);
        m_materialInstance->setParameter("view", view->getCamera()->getView());
        m_materialInstance->setParameter("projection", view->getCamera()->getProjection());
        m_materialInstance->setParameter("eyePosition", view->getCamera()->getPosition());
        m_materialInstance->setParameter("lightDirection", math::vec4(0.0f, 1.0f, 1.0f, 0.0f));
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
