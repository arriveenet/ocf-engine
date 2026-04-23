// SPDX-License-Identifier: MIT

#pragma once

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <cstddef>
#include <utility>

namespace ocf {

namespace pointermath {

template <typename P, typename T>
static inline P* add(P* a, T b)
{
    return reinterpret_cast<P*>(reinterpret_cast<uintptr_t>(a) + static_cast<uintptr_t>(b));
}

template <typename P>
static inline P* align(P* p, size_t alignment)
{
    // alignment must be a power of two
    assert((alignment & (alignment - 1)) == 0);
    return reinterpret_cast<P*>((reinterpret_cast<uintptr_t>(p) + alignment - 1) &
                                ~(alignment - 1));
}

template <typename P>
static inline P* align(P* p, size_t alignment, size_t offset)
{
    P* const r = align(add(p, offset), alignment);
    assert(r >= add(p, offset));
    return r;
}

} // namespace pointermath

namespace AreaPolicy {

class StaticArea {
public:
    explicit StaticArea(void* begin, void* end)
        : m_begin(begin)
        , m_end(end)
    {
    }

    ~StaticArea() = default;

    void* data() const { return m_begin; }
    void* begin() const { return m_begin; }
    void* end() const { return m_end; }
    size_t size() const { return uintptr_t(m_end) - uintptr_t(m_begin); }

private:
    void* m_begin;
    void* m_end;

};

class HeapArea {
public:
    HeapArea() noexcept = default;

    explicit HeapArea(size_t size)
        : m_begin(nullptr)
        , m_end(nullptr)
    {
        if (size > 0) {
            m_begin = ::malloc(size);
            m_end = pointermath::add(m_begin, size);
        }
    }

    ~HeapArea() { free(m_begin); }

    void* data() const { return m_begin; }
    void* begin() const { return m_begin; }
    void* end() const { return m_end; }
    size_t size() const { return uintptr_t(m_end) - uintptr_t(m_begin); }

private:
    void* m_begin;
    void* m_end;
};

} // namespace AreaPolicy

class FreeList {
public:
    struct Node {
        Node* next;
    };

    FreeList() = default;
    FreeList(void* begin, void* end, size_t elementSize, size_t alignment, size_t offset);
    FreeList(const FreeList& rhs) = delete;
    FreeList& operator=(const FreeList& rhs) = delete;
    FreeList(FreeList&& rhs) noexcept = default;
    FreeList& operator=(FreeList&& rhs) noexcept = default;


    void* pop()
    {
        Node* const head = m_head;
        m_head = head ? head->next : nullptr;
        return head;
    }

    void push(void* p)
    {
        Node* node = static_cast<Node*>(p);
        node->next = m_head;
        m_head = node;
    }

    void* begin() const { return m_begin; }
    void* end() const { return m_end; }

private:
    static Node* init(void* begin, void* end, size_t elementSize, size_t alignment, size_t offset);

    Node* m_head;
    void* m_begin;
    void* m_end;
};

template <size_t ELEMENT_SIZE,
          size_t ALIGNMENT = alignof(std::max_align_t),
          size_t OFFSET = 0 >
class MemoryPool {
public:
    MemoryPool() = default;
    ~MemoryPool() = default;

    // non-copyable
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    // movable
    MemoryPool(MemoryPool&& rhs) = default;
    MemoryPool& operator=(MemoryPool&& rhs) = default;

    MemoryPool(void* begin, void* end)
        : m_freeList(begin, end, ELEMENT_SIZE, ALIGNMENT, OFFSET)
    {
    }

    MemoryPool(void* begin, size_t size)
        : MemoryPool(begin, static_cast<char*>(begin) + size)
    {
    }

    template<typename AREA>
    MemoryPool(const AREA& area)
        : MemoryPool(area.begin(), area.end())
    {
    }

    void* alloc(size_t size = ELEMENT_SIZE, size_t alignment = ALIGNMENT, size_t offset = OFFSET)
    {
        assert(size <= ELEMENT_SIZE);
        assert(alignment <= ALIGNMENT);
        assert(offset == OFFSET);
        return m_freeList.pop();
    }

    void free(void* p, size_t = ELEMENT_SIZE)
    {
        m_freeList.push(p);
    }

    constexpr size_t getSize() const { return ELEMENT_SIZE; }

private:
    FreeList m_freeList;
};

template <typename Allocator,
          typename AreaPolicy = AreaPolicy::HeapArea>
class LinearAllocator {
public:
    template <typename... ARGS>
    LinearAllocator(const char* name, size_t size)
        : m_name(name)
        , m_area(size)
        , m_allocator(m_area, std::forward<ARGS>...)
    {
    }

    void* alloc()
    {
        return m_allocator.alloc();
    }

    void* alloc(size_t size, size_t alignment = alignof(std::max_align_t), size_t offset = 0)
    {
        return m_allocator.alloc(size, alignment, offset);
    }

    template <typename... ARGS>
    void* alloc(size_t size, size_t alignment, size_t offset, ARGS&&... args)
    {
        return m_allocator.alloc(size, alignment, offset, std::forward<ARGS>(args)...);
    }

    void free(void* p)
    {
        if (p) {
            m_allocator.free(p);
        }
    }

    template <typename ... ARGS>
    void free(void* p, size_t size, ARGS&& ... args)
    {
        if (p) {
            m_allocator.free(p, size, std::forward<ARGS>(args)...);
        }
    }

    AreaPolicy& getArea() { return m_area; }
    const AreaPolicy& getArea() const { return m_area; }

    Allocator& getAllocator() { return m_allocator; }
    const Allocator& getAllocator() const { return m_allocator; }

private:
    const char* m_name;
    AreaPolicy m_area;
    Allocator m_allocator;
};

} // namespace ocf
