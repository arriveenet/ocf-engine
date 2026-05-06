#include "ocf/renderer/IndexBuffer.h"

#include "ocf/core/Engine.h"
#include "ocf/rhi/Device.h"
#include <iostream>

namespace ocf {

using namespace rhi;

struct IndexBuffer::BuilderDetails {
    uint32_t indexCount = 0;
    IndexBuffer::IndexType indexType = IndexBuffer::IndexType::USHORT;
};

IndexBuffer::Builder::Builder()
{
    m_impl = new BuilderDetails();
}

IndexBuffer::Builder::~Builder()
{
    delete m_impl;
}

IndexBuffer::Builder& IndexBuffer::Builder::indexCount(uint32_t indexCount) noexcept
{
    m_impl->indexCount = indexCount;
    return *this;
}

IndexBuffer* IndexBuffer::Builder::build(Engine& engine)
{
    IndexBuffer* indexBuffer = new IndexBuffer(engine, *this);
    return indexBuffer;
}

IndexBuffer::IndexBuffer(Engine& engine, const Builder& builder)
    : m_indexCount(builder->indexCount)
    , m_indexType(builder->indexType)
{
    Engine::Device& device = engine.getDevice();
    m_handle = device.createIndexBuffer(static_cast<ElementType>(m_indexType), m_indexCount,
                                        BufferUsage::Static);
}

IndexBuffer::~IndexBuffer()
{
}

void IndexBuffer::setBufferData(Engine& engine, const void* data, size_t size,
                                size_t offset) noexcept
{
    engine.getDevice().updateIndexBufferData(m_handle, data, size, offset);
}

} // namespace ocf
