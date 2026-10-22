// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/rhi/Device.h"
#include "ocf/math/vec3.h"
#include "ocf/renderer/MaterialEnums.h"
#include "ocf/rhi/Handle.h"

namespace ocf {

class VertexBuffer;
class IndexBuffer;
class MaterialInstance;

class Renderable {
public:
    Renderable(VertexBuffer* vertexBuffer, IndexBuffer* indexBuffer,
               MaterialInstance* materialInstance, rhi::PipelineHandle pipelineHandle,
               AlphaMode alphaMode = AlphaMode::Opaque, const math::vec3& center = math::vec3(0.0f))
        : m_vertexBuffer(vertexBuffer)
        , m_indexBuffer(indexBuffer)
        , m_materialInstance(materialInstance)
        , m_pipelineHandle(pipelineHandle)
        , m_alphaMode(alphaMode)
        , m_center(center)
    {
    }

    virtual ~Renderable() = default;

    VertexBuffer* getVertexBuffer() const noexcept { return m_vertexBuffer; }

    IndexBuffer* getIndexBuffer() const noexcept { return  m_indexBuffer; }

    MaterialInstance* getMaterialInstance() const noexcept { return m_materialInstance; }

    rhi::PipelineHandle getPipelineHandle() const noexcept { return m_pipelineHandle; }

    AlphaMode getAlphaMode() const noexcept { return m_alphaMode; }

    const math::vec3& getCenter() const noexcept { return m_center; }

protected:
    VertexBuffer* m_vertexBuffer = nullptr;
    IndexBuffer* m_indexBuffer = nullptr;
    MaterialInstance* m_materialInstance = nullptr;
    rhi::PipelineHandle m_pipelineHandle;
    AlphaMode m_alphaMode = AlphaMode::Opaque;
    math::vec3 m_center = math::vec3(0.0f);
};

} // namespace ocf
