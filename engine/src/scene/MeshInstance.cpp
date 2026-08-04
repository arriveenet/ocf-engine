// SPDX-License-Identifier: MIT
#include "ocf/scene/MeshInstance.h"

#include "ocf/resource/Mesh.h"
#include "ocf/core/Engine.h"
#include "ocf/rhi/Device.h"
#include "ocf/rhi/RHIEnums.h"
#include "ocf/renderer/VertexBuffer.h"
#include "ocf/renderer/Material.h"

namespace ocf {

MeshInstance::MeshInstance(Engine& engine, std::string_view vertexShaderPath,
                           std::string_view fragmentShaderPath)
    : m_engine(engine)
{
    auto& device = m_engine.getDevice();
    m_vertexShader = device.createShaderModule(rhi::ShaderStage::Vertex, vertexShaderPath);
    m_fragmentShader = device.createShaderModule(rhi::ShaderStage::Fragment, fragmentShaderPath);

    m_material = Material::Builder()
                     .uniformBlock(0, "SceneContents", 224)
                     .uniformMember("SceneContents", "matWorld", rhi::UniformType::Mat4, 0, 64)
                     .uniformMember("SceneContents", "matView", rhi::UniformType::Mat4, 64, 64)
                     .uniformMember("SceneContents", "matProj", rhi::UniformType::Mat4, 128, 64)
                     .texture(1, "gTex")
                     .build(engine);

    m_materialInstance = m_material->createInstance();
}

MeshInstance::~MeshInstance()
{
}

void MeshInstance::setMesh(const Ref<Mesh>& mesh)
{
    if (m_mesh == mesh) {
        return;
    }

    m_mesh = mesh;

    int subMeshCount = mesh->getSubMeshCount();
    for (int i = 0; i < subMeshCount; i++) {
        const auto& subMesh = mesh->getSubMesh(i);

        rhi::PipelineState pipeline;
        pipeline.vertexShader = m_vertexShader;
        pipeline.fragmentShader = m_fragmentShader;
        pipeline.vertexBufferInfo = subMesh.vertexBuffer->getVertexBufferInfoHandle();
        pipeline.layout = m_material->getDescriptorSetLayout().getHandle();

        m_pipelineHandle = m_engine.getDevice().createPipeline(pipeline);

        Renderable* renderable = new Renderable(subMesh.vertexBuffer, subMesh.indexBuffer,
                                                m_materialInstance.get(), m_pipelineHandle);
        m_renderables.push_back(renderable);
    }
}

Ref<Mesh> MeshInstance::getMesh() const noexcept
{
    return m_mesh;
}

} // namespace ocf
