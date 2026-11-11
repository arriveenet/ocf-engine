// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/scene/Component.h"
#include "ocf/scene/animation/Tween.h"

#include <cstdint>
#include <vector>

namespace ocf {

/**
 * @brief Component that runs tweens on the node it is attached to.
 *
 * Several tweens can run at once; they are updated in the order they were run. A tween begins
 * at the next update of the tweener and reads its start values then, so the node can still be
 * set up after run() in the same frame.
 *
 * Callbacks of the tweens (Tween::call(), Tween::custom()) may run and stop tweens on this
 * tweener. As with any component, they must not remove this component or its node.
 */
class OCF_API Tweener : public Component {
public:
    /** @brief Identifies a running tween. 0 is never used. */
    using TweenId = uint32_t;

    Tweener();
    ~Tweener() override;

    void update(float deltaTime) override;

    /** @brief Run a tween. A null tween is ignored and 0 is returned. */
    TweenId run(TweenPtr tween);

    /** @brief Stop a tween where it is. Does nothing if it already finished. */
    void stop(TweenId id);

    void stopAll();

    /** @brief Whether the tween has not finished nor been stopped. */
    bool isRunning(TweenId id) const;

    /** @brief Whether any tween is running. */
    bool isRunning() const;

    /** @brief Pause all tweens. Tweens can still be run and stopped while paused. */
    void setPaused(bool paused) noexcept { m_paused = paused; }
    bool isPaused() const noexcept { return m_paused; }

    /** @brief Multiply the time passed to the tweens. Negative values are treated as 0. */
    void setTimeScale(float timeScale) noexcept { m_timeScale = timeScale > 0.0f ? timeScale : 0.0f; }
    float getTimeScale() const noexcept { return m_timeScale; }

private:
    struct Entry {
        TweenId id = 0;
        TweenPtr tween;
        bool started = false;
        bool stopped = false;
    };

    std::vector<Entry> m_entries;
    TweenId m_nextId = 1;
    float m_timeScale = 1.0f;
    bool m_paused = false;
};

} // namespace ocf
