// SPDX-License-Identifier: MIT
#include "LengthCounter.h"

namespace ocf::audio {

static constexpr uint8_t LENGTH_COUNTER_TABLE[32] = {
    0x0A, 0xFE, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06, 0xA0, 0x08, 0x3C, 0x0A, 0x0E, 0x0C, 0x1A, 0x0E,
    0x0C, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16, 0xC0, 0x18, 0x48, 0x1A, 0x10, 0x1C, 0x20, 0x1E,
};

void LengthCounter::clock()
{
    if (m_counter > 0 && !m_halt) {
        m_counter--;
    }
}

void LengthCounter::load(uint8_t index)
{
    if (m_enabled) {
        m_counter = LENGTH_COUNTER_TABLE[index & 0x1F];
    }
}

void LengthCounter::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!enabled) {
        m_counter = 0;
    }
}

} // namespace ocf::audio
