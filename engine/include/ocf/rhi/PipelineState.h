// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/rhi/Handle.h"
#include "ocf/rhi/RHIEnums.h"

namespace ocf::rhi {

struct RHIShaderModule;
struct RHIVertexBufferInfo;
struct RHIDescriptorSetLayout;

struct PipelineState {
    PrimitiveType primitiveType = PrimitiveType::Triangles;
    RasterState rasterState;
    Handle<RHIShaderModule> vertexShader;
    Handle<RHIShaderModule> fragmentShader;
    Handle<RHIVertexBufferInfo> vertexBufferInfo;
    Handle<RHIDescriptorSetLayout> layout;
};

} // namespace ocf::rhi
