// SPDX-License-Identifier: MIT
#include "ocf/core/event/EventDispatcher.h"

#include <algorithm>
#include <utility>

namespace ocf {

namespace detail {

void ListenerRegistry::remove(std::type_index type, uint64_t id)
{
    auto it = listeners.find(type);
    if (it == listeners.end()) {
        return;
    }

    for (auto& listener : it->second) {
        if (listener->id == id) {
            listener->removed = true;
            hasRemoved = true;
            break;
        }
    }

    if (dispatchDepth == 0) {
        eraseRemoved();
    }
}

void ListenerRegistry::eraseRemoved()
{
    if (!hasRemoved) {
        return;
    }

    for (auto& [type, entries] : listeners) {
        std::erase_if(entries, [](const auto& listener) { return listener->removed; });
    }
    hasRemoved = false;
}

} // namespace detail

Subscription::Subscription(std::weak_ptr<detail::ListenerRegistry> registry,
                           std::type_index type, uint64_t id)
    : m_registry(std::move(registry))
    , m_type(type)
    , m_id(id)
{
}

Subscription::~Subscription()
{
    reset();
}

Subscription::Subscription(Subscription&& rhs) noexcept
    : m_registry(std::move(rhs.m_registry))
    , m_type(rhs.m_type)
    , m_id(std::exchange(rhs.m_id, 0))
{
    rhs.m_registry.reset();
}

Subscription& Subscription::operator=(Subscription&& rhs) noexcept
{
    if (this != &rhs) {
        reset();
        m_registry = std::move(rhs.m_registry);
        m_type = rhs.m_type;
        m_id = std::exchange(rhs.m_id, 0);
        rhs.m_registry.reset();
    }
    return *this;
}

void Subscription::reset()
{
    if (m_id != 0) {
        if (auto registry = m_registry.lock()) {
            registry->remove(m_type, m_id);
        }
    }
    m_registry.reset();
    m_id = 0;
}

EventDispatcher::EventDispatcher()
    : m_registry(std::make_shared<detail::ListenerRegistry>())
{
}

EventDispatcher::~EventDispatcher()
{
}

} // namespace ocf
