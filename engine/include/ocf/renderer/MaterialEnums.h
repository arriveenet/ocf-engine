// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>

namespace ocf {

enum class VertexAttribute : uint8_t {
    Position = 0,
    Normal = 1,
    Tangent = 2,
    Color = 3,
    TexCoord0 = 4,
    TexCoord1 = 5,
    BoneIndices = 6,
    BoneWeights = 7,
    Custom0 = 8,
    Custom1 = 9,
    Custom2 = 10,
    Custom3 = 11,
    Custom4 = 12,
    Custom5 = 13,
    Custom6 = 14,
    Custom7 = 15,
    Custom8 = 16
};

enum class AlphaMode : uint8_t {
    Opaque = 0,
    Mask   = 1,
    Blend  = 2
};

} // namespace ocf
