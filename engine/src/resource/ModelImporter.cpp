// SPDX-License-Identifier: MIT
#include "ocf/resource/ModelImporter.h"

#include "resource/GLTFLoader.h"
#include "resource/ObjLoader.h"

#include "ocf/platform/FileSystem.h"
#include "ocf/resource/Mesh.h"

namespace ocf {

ModelImporter::ModelImporter()
{
    registerLoader(std::make_unique<GLTFLoader>());
    registerLoader(std::make_unique<ObjLoader>());
}

void ModelImporter::registerLoader(std::unique_ptr<IModelLoader> loader)
{
    m_loaders.push_back(std::move(loader));
}

ModelImporter::Result ModelImporter::loadFromFile(std::string_view filePath)
{
    const auto fullPath = FileSystem::getInstance()->getAssetFullPath(filePath);
    if (fullPath.empty()) {
        return Result::Err("Failed to get full path");
    }

    const auto& ext = FileSystem::getInstance()->getExtension(filePath);

    for (const auto& loader : m_loaders) {
        if (!loader->supportsExtension(ext)) {
            continue;
        }

        Mesh* mesh = loader->createMesh(fullPath);
        if (!mesh) {
            return Result::Err("Failed to create mesh");
        }
        else {
            return Result::Ok(Ref<Mesh>(mesh));
        }
    }

    return Result::Err("Unsupported file extension");
}

} // namespace ocf
