// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/rhi/Handle.h"

namespace ocf {

class VertexBuffer;
class IndexBuffer;
class MaterialInstance;

class Renderable {
public:
    virtual ~Renderable() = default;

    VertexBuffer* getVertexBuffer() const noexcept { return m_vertexBuffer; }

    IndexBuffer* getIndexBuffer() const noexcept { return  m_indexBuffer; }

    MaterialInstance* getMaterialInstance() const noexcept { return m_materialInstance; }

    rhi::PipelineHandle getPipelineHandle() const noexcept { return m_pipelineHandle; }

protected:
    VertexBuffer* m_vertexBuffer = nullptr;
    IndexBuffer* m_indexBuffer = nullptr;
    MaterialInstance* m_materialInstance = nullptr;
    rhi::PipelineHandle m_pipelineHandle;
};

} // namespace ocf
