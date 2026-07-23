// SPDX-License-Identifier: MIT
#include "ocf/core/event/Event.h"

namespace ocf {

Event::Event(EventType eventType)
    : m_eventType(eventType)
{
}

Event::~Event()
{
}

} // namespace ocf
