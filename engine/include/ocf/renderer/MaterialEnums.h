#pragma once
#include <stdint.h>

namespace ocf {

enum class VertexAttribute : uint8_t {
    POSITION = 0,
    NORMAL = 1,
    COLOR = 2,
    TEXCOORD0 = 3,
    TEXCOORD1 = 4,
    BONE_INDICES = 5,
    BONE_WEIGHTS = 6,
    CUSTOM0 = 7,
    CUSTOM1 = 8,
    CUSTOM2 = 9,
    CUSTOM3 = 10,
    CUSTOM4 = 11,
    CUSTOM5 = 12,
    CUSTOM6 = 13,
    CUSTOM7 = 14,
    CUSTOM8 = 15
};

} // namespace ocf
