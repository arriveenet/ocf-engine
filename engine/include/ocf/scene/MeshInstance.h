// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/Reference.h"
#include "ocf/scene/Component.h"

#include <memory>
#include <string_view>

namespace ocf {

class Mesh;
class Material;
class MaterialInstance;
class Engine;

class MeshInstance : public Component {
public:
    MeshInstance(Engine& engine, std::string_view vertexShaderPath,
                 std::string_view fragmentShaderPath);
    ~MeshInstance() override;

    void setMesh(const Ref<Mesh>& mesh);

    Ref<Mesh> getMesh() const noexcept;

    std::shared_ptr<MaterialInstance> getMaterialInstance() const noexcept
    {
        return m_materialInstance;
    }

private:
    Engine& m_engine;
    Ref<Mesh> m_mesh;
    rhi::ShaderModuleHandle m_vertexShader;
    rhi::ShaderModuleHandle m_fragmentShader;
    rhi::PipelineHandle m_pipelineHandle;
    Material* m_material;
    std::shared_ptr<MaterialInstance> m_materialInstance;
};

} // namespace ocf
