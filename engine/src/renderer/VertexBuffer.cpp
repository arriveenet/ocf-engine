#include "ocf/renderer/VertexBuffer.h"

#include "ocf/core/Engine.h"
#include "ocf/core/Logger.h"
#include "ocf/rhi/Device.h"


namespace ocf {

using namespace rhi;

struct VertexBuffer::BuilderDetails {
    uint8_t bufferCount = 0;
    uint32_t vertexCount = 0;
    AttributeArray attributes;
};

VertexBuffer::Builder::Builder()
{
    m_impl = new BuilderDetails();
}

VertexBuffer::Builder::~Builder()
{
    delete m_impl;
}

VertexBuffer::Builder& VertexBuffer::Builder::bufferCount(uint8_t bufferCount) noexcept
{
    m_impl->bufferCount = bufferCount;
    return *this;
}

VertexBuffer::Builder& VertexBuffer::Builder::vertexCount(uint32_t vertexCount) noexcept
{
    m_impl->vertexCount = vertexCount;
    return *this;
}

VertexBuffer::Builder& VertexBuffer::Builder::attribute(VertexAttribute attribute,
                                                        AttributeType type, uint8_t stride,
                                                        uint32_t offset) noexcept
{
    if (size_t(attribute) < VERTEX_ATTRIBUTE_COUNT_MAX) {
        auto& entry = m_impl->attributes[size_t(attribute)];
        entry.type = type;
        entry.stride = stride;
        entry.offset = offset;
        entry.buffer = 0;
    }
    else {
        OCF_LOG_WARN("Ignore VertexBuffe attribute, the limit of {} attributes has been "
                     "execeeded.",
                     VERTEX_ATTRIBUTE_COUNT_MAX);
    }
    return *this;
}

VertexBuffer* VertexBuffer::Builder::build(Engine& engine)
{
    VertexBuffer* vertexBuffer = new VertexBuffer(engine, *this);
    return vertexBuffer;
}

VertexBuffer::VertexBuffer(Engine& engine, const Builder& builder)
{
    // calculate buffer sizes
    uint32_t bufferSize = {};
    for (size_t i = 0; i < VERTEX_ATTRIBUTE_COUNT_MAX; i++) {
        m_attributes[i] = builder->attributes[i];

        const auto& entry = m_attributes[i];
        if (entry.buffer != Attribute::BUFFER_UNUSED) {
            const uint32_t size = entry.stride * m_vertexCount;
            bufferSize = std::max(bufferSize, size);
        }
    }
    m_byteCount = bufferSize;

    // create the buffer
    Engine::Device& device = engine.getDevice();
    m_vertexBufferInfoHandle = device.createVertexBufferInfo(0, m_attributes);
    m_handle =
        device.createVertexBuffer(m_vertexCount, m_byteCount, m_usage, m_vertexBufferInfoHandle);
}

void VertexBuffer::terminate(Engine& engine)
{
    engine.getDevice().destroyVertexBuffer(m_handle);
}

void VertexBuffer::setBufferData(Engine& engine, const void* data, size_t size, size_t offset) const
{
    engine.getDevice().updateBufferData(m_handle, data, size, offset);
}

} // namespace ocf
