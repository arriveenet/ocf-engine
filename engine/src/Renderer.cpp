#include "ocf/FileUtils.h"
#include "ocf/IndexBuffer.h"
#include "ocf/Program.h"
#include "ocf/Renderer.h"
#include "ocf/Types.h"
#include "ocf/VertexArrayObject.h"
#include "ocf/VertexBuffer.h"
#include <glad/gl.h>

namespace ocf {

static Vertex2fC4f vertices[4] = {{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                           {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
                           {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
                           {{0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}}};

static uint16_t indices[6] = {0, 1, 2, 2, 1, 3};

Renderer::Renderer()
{
    const auto vertPath = FileUtils::getInstance()->fullPathForFilename("shaders/basic.vert");
    const auto fragPath = FileUtils::getInstance()->fullPathForFilename("shaders/basic.frag");
    m_program = Program::Builder()
        .vertexShader(vertPath)
        .fragmentShader(fragPath)
        .build();

    VertexBuffer* vb = VertexBuffer::Builder()
        .bufferCount(1).vertexCount(4)
        .attribute(VertexAttribute::POSITION, ElementType::FLOAT2, sizeof(Vertex2fC4f), offsetof(Vertex2fC4f, position))
        .attribute(VertexAttribute::COLOR, ElementType::FLOAT4, sizeof(Vertex2fC4f), offsetof(Vertex2fC4f, color))
        .build();
    vb->setBufferData(vertices, sizeof(vertices), 0);
    
    IndexBuffer* ib = IndexBuffer::Builder()
        .indexCount(6)
        .build();
    ib->setBufferData(indices, sizeof(indices), 0);

    m_vertexArrayObject = VertexArrayObject::Builder()
        .geometry(PrimitiveType::TRIANGLES, vb, ib, 0, 6)
        .build();
}

Renderer::~Renderer()
{
    delete m_program;
    delete m_vertexArrayObject;
}

void Renderer::render()
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_program->getHandle());

    m_vertexArrayObject->bind();

    const GLenum mode = m_vertexArrayObject->getPrimitiveType();
    const size_t count = m_vertexArrayObject->getVertexCount();
    const size_t offset = m_vertexArrayObject->offset();

    if (m_vertexArrayObject->hasIndexBuffer()) {
        const GLenum indexType = m_vertexArrayObject->getIndexType();
        glDrawElements(mode, static_cast<GLint>(count), indexType,
                       reinterpret_cast<const void*>(static_cast<uintptr_t>(offset)));
    }
    else {
        glDrawArrays(mode, static_cast<GLint>(offset), static_cast<GLsizei>(count));
    }
}

} // namespace ocf
