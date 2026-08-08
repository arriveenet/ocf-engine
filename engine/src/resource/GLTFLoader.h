// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/resource/IModelLoader.h"

#include <vector>

struct cgltf_primitive;
struct cgltf_material;
struct cgltf_texture_view;  

namespace ocf {

class GLTFLoader : public IModelLoader {
public:
    GLTFLoader();
    ~GLTFLoader() override;

    bool load(std::string_view fileName, Mesh& mesh) override;

    bool supportsExtension(const std::string& extension) const override;

private:

    void processPrimitive(const cgltf_primitive& primitive, Mesh& mesh);

    void processMaterial(const cgltf_material& material, Mesh::MaterialParams& materialParams,
                         std::unordered_map<std::string, Mesh::TextureData>& textures);

    void processTexture(const char* name, const cgltf_texture_view& textureView,
                        std::unordered_map<std::string, Mesh::TextureData>& textures);

private:
    std::string m_filePath;
};

} // namespace ocf
