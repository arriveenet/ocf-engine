// SPDX-License-Identifier: MIT
#pragma once
#include "ocf/core/event/Event.h"
#include "ocf/core/input/Mouse.h"
#include "ocf/math/vec2.h"

namespace ocf {

class MouseEvent : public Event {
public:
    enum class MouseEventType {
        None,
        Down,
        Up,
        Move,
        Scroll
    };

    MouseEvent(MouseEventType mouseEventType)
        : Event(EventType::MouseEvent)
        , m_mouseEventType(mouseEventType)
    {
    }

    void setMouseButton(MouseButton button) { m_mouseButton = button; }
    MouseButton getMouseButton() const { return m_mouseButton; }

    void setPosition(const math::vec2& position) { m_position = position; }
    void setLastPosition(const math::vec2& position) { m_lastPosition = position; }
    math::vec2 getLastPosition() const { return m_lastPosition; }
    math::vec2 getPosition() const { return m_position; }
    math::vec2 getDelta() const { return m_position - m_lastPosition; }

    void setScrollDelta(const math::vec2& scrollDelta) { m_scrollDelta = scrollDelta; }
    math::vec2 getScrollDelta() const { return m_scrollDelta; }

protected:
    MouseEventType m_mouseEventType;
    MouseButton m_mouseButton = MouseButton::ButtonMax;
    math::vec2 m_position = math::vec2(0.0f, 0.0f);
    math::vec2 m_lastPosition = math::vec2(0.0f, 0.0f);
    math::vec2 m_scrollDelta = math::vec2(0.0f, 0.0f);
};

} // namespace ocf