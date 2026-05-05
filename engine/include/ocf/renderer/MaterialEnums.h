// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>

namespace ocf {

enum class VertexAttribute : uint8_t {
    Position = 0,
    Normal = 1,
    Color = 2,
    TexCoord0 = 3,
    TexCoord1 = 4,
    BoneIndices = 5,
    BoneWeights = 6,
    Custom0 = 7,
    Custom1 = 8,
    Custom2 = 9,
    Custom3 = 10,
    Custom4 = 11,
    Custom5 = 12,
    Custom6 = 13,
    Custom7 = 14,
    Custom8 = 15
};

} // namespace ocf
