// SPDX-License-Identifier: MIT

#include "ocf/renderer/Renderer.h"

#include "ocf/core/Engine.h"
#include "ocf/platform/FileSystem.h"
#include "ocf/rhi/CommandBuffer.h"
#include "ocf/rhi/Device.h"

#include <cstddef>
#include <ocf/rhi/PipelineState.h>
#include <vector>

namespace ocf {

using namespace rhi;

Renderer::Renderer(Engine& engine, rhi::Device* device)
    : m_engine(engine)
    , m_device(device)
{
}

Renderer::~Renderer()
{
    m_device->destroyPipeline(m_pipelineHandle);
    m_vertexBuffer->terminate(m_engine);
}

bool Renderer::init()
{
    const std::vector<Vertex> triangleVertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}}, // Red
        {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}}, // Green
        {{ 0.0f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}, // Blue
    };
    const size_t bufferSize = sizeof(Vertex) * triangleVertices.size();

    m_vertexBuffer = VertexBuffer::Builder()
                         .attribute(VertexAttribute::POSITION, VertexBuffer::AttributeType::FLOAT3,
                                    sizeof(Vertex), offsetof(Vertex, position))
                         .attribute(VertexAttribute::COLOR, VertexBuffer::AttributeType::FLOAT3,
                                    sizeof(Vertex), offsetof(Vertex, color))
                         .bufferCount(1)
                         .vertexCount(3)
                         .build(m_engine);
    m_vertexBuffer->setBufferData(m_engine, triangleVertices.data(), bufferSize, 0);

    auto vsPath = FileSystem::getInstance()->getAssetFullPath("shaders/triangle.vert.spv");
    auto fsPath = FileSystem::getInstance()->getAssetFullPath("shaders/triangle.frag.spv");

    ShaderModuleHandle vs = m_device->createShaderModule(ShaderStage::Vertex, vsPath);
    ShaderModuleHandle fs = m_device->createShaderModule(ShaderStage::Fragment, fsPath);

    PipelineState pipeline;
    pipeline.vertexShader = vs;
    pipeline.fragmentShader = fs;
    pipeline.vertexBufferInfo = m_vertexBuffer->getVertexBufferInfoHandle();

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
    auto commandBuffer = m_device->getCommandBuffer();
    commandBuffer->begin();
    // Change to color layout
    commandBuffer->transitionLayout(rhi::ResourceState::Undefined,
                                    rhi::ResourceState::ColorAttachment);

    rhi::RenderingInfo info;
    info.clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    commandBuffer->beginRendering(info);

    commandBuffer->bindPipeline(m_pipelineHandle);
    commandBuffer->bindVertexBuffers(0, 1, m_vertexBuffer->getHandle());
    commandBuffer->draw(3, 1, 0, 0);

    commandBuffer->endRendering();

    // Change to present layout
    commandBuffer->transitionLayout(rhi::ResourceState::ColorAttachment,
                                    rhi::ResourceState::Present);
    commandBuffer->end();
}

} // namespace ocf
