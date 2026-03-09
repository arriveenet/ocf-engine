#pragma once
#include "ocf/RendererEnums.h"
#include <stdint.h>

namespace ocf {

class VertexBuffer {
    struct BuilderDetails;

public:
    using AttributeType = ElementType;

    class Builder {
    public:
        Builder();
        ~Builder();
        BuilderDetails* operator->() noexcept { return m_impl; }
        const BuilderDetails* operator->() const noexcept { return m_impl; }

        Builder& bufferCount(uint8_t bufferCount) noexcept;
        Builder& vertexCount(uint32_t vertexCount) noexcept;
        Builder& attribute(VertexAttribute attribute, AttributeType type, uint8_t stride,
                           uint32_t offset) noexcept;

        VertexBuffer* build();

    private:
        BuilderDetails* m_impl = nullptr;
    };

    VertexBuffer(const Builder& builder);
    ~VertexBuffer();

    void setBufferData(const void* data, size_t size, size_t offset) noexcept;

    unsigned int getHandle() const noexcept { return m_handle; }

    uint8_t getVersion() const noexcept { return m_version; }

protected:
    unsigned int m_handle = 0;
    uint32_t m_vertexCount = 0;
    size_t m_bufferSize = 0;
    AttributeArray m_attributes;
    uint8_t m_version = 0xff;

private:
    friend class VertexArrayObject;
};

} // namespace ocf
