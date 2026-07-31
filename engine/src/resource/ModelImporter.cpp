#include "ocf/resource/ModelImporter.h"

#include "ocf/platform/FileSystem.h"
#include "ocf/resource/Mesh.h"
#include "ocf/resource/ObjLoader.h"

namespace ocf {

ModelImporter::ModelImporter()
{
    registerLoader(std::make_unique<ObjLoader>());
}

void ModelImporter::registerLoader(std::unique_ptr<IModelLoader> loader)
{
    m_loaders.push_back(std::move(loader));
}

bool ModelImporter::import(std::string_view filePath, Mesh& mesh)
{
    const auto fullPath = FileSystem::getInstance()->getAssetFullPath(filePath);
    if (fullPath.empty()) {
        return false;
    }

    const auto& ext = FileSystem::getInstance()->getExtension(filePath);

    for (const auto& loader : m_loaders) {
        if (loader->supportsExtension(ext)) {
            return loader->load(fullPath, mesh);
        }
    }

    return false;
}

} // namespace ocf
