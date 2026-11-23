// SPDX-License-Identifier: MIT
#include "ocf/renderer/ShaderLoader.h"

#include "ocf/core/Logger.h"
#include "ocf/rhi/Device.h"

#include <fstream>
#include <string>
#include <vector>

namespace ocf {

rhi::ShaderModuleHandle ShaderLoader::load(rhi::Device& device, rhi::ShaderStage stage,
                                           std::string_view filePath, const char* entryPoint)
{
    const std::string path(filePath);
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        OCF_LOG_ERROR("Failed to open shader file: {}", path);
        return {};
    }

    const auto fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> code(fileSize);
    if (!file.seekg(0).read(code.data(), std::streamsize(fileSize))) {
        OCF_LOG_ERROR("Failed to read shader file: {}", path);
        return {};
    }

    return device.createShaderModule(stage, code.data(), code.size(), entryPoint);
}

} // namespace ocf
