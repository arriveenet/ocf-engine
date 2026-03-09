#include "ocf/VertexArrayObject.h"
#include "ocf/VertexBuffer.h"
#include "ocf/IndexBuffer.h"
#include "ocf/OpenGLUtility.h"
#include <glad/gl.h>
#include <assert.h>
#include <iostream>

namespace ocf {

struct VertexArrayObject::BuilderDetails {
	PrimitiveType primitiveType = PrimitiveType::TRIANGLES;
	VertexBuffer* vertexBuffer = nullptr;
	IndexBuffer* indexBuffer = nullptr;
	size_t offset = 0;
	size_t count = 0;
};

VertexArrayObject::Builder::Builder()
{
    m_impl = new BuilderDetails();
}

VertexArrayObject::Builder::~Builder()
{
    delete m_impl;
}

VertexArrayObject::Builder& VertexArrayObject::Builder::geometry(PrimitiveType type,
                                                                 VertexBuffer* vb, IndexBuffer* ib,
                                                                 size_t offset, size_t count)
{
    assert(vb != nullptr);

    m_impl->primitiveType = type;
    m_impl->vertexBuffer = vb;
    m_impl->indexBuffer = ib;
    m_impl->offset = offset;
    m_impl->count = count;
    return *this;
}

VertexArrayObject* VertexArrayObject::Builder::build()
{
    VertexArrayObject* vao = new VertexArrayObject(*this);
    return vao;
}

VertexArrayObject::VertexArrayObject(const Builder& builder)
    : m_primitiveType(builder->primitiveType)
    , m_vertexBuffer(builder->vertexBuffer)
    , m_indexBuffer(builder->indexBuffer)
    , m_offset(builder->offset)
    , m_count(builder->count)
{
    glGenVertexArrays(1, &m_handle);
    glBindVertexArray(m_handle);
    if (m_indexBuffer != nullptr) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer->getHandle());
    }

    CHECK_GL_ERROR(std::cerr);
}

VertexArrayObject::~VertexArrayObject()
{
     terminate();
}

void VertexArrayObject::terminate()
{
    glDeleteVertexArrays(1, &m_handle);
    m_handle = 0;

    delete m_vertexBuffer;
    m_vertexBuffer = nullptr;

    delete m_indexBuffer;
    m_indexBuffer = nullptr;
}

void VertexArrayObject::bind()
{
    glBindVertexArray(m_handle);
    update();
}

void VertexArrayObject::unbind()
{
    glBindVertexArray(0);
}

unsigned int VertexArrayObject::getPrimitiveType() const noexcept
{
    switch (m_primitiveType) {
    case PrimitiveType::POINTS:
    return GL_POINTS;
    case PrimitiveType::LINES:
        return GL_LINES;
    case PrimitiveType::LINE_STRIP:
        return GL_LINE_STRIP;
    case PrimitiveType::TRIANGLES:
        return GL_TRIANGLES;
    case PrimitiveType::TRIANGLE_STRIP:
        return GL_TRIANGLE_STRIP;
    }
    return 0;
}

GLenum VertexArrayObject::getIndexType() const noexcept
{
    if (m_indexBuffer != nullptr) {
        if (m_indexBuffer->getIndexType() == IndexBuffer::IndexType::USHORT) {
            return GL_UNSIGNED_SHORT;
        } else if (m_indexBuffer->getIndexType() == IndexBuffer::IndexType::UINT) {
            return GL_UNSIGNED_INT;
        }
    }
    return 0;
}

void VertexArrayObject::update()
{
    assert(m_vertexBuffer != nullptr);

    if (m_vertexBufferVersion == m_vertexBuffer->getVersion()) {
        return;
    }

    for (size_t i = 0, n = m_vertexBuffer->m_attributes.size(); i < n; i++) {
        const auto& attribute = m_vertexBuffer->m_attributes[i];
        const uint8_t buffer = attribute.buffer;
        if (buffer != Attribute::BUFFER_UNUSED) {
            glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer->getHandle());
            const GLuint index = static_cast<GLuint>(i);
            const GLint size = static_cast<GLint>(OpenGLUtility::getComponentCount(attribute.type));
            const GLenum type = OpenGLUtility::getComponentType(attribute.type);
            const GLsizei stride = attribute.stride;
            const void* pointer = reinterpret_cast<void*>(static_cast<uintptr_t>(attribute.offset));

            glVertexAttribPointer(index, size, type, GL_FALSE, stride, pointer);
            glEnableVertexAttribArray(index);
        }
    }

    m_vertexBufferVersion = m_vertexBuffer->getVersion();
}

} // namespace ocf
