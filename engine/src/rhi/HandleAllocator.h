// SPDX-License-Identifier: MIT

#pragma once

#include "ocf/core/Allocator.h"
#include "ocf/rhi/Handle.h"

#include <unordered_map>

namespace ocf::rhi {

template <size_t P0, size_t P1, size_t P2>
class HandleAllocator {
public:
    HandleAllocator(const char* name, size_t size);
    ~HandleAllocator();

    template <typename D, typename... Args>
    Handle<D> allocateAndConstruct(Args&&... args)
    {
        Handle<D> handle{allocateHandle<D>()};
        D* addr = handle_cast<D*>(handle);
        new (addr) D(std::forward<Args>(args)...);
        return handle;
    }

    template<typename D>
    Handle<D> allocate() noexcept
    {
        Handle<D> handle{allocateHandle<D>()};
        return handle;
    }

    template <typename D, typename B, typename... ARGS>
    std::enable_if_t<std::is_base_of_v<B, D>, D>*
    destroyAndConstruct(Handle<B> const& handle, ARGS&&... args)
    {
        D* addr = handle_cast<D*>(const_cast<Handle<B>&>(handle));
        addr->~D();
        new (addr) D(std::forward<ARGS>(args)...);
        return addr;
    }

    template <typename B, typename D,
              typename = std::enable_if_t<std::is_base_of_v<B, D>, D>>
    void deallocate(Handle<B>& handle, D const* p) noexcept
    {
        if (p) {
            p->~D();
            deallocateHandle<D>(handle.getId());
        }
    }

    template<typename Dp, typename B>
    inline std::enable_if_t<
        std::is_pointer_v<Dp> &&
        std::is_base_of_v<B, std::remove_pointer_t<Dp>>, Dp>
    handle_cast(Handle<B>& handle) const noexcept
    {
        auto [p, tag] = handleToPointer(handle.getId());

       return static_cast<Dp>(p);
    }

    template <typename Dp, typename B>
    inline std::enable_if_t<
        std::is_pointer_v<Dp> &&
        std::is_base_of_v<B, std::remove_pointer_t<Dp>>, Dp>
    handle_cast(const Handle<B>& handle) const noexcept
    {
        return handle_cast<Dp>(const_cast<Handle<B>&>(handle));
    }

private:

    template <typename D> static constexpr size_t getBucketSize() noexcept
    {
        if constexpr (sizeof(D) <= P0) return P0;
        if constexpr (sizeof(D) <= P1) return P1;
        static_assert(sizeof(D) <= P2, "Handle type is too large");
        return P2;
    }

    class Allocator {
        friend class HandleAllocator;
        static constexpr size_t MIN_ALIGNMENT = alignof(std::max_align_t);
        struct Node {
            uint8_t age;
        };
        template <size_t SIZE>
        using Pool = MemoryPool<SIZE, MIN_ALIGNMENT, sizeof(Node)>;
        Pool<P0> m_pool0;
        Pool<P1> m_pool1;
        Pool<P2> m_pool2;
        const AreaPolicy::HeapArea& m_area;

    public:
        explicit Allocator(const AreaPolicy::HeapArea& area);

        static constexpr size_t getAliment() noexcept { return MIN_ALIGNMENT; }

        void* alloc(size_t size, size_t, size_t, uint8_t* outAge) noexcept
        {
            void* p = nullptr;
            if      (size <= m_pool0.getSize()) p = m_pool0.alloc(size);
            else if (size <= m_pool1.getSize()) p = m_pool1.alloc(size);
            else if (size <= m_pool2.getSize()) p = m_pool2.alloc(size);

            if (p != nullptr) {
                const Node* const pNode = static_cast<Node const*>(p);
                *outAge = pNode[-1].age;
            }
            return p;
        }

