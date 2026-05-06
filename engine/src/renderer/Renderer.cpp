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

const math::vec3 A(-0.5f, 0.5f, 0.5f), B(-0.5f, -0.5f, 0.5f), C(0.5f, 0.5f, 0.5f),
D(0.5f, -0.5f, 0.5f), E(-0.5f, 0.5f, -0.5f), F(-0.5f, -0.5f, -0.5f), G(0.5f, 0.5f, -0.5f),
H(0.5f, -0.5f, -0.5f);

std::vector<Vertex> vertices = {
    // front
    {A, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f}},
    {B, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
    {C, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
    {D, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.0f}},
    // back
    {E, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},
    {F, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 0.0f}},
    {G, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 1.0f}},
    {H, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
    // right
    {C, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
    {D, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}},
    {G, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f}},
    {H, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    // left
    {E, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {F, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
    {A, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f}},
    {B, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    // top
    {E, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {A, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}},
    {G, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 1.0f}},
    {C, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
    // bottom
    {B, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {F, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
    {D, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}},
    {H, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
};

std::vector<uint32_t> indices = {
    0,  1,  2,  2,  1,  3,  // front
    6,  7,  4,  4,  7,  5,  // back
    8,  9,  10, 10, 9,  11, // right
    12, 13, 14, 14, 13, 15, // left
    16, 17, 18, 18, 17, 19, // top
    20, 21, 22, 22, 21, 23, // bottom
};


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
                        .indexCount(uint32_t(indices.size()))
                        .build(m_engine);
    m_indexBuffer->setBufferData(m_engine, indices.data(), indexBufferSize, 0);

    auto vsPath = FileSystem::getInstance()->getAssetFullPath("shaders/cube.vert.spv");
    auto fsPath = FileSystem::getInstance()->getAssetFullPath("shaders/cube.frag.spv");

    ShaderModuleHandle vs = m_device->createShaderModule(ShaderStage::Vertex, vsPath);
    ShaderModuleHandle fs = m_device->createShaderModule(ShaderStage::Fragment, fsPath);

    DescriptorSetLayout descriptorSetLayout;
    DescriptorLayoutBinding binding{
        .binding = 0,
        .type = DescriptorType::UniformBuffer,
        .shaderStageFlags = ShaderStageFlags::Vertex | ShaderStageFlags::Fragment
    };
    descriptorSetLayout.descriptors.push_back(binding);
    m_device->createDescriptorLayoutSet(descriptorSetLayout);

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
    commandBuffer->bindIndexBuffer(m_indexBuffer->getHandle(), 0);
    commandBuffer->drawIndexed(uint32_t(indices.size()), 1, 0, 0, 0);

    commandBuffer->endRendering();

    // Change to present layout
    commandBuffer->transitionLayout(rhi::ResourceState::ColorAttachment,
                                    rhi::ResourceState::Present);
    commandBuffer->end();
}

} // namespace ocf
