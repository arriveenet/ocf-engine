// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/resource/IModelLoader.h"

namespace ocf {

class Engine;

class ObjLoader : public IModelLoader {
public:
    ObjLoader();
    ~ObjLoader() override;

    bool load(std::string_view fileName, Mesh& mesh) override;

    Mesh* createMesh(std::string_view fileName) override;

    bool supportsExtension(const std::string& extension) const override;
};

} // namespace ocf
