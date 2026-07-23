// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/event/Event.h"

#include <functional>
#include <unordered_map>
#include <vector>
#include <typeindex>

namespace ocf {

class EventDispatcher {
public:
    using EventCallback = std::function<void(Event&)>;

    EventDispatcher();
    
    ~EventDispatcher();

    template<typename T>
    void subscribe(std::function<void(T&)> callback) {
        auto type = std::type_index(typeid(T));
        m_listeners[type].emplace_back([callback](Event& event) {
            callback(static_cast<T&>(event));
        });
    }

    template <typename T>
    void dispatch(T& event)
    {
        auto type = std::type_index(typeid(T));
        if (m_listeners.find(type) == m_listeners.end()) {
            return;
        }

        for (auto& listener : m_listeners[type]) {
            listener(event);

            if (event.isHandled()) {
                break;
            }
        }
    }

private:
    std::unordered_map<std::type_index, std::vector<EventCallback>> m_listeners;
};

} // namespace ocf
