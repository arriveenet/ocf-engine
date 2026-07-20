#pragma once
#include <stdint.h>

struct SweepUnit
{
    void clock();

    void setEnabled(bool enabled);

    void setNegateFlag(bool negate);

    void setDividerPeriod(uint8_t period);

    void setShiftCount(uint8_t count);

    void setTimer(uint16_t timer);

    bool isMuted() const { return m_isMuted; }

    uint16_t getTargetTimer() const;

private:
    bool m_enabled = false;
    bool m_negateFlag = false;
    bool m_isMuted = false;
    uint8_t m_divider = 0;
    uint8_t m_dividerPeriod = 0;
    uint8_t m_shiftCount = 0;
    uint16_t m_timerReload = 0;
    uint16_t m_targetTimer = 0;
};
