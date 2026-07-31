// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/resource/IModelLoader.h"

#include <memory>
#include <string_view>
#include <vector>

namespace ocf {

class Mesh;

class ModelImporter {
public:
    ModelImporter();

    void registerLoader(std::unique_ptr<IModelLoader> loader);

    bool import(std::string_view filePath, Mesh& mesh);

private:
    std::vector<std::unique_ptr<IModelLoader>> m_loaders;
};

} // namespace ocf
