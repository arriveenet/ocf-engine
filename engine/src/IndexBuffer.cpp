#include "ocf/IndexBuffer.h"
#include "ocf/OpenGLUtility.h"
#include <glad/gl.h>
#include <iostream>

namespace ocf {

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

IndexBuffer* IndexBuffer::Builder::build()
{
    IndexBuffer* indexBuffer = new IndexBuffer(*this);
    return indexBuffer;
}

IndexBuffer::IndexBuffer(const Builder& builder)
    : m_indexCount(builder->indexCount)
    , m_indexType(builder->indexType)
{
    const size_t bufferSize =
        m_indexCount * (m_indexType == IndexType::USHORT ? sizeof(uint16_t) : sizeof(uint32_t));
    glGenBuffers(1, &m_handle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_handle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferSize, nullptr, GL_STATIC_DRAW);

    CHECK_GL_ERROR(std::cerr);
}

IndexBuffer::~IndexBuffer()
{
    glDeleteBuffers(1, &m_handle);
    m_handle = 0;
}

void IndexBuffer::setBufferData(const void* data, size_t size, size_t offset) noexcept
{
    glBindVertexArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_handle);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, size, data);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    CHECK_GL_ERROR(std::cerr);
}

} // namespace ocf
