// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/compiler.h"

namespace ocf {

/**
 * @brief Easing curves for tweens, as in https://easings.net.
 *
 * "In" curves start slowly, "Out" curves end slowly and "InOut" curves do both.
 */
enum class Ease {
    Linear,
    InQuad,
    OutQuad,
    InOutQuad,
    InCubic,
    OutCubic,
    InOutCubic,
    InSine,
    OutSine,
    InOutSine,
    InExpo,
    OutExpo,
    InOutExpo,
    InBack, // Overshoots below 0 at the start
    OutBack, // Overshoots above 1 at the end
    InOutBack,
    InElastic,
    OutElastic,
    InOutElastic,
    InBounce,
    OutBounce,
    InOutBounce,
};

/**
 * @brief Apply an easing curve to a progress.
 *
 * @param t Progress, clamped to [0, 1].
 * @return The eased progress. It is exactly 0 at t = 0 and 1 at t = 1, but Back and Elastic
 *         curves leave [0, 1] in between.
 */
OCF_API float ease(Ease curve, float t);

} // namespace ocf
