// SPDX-License-Identifier: MIT
#include "ObjLoader.h"

#include "ocf/core/Logger.h"
#include "ocf/math/vec2.h"
#include "ocf/math/vec3.h"
#include "ocf/platform/FileSystem.h"
#include "ocf/resource/Mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <unordered_map>

struct Vertex {
    ocf::math::vec3 position;
    ocf::math::vec3 normal;
    ocf::math::vec2 texCoord;

    bool operator==(const Vertex& other) const
    {
        return position == other.position && normal == other.normal && texCoord == other.texCoord;
    }
};

namespace std {

template <>
struct hash<ocf::math::vec2> {
    size_t operator()(const ocf::math::vec2& v) const noexcept
    {
        size_t h1 = std::hash<float>()(v.x);
        size_t h2 = std::hash<float>()(v.y);
        return h1 ^ (h2 << 1);
    }
};

template <>
struct hash<ocf::math::vec3> {
    size_t operator()(const ocf::math::vec3&v) const noexcept
    {
        size_t h1 = std::hash<float>()(v.x);
        size_t h2 = std::hash<float>()(v.y);
        size_t h3 = std::hash<float>()(v.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

template <>
struct hash<Vertex> {
    size_t operator()(const Vertex& v) const
    {
        return ((hash<ocf::math::vec3>()(v.position) ^ (hash<ocf::math::vec3>()(v.normal) << 1)) >>
                1) ^
               (hash<ocf::math::vec2>()(v.texCoord) << 1);
    }
};

} // namespace std

namespace ocf {

using namespace math;

ObjLoader::ObjLoader()
{
}

ObjLoader::~ObjLoader()
{
}

bool ObjLoader::load(std::string_view fileName, Mesh& mesh)
{
    tinyobj::ObjReaderConfig readerConfig;
    readerConfig.mtl_search_path = FileSystem::getInstance()->getParentFullPath(fileName);

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(fileName.data(), readerConfig)) {
        if (!reader.Error().empty()) {
            OCF_LOG_ERROR("TinyObjReader: {}", reader.Error().c_str());
        }
        return false;
    }

    if (!reader.Warning().empty()) {
        OCF_LOG_ERROR("TinyObjReader: {}", reader.Warning().c_str());
    }

    const tinyobj::attrib_t& attrib = reader.GetAttrib();
    const std::vector<tinyobj::shape_t>& shapes = reader.GetShapes();
    //const std::vector<tinyobj::material_t>& materials = reader.GetMaterials();

    for (size_t s = 0; s < shapes.size(); s++) {
        std::array<Variant, Mesh::ArrayType::ArrayMax> arrays;
        std::unordered_map<Vertex, uint32_t> uniqueVertices;
        PackedVec3Array positions;
        PackedVec3Array normals;
        PackedVec2Array texCoords;
        PackedUint32Array indices;
        size_t indexOffset = 0;

        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = shapes[s].mesh.num_face_vertices[f];

             // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t index = shapes[s].mesh.indices[indexOffset + v];

                Vertex vertex{};

                 // access to vertex
                vertex.position =
                    vec3{attrib.vertices[3 * static_cast<size_t>(index.vertex_index) + 0],
                         attrib.vertices[3 * static_cast<size_t>(index.vertex_index) + 1],
                         attrib.vertices[3 * static_cast<size_t>(index.vertex_index) + 2]};

                 // Check if `normal_index` is zero or positive. negative = no normal data
                if (index.normal_index >= 0) {
                    vertex.normal =
                        vec3{attrib.normals[3 * static_cast<size_t>(index.normal_index) + 0],
                             attrib.normals[3 * static_cast<size_t>(index.normal_index) + 1],
                             attrib.normals[3 * static_cast<size_t>(index.normal_index) + 2]};
                }

                // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                if (index.texcoord_index >= 0) {
                    vertex.texCoord =
                        vec2{attrib.texcoords[2 * static_cast<size_t>(index.texcoord_index) + 0],
                             attrib.texcoords[2 * static_cast<size_t>(index.texcoord_index) + 1]};
                }

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(uniqueVertices.size());
                    positions.push_back(vertex.position);
                    normals.push_back(vertex.normal);
                    texCoords.push_back(vertex.texCoord);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
            indexOffset += fv;

            // shapes[s].mesh.material_ids[f];
        }

        arrays[Mesh::ArrayType::ArrayVertex]    = std::move(positions);
        arrays[Mesh::ArrayType::ArrayNormal]    = std::move(normals);
        arrays[Mesh::ArrayType::ArrayTexCoord0] = std::move(texCoords);
        arrays[Mesh::ArrayType::ArrayIndex]     = std::move(indices);

        Mesh::MaterialParams materialParams;
        std::unordered_map<std::string, Mesh::TextureData> textures;
        
        mesh.addSubMeshFromArrays(Mesh::PrimitiveType::Triangles, arrays, materialParams, textures);
    }

    return true;
}

bool ObjLoader::supportsExtension(const std::string& extension) const
{
    if (extension == "obj") {
        return true;
    }

    return false;
}

} // namespace ocf
