// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/compiler.h"
#include "ocf/core/event/Event.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace ocf {

namespace detail {

struct ListenerRegistry {
    struct Listener {
        uint64_t id = 0;
        std::function<void(Event&)> callback;
        bool removed = false;
    };

    void remove(std::type_index type, uint64_t id);

    void eraseRemoved();

    std::unordered_map<std::type_index, std::vector<std::shared_ptr<Listener>>> listeners;
    uint64_t nextId = 1;
    uint32_t dispatchDepth = 0; // Listeners are only erased while no dispatch iterates them
    bool hasRemoved = false;
};

} // namespace detail

/**
 * @brief Keeps an event listener subscribed for as long as it is alive.
 *
 * Destroying or resetting the subscription unsubscribes the listener. Holding it as a member
 * of the object the listener captures ties the listener to that object's lifetime. A
 * subscription may outlive its dispatcher.
 */
class OCF_API Subscription {
public:
    Subscription() = default;
    ~Subscription();

    Subscription(Subscription&& rhs) noexcept;
    Subscription& operator=(Subscription&& rhs) noexcept;

    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;

    /** @brief Unsubscribe the listener now. */
    void reset();

    bool isActive() const noexcept { return m_id != 0 && !m_registry.expired(); }

private:
    friend class EventDispatcher;

    Subscription(std::weak_ptr<detail::ListenerRegistry> registry, std::type_index type,
                 uint64_t id);

    std::weak_ptr<detail::ListenerRegistry> m_registry;
    std::type_index m_type = std::type_index(typeid(void));
    uint64_t m_id = 0;
};

class OCF_API EventDispatcher {
public:
    using EventCallback = std::function<void(Event&)>;

    EventDispatcher();

    ~EventDispatcher();

    /**
     * @brief Subscribe to events of type T.
     *
     * The listener stays subscribed while the returned Subscription is alive.
     */
    template<typename T>
    [[nodiscard]] Subscription subscribe(std::function<void(T&)> callback) {
        auto type = std::type_index(typeid(T));
        auto listener = std::make_shared<detail::ListenerRegistry::Listener>();
        listener->id = m_registry->nextId++;
        listener->callback = [callback = std::move(callback)](Event& event) {
            callback(static_cast<T&>(event));
        };
        m_registry->listeners[type].push_back(listener);
        return Subscription(m_registry, type, listener->id);
    }

    template <typename T>
    void dispatch(T& event)
    {
        detail::ListenerRegistry& registry = *m_registry;
        auto it = registry.listeners.find(std::type_index(typeid(T)));
        if (it == registry.listeners.end()) {
            return;
        }

        // Listeners may subscribe or unsubscribe from their callback. Entries are not erased
        // during the dispatch, and listeners added by it are called from the next one.
        auto& listeners = it->second;
        const size_t count = listeners.size();

        registry.dispatchDepth++;
        for (size_t i = 0; i < count; i++) {
            // Hold the listener, since its callback may unsubscribe it
            std::shared_ptr<detail::ListenerRegistry::Listener> listener = listeners[i];
            if (listener->removed) {
                continue;
            }

            listener->callback(event);

            if (event.isHandled()) {
                break;
            }
        }
        registry.dispatchDepth--;

        if (registry.dispatchDepth == 0) {
            registry.eraseRemoved();
        }
    }

private:
    std::shared_ptr<detail::ListenerRegistry> m_registry;
};

} // namespace ocf
