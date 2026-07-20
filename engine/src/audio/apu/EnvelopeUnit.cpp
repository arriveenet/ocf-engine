#include "EnvelopeUnit.h"
#include "SweepUnit.h"

void EnvelopeUnit::clock()
{
    if (m_startFlag) {
        m_startFlag = false;
        m_decayLevelCounter = 15;
        m_divider = m_dividerPeriod;
    }
    else {
        if (m_divider == 0) {
            m_divider = m_dividerPeriod;
            if (m_decayLevelCounter > 0) {
                m_decayLevelCounter--;
            }
            else if (m_loopFlag) {
                m_decayLevelCounter = 15;
            }
        }
        else {
            m_divider--;
        }
    }
}

void EnvelopeUnit::start()
{
    m_startFlag = true;
}

uint8_t EnvelopeUnit::getOutput()
{
    if (m_constantVolumeFlag) {
        // Constant volume
        return m_dividerPeriod;
    }
    else {
        // Envelope volume
        return m_decayLevelCounter;
    }
}

void EnvelopeUnit::setConstantVolumeFlag(bool flag)
{
    m_constantVolumeFlag = flag;
}

void EnvelopeUnit::setLoop(bool loop)
{
    m_loopFlag = loop;
}

void EnvelopeUnit::setDividerPeriod(uint8_t period)
{
    m_dividerPeriod = period;
}
