// SPDX-License-Identifier: MIT

#include "ocf/renderer/Renderer.h"

#include "ocf/rhi/CommandBuffer.h"
#include "ocf/rhi/Device.h"

namespace ocf {

Renderer::Renderer(rhi::Device* device)
    : m_device(device)
{
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
