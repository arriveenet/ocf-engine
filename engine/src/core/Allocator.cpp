// SPDX-License-Identifier: MIT

#include "ocf/core/Allocator.h"

#include <stdlib.h>

namespace ocf::rhi {


FreeList::FreeList(void* begin, void* end, size_t blockSize, size_t alignment)
    : m_begin(begin)
    , m_end(end)
{
}

void FreeList::initialize(void* begin, void* end, size_t blockSize, size_t alignment)
{
}

PoolAllocator::PoolAllocator(size_t size, int count)
{
}

PoolAllocator::~PoolAllocator()
{
}

} // namespace ocf::rhi
