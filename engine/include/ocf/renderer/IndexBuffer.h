// SPDX-License-Identifier: MIT
#pragma once
#include "ocf/rhi/Handle.h"
#include "ocf/rhi/RHIEnums.h"
#include <stdint.h>

namespace ocf {

class Engine;

class IndexBuffer {
    struct BuilderDetails;

public:
    using IndexBufferHandle = rhi::IndexBufferHandle;
    using BufferUsage = rhi::BufferUsage;
    using ElementType = rhi::ElementType;

    enum class IndexType : uint8_t {
        Ushort = uint8_t(ElementType::Ushort),
        Uint = uint8_t(ElementType::Uint)
    };

    class Builder {
    public:
        Builder();
        ~Builder();

        BuilderDetails* operator->() noexcept
        {
            return m_impl;
        }
        const BuilderDetails* operator->() const noexcept
        {
            return m_impl;
        }

        Builder& indexType(IndexType type) noexcept;

        Builder& indexCount(uint32_t indexCount) noexcept;

        IndexBuffer* build(Engine& engine);

    private:
        BuilderDetails* m_impl = nullptr;
    };

    IndexBuffer(Engine& engine, const Builder& builder);
    ~IndexBuffer();

    void terminate(Engine& engine);

    void setBufferData(Engine& engine, const void* data, size_t size, size_t offset) noexcept;

    IndexBufferHandle getHandle() const noexcept
    {
        return m_handle;
    }

    uint32_t getIndexCount() const noexcept
    {
        return m_indexCount;
    }

    IndexType getIndexType() const noexcept
    {
        return m_indexType;
    }

protected:
    IndexBufferHandle m_handle;
    uint32_t m_indexCount = 0;
    IndexType m_indexType = IndexType::Ushort;
};

} // namespace ocf
