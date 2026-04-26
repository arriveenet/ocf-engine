// SPDX-License-Identifier: MIT
#include "rhi/HandleAllocator.h"
#include <stdlib.h>
#include <string.h>

namespace ocf::rhi {

template <size_t P0, size_t P1, size_t P2>
HandleAllocator<P0, P1, P2>::HandleAllocator(const char* name, size_t size)
    : m_allocator(name, size)
{
}

template <size_t P0, size_t P1, size_t P2>
HandleAllocator<P0, P1, P2>::~HandleAllocator()
{
    for (auto& entry : m_handleMap) {
        ::free(entry.second);
    }
}

template <size_t P0, size_t P1, size_t P2>
HandleBase::HandleId HandleAllocator<P0, P1, P2>::allocateHandleMap(size_t size)
{
    void* p = ::malloc(size);

    HandleBase::HandleId id = ++m_id;

    m_handleMap.emplace(id, p);

    return id;
}

template <size_t P0, size_t P1, size_t P2>
void HandleAllocator<P0, P1, P2>::deallocateHandleMap(HandleBase::HandleId id, size_t) noexcept
{
    void* p = nullptr;

    const auto& iter = m_handleMap.find(id);
    if (iter != m_handleMap.end()) {
        p = iter->second;
        m_handleMap.erase(iter);
    }

    ::free(p);
}

template <size_t P0, size_t P1, size_t P2>
void* HandleAllocator<P0, P1, P2>::handleToPointerHandleMap(HandleBase::HandleId id) const noexcept
{
    const auto& iter = m_handleMap.find(id);
    if (iter != m_handleMap.end()) {
        return iter->second;
    }

    return nullptr;
}

template <size_t P0, size_t P1, size_t P2>
HandleAllocator<P0, P1, P2>::Allocator::Allocator(const AreaPolicy::HeapArea& area)
    : m_area(area)
{
    const size_t heapSizeMax = area.size();

    memset(area.data(), 0, heapSizeMax);

    const size_t count = heapSizeMax / (P0 + P1 + P2);
    char* p0 = static_cast<char*>(area.begin());
    char* p1 = p0 + count * P0;
    char* p2 = p1 + count * P1;

    m_pool0 = Pool<P0>(p0, count * P0);
    m_pool1 = Pool<P1>(p1, count * P1);
    m_pool2 = Pool<P2>(p2, count * P2);
}

template class HandleAllocator<32, 96, 184>;

} // namespace ocf::rhi
