// SPDX-License-Identifier: MIT
#pragma once
#include "ocf/math/vec2.h"
#include "ocf/math/vec3.h"
#include "ocf/math/vec4.h"
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace ocf {

using NoneType = std::monostate;
using PackedUint32Array = std::vector<uint32_t>;
using PackedVec2Array = std::vector<math::vec2>;
using PackedVec3Array = std::vector<math::vec3>;
using PackedVec4Array = std::vector<math::vec4>;

enum class VariantType {
    None = 0,
    Bool,
    Int,
    Float,
    String,
    PackedUint32Array,
    PackedVec2Array,
    PackedVec3Array,
    PackedVec4Array,
};

using Variant = std::variant<NoneType, bool, int, float, std::string,
                             PackedUint32Array, PackedVec2Array, PackedVec3Array, PackedVec4Array>;

} // namespace ocf
