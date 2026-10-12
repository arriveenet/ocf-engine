#include "SweepUnit.h"

namespace ocf::audio {

static constexpr uint16_t MAX_PERIOD = 0x7FF;
static constexpr uint16_t MIN_PERIOD = 8;

SweepUnit::SweepUnit(bool onesComplement)
    : m_onesComplement(onesComplement)
{
}

void SweepUnit::clock()
{
    // Update the period
    if (m_divider == 0 && m_enabled && m_shiftCount != 0 && !isMuted()) {
        m_period = getTargetPeriod();
    }

    // Update divider
    if (m_divider == 0 || m_reloadFlag) {
        m_divider = m_dividerPeriod;
        m_reloadFlag = false;
    }
    else {
        m_divider--;
    }
}

void SweepUnit::setRegister(uint8_t data)
{
    m_enabled = (data & 0x80) != 0;
    m_dividerPeriod = (data & 0x70) >> 4;
    m_negateFlag = (data & 0x08) != 0;
    m_shiftCount = data & 0x07;
    m_reloadFlag = true;
}

uint16_t SweepUnit::getTargetPeriod() const
{
    const int change = m_period >> m_shiftCount;
    if (!m_negateFlag) {
        return static_cast<uint16_t>(m_period + change);
    }

    const int target = m_period - change - (m_onesComplement ? 1 : 0);
    return static_cast<uint16_t>(target < 0 ? 0 : target);
}

bool SweepUnit::isMuted() const
{
    return m_period < MIN_PERIOD || getTargetPeriod() > MAX_PERIOD;
}

} // namespace ocf::audio
