// SPDX-License-Identifier: MIT
#include "ocf/core/Reference.h"

namespace ocf {

RefCounted::RefCounted()
{
}

RefCounted::~RefCounted()
{
}

bool RefCounted::retain()
{
    m_referenceCount.fetch_add(1, std::memory_order_relaxed);
    return true;
}

bool RefCounted::release()
{
    if (m_referenceCount.fetch_sub(1, std::memory_order_release) == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        delete this;
    }
    return true;
}

} // namespace ocf

