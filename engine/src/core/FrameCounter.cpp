// SPDX-License-Identifier: MIT
#include "ocf/core/FrameCounter.h"
#include "ocf/core/Logger.h"

namespace ocf {

FrameCounter::FrameCounter()
{
    m_lastUpdate = std::chrono::steady_clock::now();
}

void FrameCounter::update()
{
    m_frameCount++;

    // Calculate delta time
    auto now = std::chrono::steady_clock::now();
    m_deltaTime =
        std::chrono::duration_cast<std::chrono::microseconds>(now - m_lastUpdate).count() /
        1000000.0f;
    m_lastUpdate = now;

    // Update frame count and accumulator
    m_intervalFrameCount++;
    m_accumulator += m_deltaTime;

    // Update frame rate every FPS_UPDATE_INTERVAL seconds
    if (m_accumulator >= FPS_UPDATE_INTERVAL) {
        m_frameRate = m_intervalFrameCount / m_accumulator;
        m_intervalFrameCount = 0;
        m_accumulator = 0.0f;

        //OCF_LOG_INFO("Frame Rate: {:.2f} FPS", m_frameRate);
    }
}

} // namespace ocf
