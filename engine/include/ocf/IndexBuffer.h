#pragma once
#include "ocf/RendererEnums.h"
#include <stdint.h>

namespace ocf {

class IndexBuffer {
    struct BuilderDetails;

public:
    enum class IndexType : uint8_t {
        USHORT = uint8_t(ElementType::UNSIGNED_SHORT),
        UINT = uint8_t(ElementType::UNSIGNED_INT)
    };

    class Builder {
    public:
        Builder();
        ~Builder();

        BuilderDetails* operator->() noexcept { return m_impl; }
        const BuilderDetails* operator->() const noexcept { return m_impl; }
        Builder& indexCount(uint32_t indexCount) noexcept;

        IndexBuffer* build();
    private:
        BuilderDetails* m_impl = nullptr;
    };

    IndexBuffer(const Builder& builder);
    ~IndexBuffer();

    void setBufferData(const void* data, size_t size, size_t offset) noexcept;

    unsigned int getHandle() const noexcept { return m_handle; }

    uint32_t getIndexCount() const noexcept { return m_indexCount; }

    IndexType getIndexType() const noexcept { return m_indexType; }

protected:
    unsigned int m_handle = 0;
    IndexType m_indexType = IndexType::USHORT;
    uint32_t m_indexCount = 0;
};

} // namespace ocf
