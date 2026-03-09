#include "ocf/VertexBuffer.h"
#include "ocf/OpenGLUtility.h"
#include <glad/gl.h>
#include <algorithm>
#include <iostream>

namespace ocf {

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
    return *this;
}

VertexBuffer* VertexBuffer::Builder::build()
{
    VertexBuffer* vertexBuffer = new VertexBuffer(*this);
    return vertexBuffer;
}

VertexBuffer::VertexBuffer(const Builder& builder)
    : m_vertexCount(builder->vertexCount)
{
    // calculate buffer sizes
    size_t bufferSize = {};
    for (size_t i = 0; i < VERTEX_ATTRIBUTE_COUNT_MAX; i++) {
        m_attributes[i] = builder->attributes[i];

        const auto& entry = m_attributes[i];
        if (entry.buffer != Attribute::BUFFER_UNUSED) {
            const size_t size = entry.stride * m_vertexCount;
            bufferSize = std::max(bufferSize, size);
        }
    }
    m_bufferSize = bufferSize;

    static constexpr uint32_t kMaxVersion =
    std::numeric_limits<decltype(m_version)>::max();
    const uint32_t version = m_version;
    m_version = (version + 1) % kMaxVersion;

    glGenBuffers(1, &m_handle);
    glBindBuffer(GL_ARRAY_BUFFER, m_handle);
    glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    CHECK_GL_ERROR(std::cerr);
}

VertexBuffer::~VertexBuffer()
{
    glDeleteBuffers(1, &m_handle);
    m_handle = 0;
}

void VertexBuffer::setBufferData(const void* data, size_t size, size_t offset) noexcept
{
    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, m_handle);

    if (offset == 0 && size == m_bufferSize) {
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    }
    else {
        glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    CHECK_GL_ERROR(std::cerr);
}

} // namespace ocf
