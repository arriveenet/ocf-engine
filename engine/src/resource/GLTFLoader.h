// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/math/mat4.h"
#include "ocf/resource/IModelLoader.h"

#include <vector>

struct cgltf_primitive;
struct cgltf_material;
struct cgltf_texture_view;
struct cgltf_node;
struct cgltf_data;

namespace ocf {

class GLTFLoader : public IModelLoader {
public:
    GLTFLoader();
    ~GLTFLoader() override;

    bool load(std::string_view fileName, Mesh& mesh) override;

    Mesh* createMesh(std::string_view fileName) override;

    bool supportsExtension(const std::string& extension) const override;

private:

    void recursePrimitives(const cgltf_node* node, Mesh* mesh);

    void createPrimitive(const cgltf_node* node, Mesh* mesh);

    void processPrimitive(const cgltf_primitive& primitive, const math::mat4& transform,
                          Mesh* mesh);

    void processMaterial(const cgltf_material& material, Mesh::MaterialParams& materialParams,
                         std::unordered_map<std::string, Mesh::TextureData>& textures);

    void processTexture(const char* name, const cgltf_texture_view& textureView,
                        std::unordered_map<std::string, Mesh::TextureData>& textures);

private:
    std::string m_gltfPath;
    cgltf_data* m_gltfData = nullptr;
};

} // namespace ocf
