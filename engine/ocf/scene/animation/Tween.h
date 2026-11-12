// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/compiler.h"
#include "ocf/math/vec3.h"
#include "ocf/math/vec4.h"
#include "ocf/scene/animation/Easing.h"

#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace ocf {

class Drawable2D;
class Node;
class Tween;

using TweenPtr = std::unique_ptr<Tween>;

/**
 * @brief A change of values over time, run by a Tweener on the node it is attached to.
 *
 * Tweens are made with the factory functions and composed with sequence(), parallel() and
 * repeat(). Each tween reads its start values when it begins, not when it is made, so moveBy()
 * after another move continues from where that move ended.
 *
 * A tween keeps its own progress, so one instance can run only once at a time. Make a new one
 * to run the same change on another node.
 */
class OCF_API Tween {
public:
    virtual ~Tween();

    /** @brief Begin, or begin again, on the node. Reads the start values. */
    virtual void start(Node& node) = 0;

    /**
     * @brief Advance the tween. Must be called after start().
     *
     * @return The part of deltaTime that was not needed because the tween finished, so that
     *         the next tween in a sequence can use it. 0 while the tween is running.
     */
    virtual float update(Node& node, float deltaTime) = 0;

    virtual bool isFinished() const noexcept = 0;

    // ---- Node transform ----------------------------------------------------------------------

    /** @brief Move the node to a position in its parent's space. */
    static TweenPtr moveTo(const math::vec3& position, float duration, Ease curve = Ease::Linear);
    /** @brief Move the node by an offset from where it is when the tween begins. */
    static TweenPtr moveBy(const math::vec3& offset, float duration, Ease curve = Ease::Linear);

    /** @brief Rotate the node to Euler angles in degrees, as in Node::setRotation(). */
    static TweenPtr rotateTo(const math::vec3& degrees, float duration, Ease curve = Ease::Linear);
    /** @brief Rotate the node by Euler angles in degrees. Can turn more than once around. */
    static TweenPtr rotateBy(const math::vec3& degrees, float duration, Ease curve = Ease::Linear);

    static TweenPtr scaleTo(const math::vec3& scale, float duration, Ease curve = Ease::Linear);
    /** @brief Multiply the scale of the node by factors, per axis. */
    static TweenPtr scaleBy(const math::vec3& factors, float duration, Ease curve = Ease::Linear);

    // ---- Drawable2D color --------------------------------------------------------------------

    /**
     * @brief Change the color of a 2D component, usually one on the same node.
     *
     * The target is not owned and must outlive the tween.
     */
    static TweenPtr colorTo(Drawable2D* target, const math::vec4& color, float duration,
                            Ease curve = Ease::Linear);

    /** @brief Change only the alpha of the color of a 2D component. */
    static TweenPtr fadeTo(Drawable2D* target, float alpha, float duration,
                           Ease curve = Ease::Linear);

    // ---- Anything else -----------------------------------------------------------------------

    /**
     * @brief Call a function with the eased progress every update.
     *
     * The progress goes from 0 to 1; apply() is called with 1 when the tween finishes. Use it
     * to tween values that have no factory, e.g. lerp between two values captured in the lambda.
     */
    static TweenPtr custom(std::function<void(float progress)> apply, float duration,
                           Ease curve = Ease::Linear);

    /** @brief Wait. Useful in a sequence. */
    static TweenPtr delay(float duration);

    /** @brief Call a function once and finish at once. Useful in a sequence. */
    static TweenPtr call(std::function<void()> function);

    // ---- Composition -------------------------------------------------------------------------

    /** @brief Run the tweens one after another. */
    static TweenPtr sequence(std::vector<TweenPtr> tweens);

    template<typename... Tweens>
    static TweenPtr sequence(TweenPtr first, Tweens&&... rest)
    {
        return sequence(makeList(std::move(first), std::forward<Tweens>(rest)...));
    }

    /** @brief Run the tweens at the same time. Finishes when the longest one finishes. */
    static TweenPtr parallel(std::vector<TweenPtr> tweens);

    template<typename... Tweens>
    static TweenPtr parallel(TweenPtr first, Tweens&&... rest)
    {
        return parallel(makeList(std::move(first), std::forward<Tweens>(rest)...));
    }

    /**
     * @brief Run a tween again and again.
     *
     * Each run begins anew and reads the start values again, so a repeated moveBy() keeps
     * moving. To go back and forth, repeat a sequence of moveBy(offset) and moveBy(-offset).
     *
     * @param count Number of runs; 0 or less repeats forever.
     */
    static TweenPtr repeat(TweenPtr tween, int count = 0);

private:
    template<typename... Tweens>
    static std::vector<TweenPtr> makeList(Tweens&&... tweens)
    {
        std::vector<TweenPtr> list;
        list.reserve(sizeof...(tweens));
        (list.push_back(std::forward<Tweens>(tweens)), ...);
        return list;
    }
};

} // namespace ocf
