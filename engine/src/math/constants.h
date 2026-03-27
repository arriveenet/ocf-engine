#pragma once

namespace ocf {
namespace math {

    /** return 0 */
    template <typename genType>
    constexpr genType zero();

    /** return 1 */
    template <typename genType>
    constexpr genType one();

    /** return pi */
    template <typename genType>
    constexpr genType pi();

    /** return pi * 2 */
    template <typename genType>
    constexpr genType two_pi();

    /** return square root of pi */
    template <typename genType>
    constexpr genType root_pi();

    /** return pi / 2 */
    template <typename genType>
    constexpr genType half_pi();

    /** return pi / 2 * 3 */
    template <typename genType>
    constexpr genType three_over_two_pi();

    /** return pi / 4 */
    template <typename genType>
    constexpr genType quarter_pi();

    /** return 1 / pi */
    template <typename genType>
    constexpr genType one_over_pi();

    /** return 1 / (pi * 2) */
    template <typename genType>
    constexpr genType one_over_two_pi();

    /** return 2 / pi */
    template <typename genType>
    constexpr genType two_over_pi();

    /** return 4 / pi */
    template <typename genType>
    constexpr genType four_over_pi();

    /** return 2 / sqrt(pi) */
    template <typename genType>
    constexpr genType two_over_root_pi();

    /** return 1 / sqrt(2) */
    template <typename genType>
    constexpr genType one_over_root_two();

    /** return sqrt(pi / 2) */
    template <typename genType>
    constexpr genType root_half_pi();

    /** return sqrt(2 * pi) */
    template <typename genType>
    constexpr genType root_two_pi();

    /** return sqrt(ln(4)) */
    template <typename genType>
    constexpr genType root_ln_four();

    /** return e constant */
    template <typename genType>
    constexpr genType e();

    /** return Euler constant */
    template <typename genType>
    constexpr genType euler();

    /** return sqrt(2) */
    template <typename genType>
    constexpr genType root_two();

} // namespace math
} // namespace ocf

#include "constants.inl"
