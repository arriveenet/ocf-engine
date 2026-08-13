// SPDX-License-Identifier: MIT
#include "ocf/scene/MeshInstance.h"

#include "ocf/core/Engine.h"
#include "ocf/renderer/Material.h"
#include "ocf/renderer/MaterialInstance.h"
#include "ocf/renderer/Renderer.h"
#include "ocf/renderer/VertexBuffer.h"
#include "ocf/renderer/IndexBuffer.h"
#include "ocf/resource/Mesh.h"
#include "ocf/rhi/Device.h"
#include "ocf/rhi/RHIEnums.h"

namespace ocf {

MeshInstance::MeshInstance(Engine& engine, std::string_view vertexShaderPath,
                           std::string_view fragmentShaderPath)
    : m_engine(engine)
{
    auto& device = m_engine.getDevice();
    m_vertexShader = device.createShaderModule(rhi::ShaderStage::Vertex, vertexShaderPath);
    m_fragmentShader = device.createShaderModule(rhi::ShaderStage::Fragment, fragmentShaderPath);

    m_material = Material::Builder()
            .uniformBlock(0, "MaterialParameters", 80)
            .uniformMember("MaterialParameters", "baseColorFactor", rhi::UniformType::Float4, 0, 16)
            .uniformMember("MaterialParameters", "metallicFactor", rhi::UniformType::Float, 16, 4)
            .uniformMember("MaterialParameters", "roughnessFactor", rhi::UniformType::Float, 20, 4)
            .uniformMember("MaterialParameters", "alphaCutoff", rhi::UniformType::Float, 24, 4)
            .uniformMember("MaterialParameters", "alphaMode", rhi::UniformType::Int, 28, 4)
            .uniformMember("MaterialParameters", "hasNormalMap", rhi::UniformType::Int, 32, 4)
            .uniformMember("MaterialParameters", "padding0", rhi::UniformType::Int, 36, 4)
            .uniformMember("MaterialParameters", "padding1", rhi::UniformType::Int, 40, 4)
            .uniformMember("MaterialParameters", "padding2", rhi::UniformType::Int, 44, 4)
            .texture(1, "baseColorTexture")
            .texture(2, "metallicRoughnessTexture")
            .texture(3, "normalMapTexture")
            .build(engine);

    m_materialInstance = m_material->createInstance();
}

MeshInstance::~MeshInstance()
{
    clear();
}

void MeshInstance::clear()
{
    auto& device = m_engine.getDevice();
    for (auto renderable : m_renderables) {
        device.destroyPipeline(renderable->getPipelineHandle());
        delete renderable;
    }
    m_renderables.clear();

    m_material->terminate(m_engine);
    delete m_material;
    m_material = nullptr;
    m_materialInstance = nullptr;
    m_mesh = nullptr;
}

void MeshInstance::setMesh(const Ref<Mesh>& mesh)
{
    if (m_mesh == mesh) {
        return;
    }

    auto& device = m_engine.getDevice();
    for (auto renderable : m_renderables) {
        device.destroyPipeline(renderable->getPipelineHandle());
        delete renderable;
    }
    m_renderables.clear();

    m_mesh = mesh;

    Material* uboMaterial = m_engine.getRenderer().getUBOMaterial();

    int subMeshCount = mesh->getSubMeshCount();
    for (int i = 0; i < subMeshCount; i++) {
        const auto& subMesh = mesh->getSubMesh(i);

        MaterialInstance* materialInstance = m_material->createInstance();    

        materialInstance->setParameter("baseColorFactor", subMesh.materialParams.baseColorFactor);
        materialInstance->setParameter("metallicFactor", subMesh.materialParams.metallicFactor);
        materialInstance->setParameter("roughnessFactor", subMesh.materialParams.roughnessFactor);
        materialInstance->setParameter("alphaCutoff", subMesh.materialParams.alphaCutoff);
        materialInstance->setParameter("alphaMode", 0);
        materialInstance->setParameter("hasNormalMap", int(subMesh.materialParams.hasNormalMap));
        materialInstance->setParameter("baseColorTexture", subMesh.baseColorTexture, subMesh.sampler);
        materialInstance->setParameter("metallicRoughnessTexture",
                                         subMesh.metallicRoughnessTexture, subMesh.sampler);
        materialInstance->setParameter("normalMapTexture", subMesh.normalMapTexture, subMesh.sampler);

        materialInstance->commit(m_engine);

        rhi::PipelineState pipeline;
        pipeline.vertexShader = m_vertexShader;
        pipeline.fragmentShader = m_fragmentShader;
        pipeline.vertexBufferInfo = subMesh.vertexBuffer->getVertexBufferInfoHandle();
        pipeline.pipelineLayout.setLayout[0] = uboMaterial->getDescriptorSetLayout().getHandle();
        pipeline.pipelineLayout.setLayout[1] = m_material->getDescriptorSetLayout().getHandle();
        pipeline.rasterState = subMesh.materialParams.rasterState;

        auto pipelineHandle = m_engine.getDevice().createPipeline(pipeline);

        Renderable* renderable = new Renderable(subMesh.vertexBuffer, subMesh.indexBuffer,
                                                materialInstance, pipelineHandle);
        m_renderables.push_back(renderable);
    }

    device.destroyShaderModule(m_vertexShader);
    device.destroyShaderModule(m_fragmentShader);
}

Ref<Mesh> MeshInstance::getMesh() const noexcept
{
    return m_mesh;
}

} // namespace ocf
