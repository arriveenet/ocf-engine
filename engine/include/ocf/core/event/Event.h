// SPDX-License-Identifier: MIT
#pragma once

namespace ocf {

enum class EventType {
    KeyboardEvent,
    MouseEvent,
    WindowResizeEvent,
};

class Event {
public:
    Event(EventType eventType);
    virtual ~Event();

    EventType getEventType() const noexcept { return m_eventType; }

    bool isHandled() const noexcept { return m_handled; }
    void setHandled(bool handled) noexcept { m_handled = handled; }

protected:
    EventType m_eventType;
    bool m_handled = false;
};

} // namespace ocf
