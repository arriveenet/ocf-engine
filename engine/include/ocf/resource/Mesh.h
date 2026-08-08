// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/Variant.h"
#include "ocf/math/Vec4.h"
#include "ocf/renderer/MaterialEnums.h"
#include "ocf/renderer/TextureSampler.h"
#include "ocf/resource/Resource.h"
#include "ocf/rhi/RHIEnums.h"

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace ocf {

class Engine;
class Material;
class VertexBuffer;
class IndexBuffer;
class Texture;

class Mesh : public Resource {
public:
    using PrimitiveType = rhi::PrimitiveType;

    enum ArrayType : uint8_t {
        ArrayVertex     = 0,
        ArrayNormal     = 1,
        ArrayTangent    = 2,
        ArrayColor      = 3,
        ArrayTexCoord0  = 4,
        ArrayTexCoord1  = 5,
        ArrayIndex      = 6,
        ArrayMax        = 7
    };

    enum ArrayFormat : uint64_t {
        ArrayFormatVertex       = 1 << ArrayVertex,
        ArrayFormatNormal       = 1 << ArrayNormal,
        ArrayFormatTangent      = 1 << ArrayTangent,
        ArrayFormatColor        = 1 << ArrayColor,
        ArrayFormatTexCoord0    = 1 << ArrayTexCoord0,
        ArrayFormatTexCoord1    = 1 << ArrayTexCoord1,
        ArrayFormatIndex        = 1 << ArrayIndex
    };

    struct MaterialParams {
        std::string name;
        math::vec4 baseColorFactor = math::vec4(1.0f);
        float metallicFactor = 0.0f;
        float roughnessFactor = 1.0f;
        float alphaCutoff = 0.5f;
        AlphaMode alphaMode = AlphaMode::Opaque;
        bool hasNormalMap = false;
    };

    struct TextureData {
        std::string uri;
        std::vector<uint8_t> data;
        TextureSampler sampler;
    };

    struct SubMesh {
        uint64_t format;
        PrimitiveType primitive;
        VertexBuffer* vertexBuffer;
        IndexBuffer* indexBuffer;
        Texture* baseColorTexture;
        Texture* metallicRoughnessTexture;
        Texture* normalMapTexture;
        Material* material;
        MaterialParams materialParams;
   };

    Mesh();
    virtual ~Mesh();

    void createSubMeshBuffers(Engine& engine);

    void terminate(Engine& engine);

    int getSubMeshCount() const;

    const SubMesh& getSubMesh(int index) const;

    void addSubMeshFromArrays(PrimitiveType primitive,
                              const std::array<Variant, ArrayType::ArrayMax>& arrays,
                              const MaterialParams& materialParams,
                              const std::unordered_map<std::string, TextureData>& textures);

protected:
    void makeOffsetsFromFormat(uint64_t format, std::array<uint32_t, ArrayType::ArrayMax>& offsets,
                               uint32_t& vertexElementSize);

    bool setSurfaceData(const std::array<Variant, ArrayType::ArrayMax>& arrays, uint64_t format,
                        const std::array<uint32_t, ArrayType::ArrayMax>& offsets,
                        uint32_t vertexStride, std::vector<uint8_t>& vertexArray,
                        size_t vertexArrayLength, std::vector<uint8_t>& indexArray,
                        size_t indexArrayLength);

    VertexBuffer* createVertexBuffer(Engine& engine, uint64_t format, uint32_t vertexCount,
                                     const std::array<uint32_t, ArrayType::ArrayMax>& offsets,
                                     uint8_t stride, const void* data, size_t size);

    IndexBuffer* createIndexBuffer(Engine& engine, uint32_t indexCount, const void* data,
                                   size_t size);

private:
    struct SubMeshLoad {
        uint64_t format;
        std::array<uint32_t, ArrayType::ArrayMax> offsets;
        uint8_t vertexStride;
        uint32_t vertexCount;
        uint32_t indexCount;
        std::vector<uint8_t> vertexArray;
        std::vector<uint8_t> indexArray;
        std::unordered_map<std::string, TextureData> textures;
    };

    std::vector<SubMeshLoad> m_subMeshLoads;
    std::vector<SubMesh> m_subMeshes;
};

} // namespace ocf
