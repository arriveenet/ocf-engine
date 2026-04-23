// SPDX-License-Identifier: MIT

#include "ocf/core/Allocator.h"

namespace ocf {

FreeList::FreeList(void* begin, void* end, size_t elementSize, size_t alignment, size_t offset)
    : m_head(init(begin, end, elementSize, alignment, offset))
    , m_begin(begin)
    , m_end(end)
{
}

FreeList::Node* FreeList::init(void* begin, void* end, size_t elementSize, size_t alignment,
                               size_t offset)
{
    void* alignedBegin = pointermath::align(begin, alignment, offset);
    void* alignedFirst = pointermath::align(pointermath::add(alignedBegin, elementSize), alignment, offset);
    assert(alignedBegin >= begin && alignedBegin < end);
    assert(alignedFirst >= begin && alignedFirst < end && alignedFirst > alignedBegin);

    size_t stride = uintptr_t(alignedFirst) - uintptr_t(alignedBegin);
    size_t count = (uintptr_t(end) - uintptr_t(alignedBegin)) / stride;

    // first element
    Node* head = static_cast<Node*>(alignedBegin);

    // next elements
    Node* current = head;
    for (size_t i = 1; i < count; i++) {
        Node* next = pointermath::add(current, stride);
        current->next = next;
        current = next;
    }
    assert(current < end);
    assert(pointermath::add(current, stride) <= end);

    // terminate the list
    current->next = nullptr;

    return head;
}

} // namespace ocf
