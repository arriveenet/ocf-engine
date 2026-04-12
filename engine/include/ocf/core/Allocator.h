// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>

namespace ocf::rhi {

class FreeList {
public:
    struct Block {
        Block* next;
    };
    
    FreeList(void* begin, void* end, size_t blockSize, size_t alignment);

    void* allocate() {

    }

    void free(void* ptr) {

    }

    void* begin() const { return m_begin; }

    void* end() const { return m_end; }

private:
    static void initialize(void* begin, void* end, size_t blockSize, size_t alignment);

private:
    void* m_begin;
    void* m_end;
};

class PoolAllocator {
public:

    PoolAllocator(size_t size, int count);

    virtual ~PoolAllocator();

    void* allocate()
    {

    }

    void free(void* ptr)
    {

    }

private:
};

} // namespace ocf::rhi
