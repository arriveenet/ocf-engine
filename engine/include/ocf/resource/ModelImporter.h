// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/Result.h"
#include "ocf/resource/IModelLoader.h"

#include <memory>
#include <string_view>
#include <vector>

namespace ocf {

class Mesh;

class ModelImporter {
public:
    using Result = ocf::Result<Ref<Mesh>, const char*>;

    ModelImporter();

    void registerLoader(std::unique_ptr<IModelLoader> loader);

    Result loadFromFile(std::string_view filePath);

private:
    std::vector<std::unique_ptr<IModelLoader>> m_loaders;
};

} // namespace ocf
