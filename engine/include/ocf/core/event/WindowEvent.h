// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/event/Event.h"

namespace ocf {

class WindowResizeEvent : public Event {
public:
    WindowResizeEvent(int width, int height)
        : Event(EventType::WindowResizeEvent)
        , m_width(width)
        , m_height(height)
    {
    }

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    int m_width;
    int m_height;
};

} // namespace ocf
