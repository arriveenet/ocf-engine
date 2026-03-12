#include "ocf/FileUtils.h"
#include "ocf/IndexBuffer.h"
#include "ocf/Program.h"
#include "ocf/Sprite.h"
#include "ocf/Types.h"
#include "ocf/VertexArrayObject.h"
#include "ocf/VertexBuffer.h"

namespace ocf {

static Vertex2fC4f vertices[4] = {{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                                  {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
                                  {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
                                  {{0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}}};

static uint16_t indices[6] = {0, 1, 2, 2, 1, 3};

Sprite::Sprite()
{
    const auto vertPath = FileUtils::getInstance()->fullPathForFilename("shaders/basic.vert");
    const auto fragPath = FileUtils::getInstance()->fullPathForFilename("shaders/basic.frag");
    m_program = Program::Builder().vertexShader(vertPath).fragmentShader(fragPath).build();

    VertexBuffer* vb = VertexBuffer::Builder()
                           .bufferCount(1)
                           .vertexCount(4)
                           .attribute(VertexAttribute::POSITION, ElementType::FLOAT2,
                                      sizeof(Vertex2fC4f), offsetof(Vertex2fC4f, position))
                           .attribute(VertexAttribute::COLOR, ElementType::FLOAT4,
                                      sizeof(Vertex2fC4f), offsetof(Vertex2fC4f, color))
                           .build();
    vb->setBufferData(vertices, sizeof(vertices), 0);

    IndexBuffer* ib = IndexBuffer::Builder().indexCount(6).build();
    ib->setBufferData(indices, sizeof(indices), 0);

    m_vertexArrayObject =
        VertexArrayObject::Builder().geometry(PrimitiveType::TRIANGLES, vb, ib, 0, 6).build();
}

Sprite::~Sprite()
{
}

void Sprite::update(float deltaTime)
{
}

} // namespace ocf
