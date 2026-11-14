// SPDX-License-Identifier: MIT
#include "ocf/scene/animation/Easing.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace ocf {

namespace {

constexpr float PI = std::numbers::pi_v<float>;
constexpr float BACK_C1 = 1.70158f;
constexpr float BACK_C2 = BACK_C1 * 1.525f;
constexpr float BACK_C3 = BACK_C1 + 1.0f;
constexpr float ELASTIC_C4 = (2.0f * PI) / 3.0f;
constexpr float ELASTIC_C5 = (2.0f * PI) / 4.5f;

float outBounce(float t)
{
    constexpr float n1 = 7.5625f;
    constexpr float d1 = 2.75f;
    if (t < 1.0f / d1) {
        return n1 * t * t;
    }
    if (t < 2.0f / d1) {
        t -= 1.5f / d1;
        return n1 * t * t + 0.75f;
    }
    if (t < 2.5f / d1) {
        t -= 2.25f / d1;
        return n1 * t * t + 0.9375f;
    }
    t -= 2.625f / d1;
    return n1 * t * t + 0.984375f;
}

float evaluate(Ease curve, float t)
{
    switch (curve) {
    case Ease::Linear:
        return t;
    case Ease::InQuad:
        return t * t;
    case Ease::OutQuad:
        return 1.0f - (1.0f - t) * (1.0f - t);
    case Ease::InOutQuad:
        return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) * 0.5f;
    case Ease::InCubic:
        return t * t * t;
    case Ease::OutCubic:
        return 1.0f - std::pow(1.0f - t, 3.0f);
    case Ease::InOutCubic:
        return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f;
    case Ease::InSine:
        return 1.0f - std::cos(t * PI * 0.5f);
    case Ease::OutSine:
        return std::sin(t * PI * 0.5f);
    case Ease::InOutSine:
        return -(std::cos(PI * t) - 1.0f) * 0.5f;
    case Ease::InExpo:
        return std::pow(2.0f, 10.0f * t - 10.0f);
    case Ease::OutExpo:
        return 1.0f - std::pow(2.0f, -10.0f * t);
    case Ease::InOutExpo:
        return t < 0.5f ? std::pow(2.0f, 20.0f * t - 10.0f) * 0.5f
                        : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) * 0.5f;
    case Ease::InBack:
        return BACK_C3 * t * t * t - BACK_C1 * t * t;
    case Ease::OutBack:
        return 1.0f + BACK_C3 * std::pow(t - 1.0f, 3.0f) + BACK_C1 * std::pow(t - 1.0f, 2.0f);
    case Ease::InOutBack:
        return t < 0.5f
                   ? (std::pow(2.0f * t, 2.0f) * ((BACK_C2 + 1.0f) * 2.0f * t - BACK_C2)) * 0.5f
                   : (std::pow(2.0f * t - 2.0f, 2.0f) *
                          ((BACK_C2 + 1.0f) * (t * 2.0f - 2.0f) + BACK_C2) +
                      2.0f) *
                         0.5f;
    case Ease::InElastic:
        return -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * ELASTIC_C4);
    case Ease::OutElastic:
        return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * ELASTIC_C4) + 1.0f;
    case Ease::InOutElastic:
        return t < 0.5f ? -(std::pow(2.0f, 20.0f * t - 10.0f) *
                            std::sin((20.0f * t - 11.125f) * ELASTIC_C5)) *
                              0.5f
                        : (std::pow(2.0f, -20.0f * t + 10.0f) *
                           std::sin((20.0f * t - 11.125f) * ELASTIC_C5)) *
                                  0.5f +
                              1.0f;
    case Ease::InBounce:
        return 1.0f - outBounce(1.0f - t);
    case Ease::OutBounce:
        return outBounce(t);
    case Ease::InOutBounce:
        return t < 0.5f ? (1.0f - outBounce(1.0f - 2.0f * t)) * 0.5f
                        : (1.0f + outBounce(2.0f * t - 1.0f)) * 0.5f;
    }
    return t;
}

} // namespace

float ease(Ease curve, float t)
{
    // The ends are exact so that a finished tween lands on its target value
    if (!(t > 0.0f)) {
        return 0.0f;
    }
    if (t >= 1.0f) {
        return 1.0f;
    }
    return evaluate(curve, std::clamp(t, 0.0f, 1.0f));
}

} // namespace ocf
