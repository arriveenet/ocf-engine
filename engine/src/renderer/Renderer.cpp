// SPDX-License-Identifier: MIT

#include "ocf/renderer/Renderer.h"

#include "ocf/core/Engine.h"
#include "ocf/rhi/CommandBuffer.h"
#include "ocf/rhi/Device.h"

#include <vector>

namespace ocf {

using namespace rhi;

Renderer::Renderer(Engine& engine, rhi::Device* device)
    : m_engine(engine)
    , m_device(device)
{
    const std::vector<Vertex> triangleVertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}}, // Red
        {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}}, // Green
        {{ 0.0f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}, // Blue
    };
    const size_t bufferSize = sizeof(Vertex) * triangleVertices.size();

    m_vertexBuffer = VertexBuffer::Builder().bufferCount(3).build(engine);
    m_vertexBuffer->setBufferData(engine, triangleVertices.data(), bufferSize, 0);
}

Renderer::~Renderer()
{
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
    auto commandBuffer = m_device->getCommandBuffer();
    commandBuffer->begin();
    // Change to color layout
    commandBuffer->transitionLayout(rhi::ResourceState::Undefined,
                                    rhi::ResourceState::ColorAttachment);

    rhi::RenderingInfo info;
    commandBuffer->beginRendering(info);

    commandBuffer->endRendering();

    // Change to present layout
    commandBuffer->transitionLayout(rhi::ResourceState::ColorAttachment,
                                    rhi::ResourceState::Present);
    commandBuffer->end();
}

} // namespace ocf