        void free(void* p, size_t size, uint8_t) noexcept
        {
            Node* const pNode = static_cast<Node*>(p);
            uint8_t& expectedAge = pNode[-1].age;
            expectedAge = (expectedAge + 1) & 0xF; // fixme

            if (size <= m_pool0.getSize()) { m_pool0.free(p); return; }
            if (size <= m_pool1.getSize()) { m_pool1.free(p); return; }
            if (size <= m_pool2.getSize()) { m_pool2.free(p); return; }

        }
    };

    using HandleLinerAllocator = LinearAllocator<Allocator>;

    template <typename D>
    HandleBase::HandleId allocateHandle() noexcept
    {
        constexpr size_t BUCKET_SIZE = getBucketSize<D>();
        return allocateHandleInPool<BUCKET_SIZE>();
    }

    template <typename D> void deallocateHandle(HandleBase::HandleId id) noexcept
    {
        constexpr size_t BUCKET_SIZE = getBucketSize<D>();
        deallocateHandleInPool<BUCKET_SIZE>(id);
    }

    template <size_t SIZE>
    HandleBase::HandleId allocateHandleInPool() noexcept
    {
        uint8_t age;
        void* p = m_allocator.alloc(SIZE, alignof(std::max_align_t), 0, &age);
        if (p != nullptr) {
            return linearPoolPointerToHandle(p, age);
        }

        return allocateHandleMap(SIZE);
    }

    template <size_t SIZE>
    void deallocateHandleInPool(HandleBase::HandleId id) noexcept
    {
        if (isPoolHandle(id)) {
            auto [p, tag] = handleToPointer(id);
            uint8_t age = (tag & HANDLE_AGE_MASK) >> HANDLE_AGE_SHIFT;
            m_allocator.free(p, SIZE, age);
        } else {
            deallocateHandleMap(id, SIZE);
        }
    }

    std::pair<void*, uint32_t> handleToPointer(HandleBase::HandleId id) const noexcept
    {
        if (isPoolHandle(id)) {
            char* base = static_cast<char*>(m_allocator.getArea().begin());
            size_t offset = (id & HANDLE_INDEX_MASK) * Allocator::getAliment();
            return { static_cast<void*>(base + offset), 0 };
        }
        return { handleToPointerHandleMap(id), 0 };
    }

    HandleBase::HandleId linearPoolPointerToHandle(void* p, uint32_t tag) const noexcept
    {
        char* base = static_cast<char*>(m_allocator.getArea().begin());
        size_t offset = static_cast<char*>(p) - base;
        auto id = static_cast<HandleBase::HandleId>(offset / Allocator::getAliment());
        id |= tag & HANDLE_AGE_MASK;
        return id;
    }

    HandleBase::HandleId allocateHandleMap(size_t size);
    void deallocateHandleMap(HandleBase::HandleId id, size_t size) noexcept;

    void* handleToPointerHandleMap(HandleBase::HandleId id) const noexcept;

    static constexpr uint32_t HANDLE_AGE_BIT_COUNT = 4u;

    static constexpr uint32_t HANDLE_AGE_SHIFT = 27u;

    static constexpr uint32_t HANDLE_AGE_MASK = ((1u << HANDLE_AGE_BIT_COUNT) - 1u)
                                                << HANDLE_AGE_SHIFT;

    static constexpr uint32_t HANDLE_INDEX_MASK = 0x07FFFFFFu;

    static constexpr uint32_t HANDLE_HEAP_FLAG = 0x80000000u;

    static bool isPoolHandle(HandleBase::HandleId id) noexcept
    {
        return (id & HANDLE_HEAP_FLAG) == 0u;
    }

private:
    HandleLinerAllocator m_allocator;
    std::unordered_map<HandleBase::HandleId, void*> m_handleMap;
    HandleBase::HandleId m_id = 0;
};

using HandleAllocatorGL = HandleAllocator<32, 96, 184>;
using HandleAllocatorVK = HandleAllocator<32, 96, 184>;

} // namespace ocf::rhi
