// SPDX-License-Identifier: MIT
#include "ocf/scene/animation/Tween.h"

#include "ocf/scene/2d/Drawable2D.h"
#include "ocf/scene/Node.h"

#include <algorithm>

namespace ocf {

namespace {

math::vec3 lerp(const math::vec3& from, const math::vec3& to, float t)
{
    return math::vec3(from.x + (to.x - from.x) * t, from.y + (to.y - from.y) * t,
                      from.z + (to.z - from.z) * t);
}

math::vec4 lerp(const math::vec4& from, const math::vec4& to, float t)
{
    return math::vec4(from.x + (to.x - from.x) * t, from.y + (to.y - from.y) * t,
                      from.z + (to.z - from.z) * t, from.w + (to.w - from.w) * t);
}

/** A tween with a duration that applies an eased progress from 0 to 1. */
class IntervalTween : public Tween {
public:
    IntervalTween(float duration, Ease curve)
        : m_duration(std::max(duration, 0.0f))
        , m_curve(curve)
    {
    }

    void start(Node& node) override
    {
        m_elapsed = 0.0f;
        m_finished = false;
        begin(node);
    }

    float update(Node& node, float deltaTime) override
    {
        if (m_finished) {
            return deltaTime;
        }

        m_elapsed += std::max(deltaTime, 0.0f);
        if (m_elapsed < m_duration) {
            apply(node, ease(m_curve, m_elapsed / m_duration));
            return 0.0f;
        }

        apply(node, 1.0f);
        m_finished = true;
        return m_elapsed - m_duration;
    }

    bool isFinished() const noexcept override { return m_finished; }

protected:
    virtual void begin(Node& node) = 0;
    virtual void apply(Node& node, float progress) = 0;

private:
    float m_duration;
    Ease m_curve;
    float m_elapsed = 0.0f;
    bool m_finished = false;
};

enum class TransformProperty { Position, Rotation, Scale };
enum class TransformMode { To, By };

class TransformTween final : public IntervalTween {
public:
    TransformTween(TransformProperty property, TransformMode mode, const math::vec3& value,
                   float duration, Ease curve)
        : IntervalTween(duration, curve)
        , m_property(property)
        , m_mode(mode)
        , m_value(value)
    {
    }

protected:
    void begin(Node& node) override
    {
        m_from = get(node);
        if (m_mode == TransformMode::To) {
            m_to = m_value;
        } else if (m_property == TransformProperty::Scale) {
            m_to = math::vec3(m_from.x * m_value.x, m_from.y * m_value.y, m_from.z * m_value.z);
        } else {
            m_to = m_from + m_value;
        }
    }

    void apply(Node& node, float progress) override
    {
        const math::vec3 value = lerp(m_from, m_to, progress);
        switch (m_property) {
        case TransformProperty::Position:
            node.setPosition(value);
            break;
        case TransformProperty::Rotation:
            node.setRotation(value);
            break;
        case TransformProperty::Scale:
            node.setScale(value);
            break;
        }
    }

private:
    const math::vec3& get(const Node& node) const
    {
        switch (m_property) {
        case TransformProperty::Rotation:
            return node.getRotation();
        case TransformProperty::Scale:
            return node.getScale();
        case TransformProperty::Position:
            break;
        }
        return node.getPosition();
    }

    TransformProperty m_property;
    TransformMode m_mode;
    math::vec3 m_value;
    math::vec3 m_from;
    math::vec3 m_to;
};

class ColorTween final : public IntervalTween {
public:
    ColorTween(Drawable2D* target, const math::vec4& color, bool alphaOnly, float duration,
               Ease curve)
        : IntervalTween(duration, curve)
        , m_target(target)
        , m_color(color)
        , m_alphaOnly(alphaOnly)
    {
    }

protected:
    void begin(Node& /*node*/) override
    {
        if (m_target == nullptr) {
            return;
        }
        m_from = m_target->getColor();
        m_to = m_alphaOnly ? math::vec4(m_from.x, m_from.y, m_from.z, m_color.w) : m_color;
    }

    void apply(Node& /*node*/, float progress) override
    {
        if (m_target != nullptr) {
            m_target->setColor(lerp(m_from, m_to, progress));
        }
    }

private:
    Drawable2D* m_target;
    math::vec4 m_color;
    bool m_alphaOnly;
    math::vec4 m_from;
    math::vec4 m_to;
};

class CustomTween final : public IntervalTween {
public:
    CustomTween(std::function<void(float)> function, float duration, Ease curve)
        : IntervalTween(duration, curve)
        , m_function(std::move(function))
    {
    }

protected:
    void begin(Node& /*node*/) override {}

    void apply(Node& /*node*/, float progress) override
    {
        if (m_function) {
            m_function(progress);
        }
    }

private:
    std::function<void(float)> m_function;
};

class DelayTween final : public IntervalTween {
public:
    explicit DelayTween(float duration)
        : IntervalTween(duration, Ease::Linear)
    {
    }

protected:
    void begin(Node& /*node*/) override {}
    void apply(Node& /*node*/, float /*progress*/) override {}
};

class CallTween final : public Tween {
public:
    explicit CallTween(std::function<void()> function)
        : m_function(std::move(function))
    {
    }

    void start(Node& /*node*/) override { m_finished = false; }

    float update(Node& /*node*/, float deltaTime) override
    {
        if (!m_finished) {
            m_finished = true;
            if (m_function) {
                m_function();
            }
        }
        return deltaTime;
    }

    bool isFinished() const noexcept override { return m_finished; }

private:
    std::function<void()> m_function;
    bool m_finished = false;
};

class SequenceTween final : public Tween {
public:
    explicit SequenceTween(std::vector<TweenPtr> tweens)
        : m_tweens(std::move(tweens))
    {
        std::erase(m_tweens, nullptr);
    }

