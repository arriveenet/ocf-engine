// SPDX-License-Identifier: MIT
#pragma once
#include "ocf/core/event/Event.h"
#include "ocf/core/input/KeyCode.h"

namespace ocf {

class KeyboardEvent : public Event {
public:
    KeyboardEvent(KeyCode key, bool isPressed)
        : Event(EventType::KeyboardEvent)
        , m_keyCode(key)
        , m_isPressed(isPressed)
    {
    }

    KeyCode getKeyCode() const noexcept { return m_keyCode; }

    bool isPressed() const noexcept { return m_isPressed; }

private:
    KeyCode m_keyCode;
    bool m_isPressed;
};

} // namespace ocf