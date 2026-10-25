// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/math/vec2.h"
#include "ocf/math/vec3.h"

namespace ocf {

/*
 * CPU-side vertex layouts used by the engine. They are named after their attributes in
 * order; the GPU layout is still declared with VertexBuffer::Builder::attribute(), so each
 * struct must stay tightly packed to match the strides and offsets passed there.
 */

struct VertexPositionTexCoord {
    math::vec3 position;
    math::vec2 texCoord;
};
static_assert(sizeof(VertexPositionTexCoord) == sizeof(float) * 5,
              "VertexPositionTexCoord must be tightly packed");

} // namespace ocf
