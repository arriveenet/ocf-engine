// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/Variant.h"
#include "ocf/resource/Resource.h"
#include "ocf/rhi/RHIEnums.h"

#include <array>
#include <cstdint>
#include <vector>

namespace ocf {

class Engine;
class Material;
class VertexBuffer;
class IndexBuffer;

class Mesh : public Resource {
public:
    using PrimitiveType = rhi::PrimitiveType;

    enum ArrayType : uint8_t {
        ArrayVertex     = 0,
        ArrayNormal     = 1,
        ArrayColor      = 2,
        ArrayTexCoord0  = 3,
        ArrayTexCoord1  = 4,
        ArrayIndex      = 5,
        ArrayMax        = 6
    };

   enum ArrayFormat : uint64_t {
        ArrayFormatVertex       = 1 << ArrayVertex,
        ArrayFormatNormal       = 1 << ArrayNormal,
        ArrayFormatColor        = 1 << ArrayColor,
        ArrayFormatTexCoord0    = 1 << ArrayTexCoord0,
        ArrayFormatTexCoord1    = 1 << ArrayTexCoord1,
        ArrayFormatIndex        = 1 << ArrayIndex
    };

    struct SubMesh {
       uint64_t format;
       PrimitiveType primitive;
       VertexBuffer* vertexBuffer;
       IndexBuffer* indexBuffer;
       Material* material;
   };

    Mesh();
    virtual ~Mesh();

    void createSubMeshBuffers(Engine& engine);

    void terminate(Engine& engine);

    int getSubMeshCount() const;

    const SubMesh& getSubMesh(int index) const;

    void addSubMeshFromArrays(PrimitiveType primitive,
                              const std::array<Variant, ArrayType::ArrayMax>& arrays);

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
    };
    std::vector<SubMeshLoad> m_subMeshLoads;

    std::vector<SubMesh> m_subMeshes;
};

} // namespace ocf
