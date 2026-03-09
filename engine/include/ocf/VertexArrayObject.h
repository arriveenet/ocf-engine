#pragma once
#include "ocf/RendererEnums.h"

namespace ocf {

class VertexBuffer;
class IndexBuffer;

class VertexArrayObject {
    struct BuilderDetails;

public:
    class Builder {
    public:
        Builder();
        ~Builder();

        BuilderDetails* operator->() noexcept { return m_impl; }
        const BuilderDetails* operator->() const noexcept { return m_impl; }

        Builder& geometry(PrimitiveType type, VertexBuffer* vb, IndexBuffer* ib, size_t offset,
                          size_t count);

        VertexArrayObject* build();

    private:
        BuilderDetails* m_impl = nullptr;
    };

    VertexArrayObject(const Builder& builder);
    ~VertexArrayObject();

    void terminate();

    void bind();

    void unbind();

    bool hasIndexBuffer() const noexcept { return m_indexBuffer != nullptr; }

    unsigned int getPrimitiveType() const noexcept;

    size_t getVertexCount() const noexcept { return m_count; }
   
    size_t getVertexOffset() const noexcept { return m_offset; }

    size_t offset() const noexcept { return m_offset; }

    unsigned int getIndexType() const noexcept;

protected:
    void update();

protected:
    unsigned int m_handle = 0;
    PrimitiveType m_primitiveType = PrimitiveType::TRIANGLES;
    size_t m_offset = 0;
    size_t m_count = 0;
    VertexBuffer* m_vertexBuffer = nullptr;
    IndexBuffer* m_indexBuffer = nullptr;
    uint8_t m_vertexBufferVersion = 0;
};

} // namespace ocf
