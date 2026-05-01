// SPDX-License-Identifier: MIT
#pragma once
#include "ocf/rhi/Device.h"
#include "ocf/rhi/RHIEnums.h"

namespace ocf::rhi {

struct PipelineState {
    PrimitiveType primitiveType = PrimitiveType::Triangles;
    RasterState rasterState;
    Handle<RHIShaderModule> vertexShader;
    Handle<RHIShaderModule> fragmentShader;
    Handle<RHIVertexBufferInfo> vertexBufferInfo;
};

} // namespace ocf::rhi