    void start(Node& node) override
    {
        m_index = 0;
        if (!m_tweens.empty()) {
            m_tweens.front()->start(node);
        }
    }

    float update(Node& node, float deltaTime) override
    {
        // Time left over by a finished tween goes to the next one, so a sequence of short
        // tweens keeps its total duration even with long frames
        while (m_index < m_tweens.size()) {
            deltaTime = m_tweens[m_index]->update(node, deltaTime);
            if (!m_tweens[m_index]->isFinished()) {
                return 0.0f;
            }
            ++m_index;
            if (m_index < m_tweens.size()) {
                m_tweens[m_index]->start(node);
            }
        }
        return deltaTime;
    }

    bool isFinished() const noexcept override { return m_index >= m_tweens.size(); }

private:
    std::vector<TweenPtr> m_tweens;
    size_t m_index = 0;
};

class ParallelTween final : public Tween {
public:
    explicit ParallelTween(std::vector<TweenPtr> tweens)
        : m_tweens(std::move(tweens))
    {
        std::erase(m_tweens, nullptr);
    }

    void start(Node& node) override
    {
        for (const TweenPtr& tween : m_tweens) {
            tween->start(node);
        }
    }

    float update(Node& node, float deltaTime) override
    {
        // The parallel finishes when its longest tween does, so it leaves over what that one
        // left over
        float leftover = deltaTime;
        for (const TweenPtr& tween : m_tweens) {
            if (!tween->isFinished()) {
                leftover = std::min(leftover, tween->update(node, deltaTime));
            }
        }
        return isFinished() ? leftover : 0.0f;
    }

    bool isFinished() const noexcept override
    {
        return std::all_of(m_tweens.begin(), m_tweens.end(),
                           [](const TweenPtr& tween) { return tween->isFinished(); });
    }

private:
    std::vector<TweenPtr> m_tweens;
};

class RepeatTween final : public Tween {
public:
    RepeatTween(TweenPtr tween, int count)
        : m_tween(std::move(tween))
        , m_count(count)
    {
    }

    void start(Node& node) override
    {
        m_runs = 0;
        m_finished = m_tween == nullptr;
        if (m_tween != nullptr) {
            m_tween->start(node);
        }
    }

    float update(Node& node, float deltaTime) override
    {
        if (m_finished) {
            return deltaTime;
        }

        while (true) {
            const float leftover = m_tween->update(node, deltaTime);
            if (!m_tween->isFinished()) {
                return 0.0f;
            }

            ++m_runs;
            if (m_count > 0 && m_runs >= m_count) {
                m_finished = true;
                return leftover;
            }

            m_tween->start(node);
            // A run that takes no time would repeat forever within one update, so the next
            // run waits for the next update
            if (leftover >= deltaTime) {
                return 0.0f;
            }
            deltaTime = leftover;
        }
    }

    bool isFinished() const noexcept override { return m_finished; }

private:
    TweenPtr m_tween;
    int m_count;
    int m_runs = 0;
    bool m_finished = false;
};

} // namespace

Tween::~Tween() = default;

TweenPtr Tween::moveTo(const math::vec3& position, float duration, Ease curve)
{
    return std::make_unique<TransformTween>(TransformProperty::Position, TransformMode::To,
                                            position, duration, curve);
}

TweenPtr Tween::moveBy(const math::vec3& offset, float duration, Ease curve)
{
    return std::make_unique<TransformTween>(TransformProperty::Position, TransformMode::By,
                                            offset, duration, curve);
}

TweenPtr Tween::rotateTo(const math::vec3& degrees, float duration, Ease curve)
{
    return std::make_unique<TransformTween>(TransformProperty::Rotation, TransformMode::To,
                                            degrees, duration, curve);
}

TweenPtr Tween::rotateBy(const math::vec3& degrees, float duration, Ease curve)
{
    return std::make_unique<TransformTween>(TransformProperty::Rotation, TransformMode::By,
                                            degrees, duration, curve);
}

TweenPtr Tween::scaleTo(const math::vec3& scale, float duration, Ease curve)
{
    return std::make_unique<TransformTween>(TransformProperty::Scale, TransformMode::To, scale,
                                            duration, curve);
}

TweenPtr Tween::scaleBy(const math::vec3& factors, float duration, Ease curve)
{
    return std::make_unique<TransformTween>(TransformProperty::Scale, TransformMode::By, factors,
                                            duration, curve);
}

TweenPtr Tween::colorTo(Drawable2D* target, const math::vec4& color, float duration, Ease curve)
{
    return std::make_unique<ColorTween>(target, color, false, duration, curve);
}

TweenPtr Tween::fadeTo(Drawable2D* target, float alpha, float duration, Ease curve)
{
    return std::make_unique<ColorTween>(target, math::vec4(0.0f, 0.0f, 0.0f, alpha), true,
                                        duration, curve);
}

TweenPtr Tween::custom(std::function<void(float progress)> apply, float duration, Ease curve)
{
    return std::make_unique<CustomTween>(std::move(apply), duration, curve);
}

TweenPtr Tween::delay(float duration)
{
    return std::make_unique<DelayTween>(duration);
}

TweenPtr Tween::call(std::function<void()> function)
{
    return std::make_unique<CallTween>(std::move(function));
}

TweenPtr Tween::sequence(std::vector<TweenPtr> tweens)
{
    return std::make_unique<SequenceTween>(std::move(tweens));
}

TweenPtr Tween::parallel(std::vector<TweenPtr> tweens)
{
    return std::make_unique<ParallelTween>(std::move(tweens));
}

TweenPtr Tween::repeat(TweenPtr tween, int count)
{
    return std::make_unique<RepeatTween>(std::move(tween), count);
}

} // namespace ocf
