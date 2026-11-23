// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/compiler.h"
#include "ocf/rhi/Handle.h"
#include "ocf/rhi/RHIEnums.h"

#include <string_view>

namespace ocf {

namespace rhi {
class Device;
}

/** @brief Creates shader modules from the compiled shader files of the device's backend. */
class OCF_API ShaderLoader {
public:
    /**
     * @brief Read a compiled shader file and create its shader module.
     *
     * @param filePath The full path of the file
     * @return A null handle if the file cannot be read or its code is invalid
     */
    static rhi::ShaderModuleHandle load(rhi::Device& device, rhi::ShaderStage stage,
                                        std::string_view filePath,
                                        const char* entryPoint = "main");
};

} // namespace ocf
