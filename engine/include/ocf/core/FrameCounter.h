// SPDX-License-Identifier: MIT
#pragma once

#include <chrono>
#include <cstdint>

namespace ocf {

/**
 * @class FrameCounter
 * @brief Utility class for counting frames and calculating frame rate.
 *
 * The FrameCounter class tracks the number of frames rendered and calculates the time elapsed
 * between frames to provide a frame rate measurement. It is useful for performance monitoring
 * and debugging purposes.
 */
class FrameCounter {
public:
    static constexpr float FPS_UPDATE_INTERVAL = 1.0f;

    FrameCounter();
    ~FrameCounter() = default;

    void update();

    uint32_t getFrameCount() const noexcept { return m_frameCount; }

    float getFrameRate() const noexcept { return m_frameRate; }

    float getDeltaTime() const noexcept { return m_deltaTime; }

private:
    uint32_t m_frameCount = 0;    //!< Number of frames counted
    uint32_t m_intervalFrameCount = 0; //!< Number of frames counted in the current interval
    float m_accumulator = 0.0f;   //!< Accumulated time in seconds
    float m_frameRate = 0.0f;     //!< Calculated frame rate in frames per second
    float m_deltaTime = 0.0f;     //!< Time elapsed since last frame in seconds
    std::chrono::steady_clock::time_point m_lastUpdate; ///< Timestamp of the last update
};

} // namespace ocf
