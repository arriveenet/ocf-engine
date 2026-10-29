// SPDX-License-Identifier: MIT

#include "ocf/renderer/Renderer.h"

#include "ocf/renderer/ImGuiLayer.h"

#include "ocf/core/Engine.h"
#include "ocf/math/geometric.h"
#include "ocf/renderer/IndexBuffer.h"
#include "ocf/renderer/Material.h"
#include "ocf/renderer/MaterialInstance.h"
#include "ocf/renderer/RenderQueue.h"
#include "ocf/renderer/RenderTarget.h"
#include "ocf/renderer/VertexBuffer.h"
#include "ocf/rhi/CommandBuffer.h"
#include "ocf/rhi/Device.h"
#include "ocf/scene/Camera.h"
#include "ocf/scene/Node.h"
#include "ocf/scene/SceneGraph.h"
#include "ocf/scene/View.h"

#include <vector>

namespace ocf {

Renderer::Renderer(Engine& engine, rhi::Device* device)
    : m_engine(engine)
    , m_device(device)
{
}

Renderer::~Renderer()
{
    m_device->destroyRenderTarget(m_defaultRenderTarget);

    m_material->terminate(m_engine);
    delete m_material;
}

bool Renderer::init()
{
    m_depthTexture = m_device->createDepthBuffer(m_engine.getWindowSize().x, m_engine.getWindowSize().y);
    m_defaultRenderTarget = m_device->createDefaultRenderTarget();

    // The model matrix is not here: it changes per draw and is a push constant
    m_material = Material::Builder()
                .uniformBlock(0, "UBO", 160)
                .uniformMember("UBO", "projection", rhi::UniformType::Mat4, 0, 64)
                .uniformMember("UBO", "view", rhi::UniformType::Mat4, 64, 64)
                .uniformMember("UBO", "lightDirection", rhi::UniformType::Float4, 128, 16)
                .uniformMember("UBO", "eyePosition", rhi::UniformType::Float3, 144, 12)
                .uniformMember("UBO", "exposure", rhi::UniformType::Float, 156, 4)
                .build(m_engine);

    return true;
}

bool Renderer::beginFrame()
{
    if (!m_device->beginFrame()) {
        return false;
    }

    m_viewCount = 0;
    m_passCount = 0;

    m_commandBuffer = m_device->getCommandBuffer();
    m_commandBuffer->begin();
    // Change to color layout
    m_commandBuffer->transitionLayout(rhi::ResourceState::Undefined,
                                      rhi::ResourceState::ColorAttachment);
    m_commandBuffer->transitionLayout(m_depthTexture, rhi::ResourceState::Undefined,
                                      rhi::ResourceState::DepthStencilAttachment);
    return true;
}

void Renderer::endFrame()
{
    // Never present an undefined image when nothing was drawn
    if (m_passCount == 0) {
        rhi::RenderingInfo info;
        info.clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        m_commandBuffer->beginRendering(m_defaultRenderTarget, info);
        m_commandBuffer->endRendering();
    }

    // Change to present layout
    m_commandBuffer->transitionLayout(rhi::ResourceState::ColorAttachment,
                                      rhi::ResourceState::Present);
    m_commandBuffer->end();
    m_commandBuffer.reset();

    m_device->endFrame();
}

MaterialInstance* Renderer::acquireViewUniforms()
{
    if (m_viewCount == m_viewUniforms.size()) {
        m_viewUniforms.push_back(m_material->createInstance());
    }
    return m_viewUniforms[m_viewCount++];
}

void Renderer::render(const View* view)
{
    if (view == nullptr || !view->hasCamera()) {
        return;
    }

    const Camera* camera = view->getCamera();

    m_renderQueue.clear();

    const math::vec3 cameraPosition = camera->getPosition();

    const auto collect = [this, &cameraPosition](Node* node) {
        for (const auto& component : node->getComponents()) {
            for (const auto& renderable : component->getRenderables()) {
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
    };

    // Collect renderable objects from the scene graphs
    for (SceneGraph* sceneGraph : view->getSceneGraphs()) {
        sceneGraph->traverseNodes(sceneGraph->getRoot(), collect);
    }

    // SubmitOrder keeps the traversal order, so that UI elements later in the tree are on top
    if (view->getSortMode() == View::SortMode::Distance) {
        m_renderQueue.sort();
    }

    // The camera uniforms are written once per view; each view has its own buffer so that
    // views drawn later in the frame do not overwrite them
    MaterialInstance* viewUniforms = acquireViewUniforms();
    viewUniforms->setFrameIndex(m_device->getCurrentFrameIndex());
    viewUniforms->setParameter("view", camera->getView());
    viewUniforms->setParameter("projection", camera->getProjection());
    viewUniforms->setParameter("eyePosition", camera->getPosition());
    viewUniforms->setParameter("lightDirection", math::vec4(0.0f, 1.0f, 1.0f, 0.0f));
    viewUniforms->setParameter("exposure", 1.0f);
    viewUniforms->commit(m_engine);
    const rhi::DescriptorSetHandle viewDescriptorSet = viewUniforms->getDescriptorSetHandle();

    // The swapchain attachments are undefined at the start of a frame, so the first pass into
    // them always clears. A render target keeps its contents, and the device clears it the first
    // time it is drawn.
    const RenderTarget* renderTarget = view->getRenderTarget();
    const bool isDefaultTarget = (renderTarget == nullptr);
    const bool isFirstPass = isDefaultTarget && (m_passCount == 0);
    const View::ClearFlags clearFlags = view->getClearFlags();
    rhi::RenderingInfo info;
    info.clearColor = view->getClearColor();
    info.clearColorBuffer = isFirstPass || hasFlag(clearFlags, View::ClearFlags::Color);
    info.clearDepthBuffer = isFirstPass || hasFlag(clearFlags, View::ClearFlags::Depth);
    m_commandBuffer->beginRendering(
        isDefaultTarget ? m_defaultRenderTarget : renderTarget->getHandle(), info);
    if (isDefaultTarget) {
        m_passCount++;
    }

    const uint32_t frameIndex = m_device->getCurrentFrameIndex();
    for (const auto& cmd : m_renderQueue.getRenderCommands()) {
        // The fence of this frame was waited, so its copy of the material can be written
        cmd.materialInstance->setFrameIndex(frameIndex);
        cmd.materialInstance->commit(m_engine);

        // Bind pipeline, descriptor sets, vertex/index buffers, and draw
        m_commandBuffer->bindPipeline(cmd.pipelineHandle);
        m_commandBuffer->bindDescriptorSets(cmd.pipelineHandle, viewDescriptorSet, 0, 1);
        m_commandBuffer->bindDescriptorSets(cmd.pipelineHandle,
                                            cmd.materialInstance->getDescriptorSetHandle(), 1, 1);
        m_commandBuffer->pushConstants(cmd.pipelineHandle, OBJECT_PUSH_CONSTANT_RANGE.stages, 0,
                                       OBJECT_PUSH_CONSTANT_RANGE.size, &cmd.matWorld);

        // Draw Indexed
        if (cmd.indexBuffer != nullptr) {
            m_commandBuffer->bindVertexBuffers(0, 1, cmd.vertexBuffer->getHandle());
            m_commandBuffer->bindIndexBuffer(cmd.indexBuffer->getHandle(), 0);
            m_commandBuffer->drawIndexed(cmd.indexCount, 1, cmd.indexOffset, 0, 0);
        }
        // Draw Arrays
        else {
            m_commandBuffer->bindVertexBuffers(0, 1, cmd.vertexBuffer->getHandle());
            m_commandBuffer->draw(cmd.vertexCount, 1, cmd.vertexOffset, 0);
        }
    }

    m_commandBuffer->endRendering();
}

#if OCF_ENABLE_IMGUI
void Renderer::renderImGui(ImGuiLayer& imGuiLayer)
{
    // The UI does not depth test. The color attachment is undefined if no view was drawn.
    rhi::RenderingInfo info;
    info.clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    info.clearColorBuffer = (m_passCount == 0);
    info.useDepthBuffer = false;
    m_commandBuffer->beginRendering(m_defaultRenderTarget, info);
    m_passCount++;

    imGuiLayer.render(*m_commandBuffer);

    m_commandBuffer->endRendering();
}
#endif

} // namespace ocf
