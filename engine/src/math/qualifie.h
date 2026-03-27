#pragma once
#include <stdint.h>

namespace ocf {
namespace math {

#define OCF_XYZW_ONLY 0

typedef int length_t;

template <length_t L, typename T> struct vec;
template <length_t C, length_t R, typename T> struct mat;

} // namespace math
} // namespace ocf
