// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/rhi/Handle.h"
#include "ocf/rhi/RHIEnums.h"

#include <array>
#include <cstdint>

namespace ocf::rhi {

struct RHIShaderModule;
struct RHIVertexBufferInfo;
struct RHIDescriptorSetLayout;

struct PushConstantRange {
    ShaderStageFlags stages = ShaderStageFlags::Vertex;
    uint32_t size = 0; ///< 0 means the pipeline has no push constants
};

struct PipelineLayout {
    using SetLayout = std::array<Handle<RHIDescriptorSetLayout>, DESCRIPTOR_SET_COUNT_MAX>;
    SetLayout setLayout;
    PushConstantRange pushConstant;
};

struct PipelineState {
    Handle<RHIShaderModule> vertexShader;
    Handle<RHIShaderModule> fragmentShader;
    Handle<RHIVertexBufferInfo> vertexBufferInfo;
    PipelineLayout pipelineLayout;
    RasterState rasterState;
    PrimitiveType primitiveType = PrimitiveType::Triangles;
    uint8_t padding[3] = {};
};

} // namespace ocf::rhi
