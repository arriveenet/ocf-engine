// SPDX-License-Identifier: MIT
#pragma once
#include "ocf/resource/Mesh.h"

#include <string>
#include <string_view>

namespace ocf {
    
class Engine;

class IModelLoader {
public:
    IModelLoader() = default;
    virtual ~IModelLoader() = default;

    virtual bool load(std::string_view fileName, Mesh& mesh) = 0;

    virtual bool supportsExtension(const std::string& extension) const = 0;
};

} // namespace ocf
