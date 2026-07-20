#include "SweepUnit.h"

void SweepUnit::clock()
{
    if (!m_enabled || m_shiftCount == 0) {
        return;
    }

    // Calculate delta
    uint16_t delta = m_timerReload >> m_shiftCount;
    m_targetTimer = m_timerReload;
    if (m_negateFlag) {
        // Subtract
        m_targetTimer -= delta;
    }
    else {
        // Add
        m_targetTimer += delta;
    }

    m_isMuted = (m_timerReload < 8 || m_targetTimer > 0x7FF);

    if (!m_isMuted) {
        m_timerReload = m_targetTimer;
    }

    // Update divider
    if (m_divider == 0) {
        m_divider = m_dividerPeriod;
    }
    else {
        m_divider--;
    }
}

void SweepUnit::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

void SweepUnit::setNegateFlag(bool negate)
{
    m_negateFlag = negate;
}

void SweepUnit::setDividerPeriod(uint8_t period)
{
    m_dividerPeriod = period;
}

void SweepUnit::setShiftCount(uint8_t count)
{
    m_shiftCount = count;
}

void SweepUnit::setTimer(uint16_t timer)
{
    m_timerReload = timer;
}

uint16_t SweepUnit::getTargetTimer() const
{
    return m_enabled ? m_targetTimer : m_timerReload;
}
