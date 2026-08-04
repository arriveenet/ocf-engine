// SPDX-License-Identifier: MIT
#include "ocf/resource/Mesh.h"

#include "ocf/core/Logger.h"
#include "ocf/renderer/Material.h"
#include "ocf/renderer/VertexBuffer.h"
#include "ocf/renderer/IndexBuffer.h"

#include <cstring>

namespace ocf {

using namespace math;

Mesh::Mesh()
{
}

Mesh::~Mesh()
{
}

void Mesh::createSubMeshBuffers(Engine& engine)
{
    for (auto& subMeshLoad : m_subMeshLoads) {
        // Create vertex buffer
        VertexBuffer* vertexBuffer =
            createVertexBuffer(engine, subMeshLoad.format, subMeshLoad.vertexCount,
                               subMeshLoad.offsets, subMeshLoad.vertexStride,
                               subMeshLoad.vertexArray.data(), subMeshLoad.vertexArray.size());
        assert(vertexBuffer && "Failed to create vertex buffer");

        // Create index buffer
        IndexBuffer* indexBuffer =
            createIndexBuffer(engine, subMeshLoad.indexCount, subMeshLoad.indexArray.data(),
                              subMeshLoad.indexArray.size());
        assert(indexBuffer && "Failed to create index buffer");

        // Store buffers in the submesh
        SubMesh subMesh{
            .format = subMeshLoad.format,
            .primitive = PrimitiveType::Triangles, // Assuming triangles for now; adjust as needed
            .vertexBuffer = vertexBuffer,
            .indexBuffer = indexBuffer,
            .material = nullptr};
        m_subMeshes.push_back(subMesh);
    }

    m_subMeshLoads.clear();
}

void Mesh::terminate(Engine& engine)
{
    for (auto& subMesh : m_subMeshes) {
        if (subMesh.vertexBuffer) {
            subMesh.vertexBuffer->terminate(engine);
            delete subMesh.vertexBuffer;
        }
        if (subMesh.indexBuffer) {
            subMesh.indexBuffer->terminate(engine);
            delete subMesh.indexBuffer;
        }
    }
    m_subMeshes.clear();
}

int Mesh::getSubMeshCount() const
{
    return static_cast<int>(m_subMeshes.size());
}

const Mesh::SubMesh& Mesh::getSubMesh(int index) const
{
    assert(index >= 0 && index < static_cast<int>(m_subMeshes.size()) && "Index out of bounds");
    return m_subMeshes[index];
}

void Mesh::addSubMeshFromArrays(PrimitiveType primitive,
                                const std::array<Variant, ArrayType::ArrayMax>& arrays)
{
    uint64_t format = 0;

    size_t vertexCount = 0;
    size_t indexCount = 0;

    // Determine format and lengths
    for (size_t i = 0; i < arrays.size(); i++) {
        if (std::holds_alternative<NoneType>(arrays[i])) {
            continue;
        }

        format |= (1ull << i);

        if (i == ArrayType::ArrayVertex) {
            if (std::holds_alternative<PackedVec3Array>(arrays[i])) {
                vertexCount = std::get<PackedVec3Array>(arrays[i]).size();
            }
        }

        if (i == ArrayType::ArrayIndex) {
            if (std::holds_alternative<PackedUint32Array>(arrays[i])) {
                indexCount = std::get<PackedUint32Array>(arrays[i]).size();
            }
        }
    }

    // Prepare offsets and vertex element size
    std::array<uint32_t, ArrayType::ArrayMax> offsets{};
    uint32_t vertexElementSize = 0;

    makeOffsetsFromFormat(format, offsets, vertexElementSize);

    const size_t vertexArraySize = vertexCount * vertexElementSize;
    const size_t indexArraySize = indexCount * sizeof(uint32_t);

    std::vector<uint8_t> vertexArray;
    vertexArray.resize(vertexArraySize);

    std::vector<uint8_t> indexArray;
    indexArray.resize(indexArraySize);

    // Set surface data
    const bool result = setSurfaceData(arrays, format, offsets, vertexElementSize, vertexArray,
                                       vertexCount, indexArray, indexCount);
    assert(result && "Failed to set surface data");

    m_subMeshLoads.push_back({format, offsets, static_cast<uint8_t>(vertexElementSize),
                              static_cast<uint32_t>(vertexCount), static_cast<uint32_t>(indexCount),
                              std::move(vertexArray), std::move(indexArray)});
}

void Mesh::makeOffsetsFromFormat(uint64_t format,
                                 std::array<uint32_t, ArrayType::ArrayMax>& offsets,
                                 uint32_t& vertexElementSize)
{
    vertexElementSize = 0;

    for (int i = 0; i < ArrayType::ArrayMax; i++) {
        // reset offset
        offsets[i] = 0;

        if (!(format & (1ull << i))) {
            continue;
        }

        int elementSize = 0;

        switch (i) {
        case ArrayType::ArrayVertex:
            elementSize = sizeof(float) * 3;
            break;
        case ArrayType::ArrayNormal:
            elementSize = sizeof(float) * 3;
            break;
        case ArrayType::ArrayColor:
            elementSize = sizeof(float) * 4;
            break;
        case ArrayType::ArrayTexCoord0:
        case ArrayType::ArrayTexCoord1:
            elementSize = sizeof(float) * 2;
            break;
        default:
            break;
        }

        offsets[i] = vertexElementSize;
        vertexElementSize += elementSize;
    }
}

bool Mesh::setSurfaceData(const std::array<Variant, ArrayType::ArrayMax>& arrays, uint64_t format,
                          const std::array<uint32_t, ArrayType::ArrayMax>& offsets,
                          uint32_t vertexStride, std::vector<uint8_t>& vertexArray,
                          size_t vertexArrayLength, std::vector<uint8_t>& indexArray,
                          size_t indexArrayLength)
{
    uint8_t* basePtr = vertexArray.data();

    for (int index = 0; index < ArrayType::ArrayMax; index++) {
        if (!(format & (1ull << index))) {
            continue;
        }
        switch (index) {
        case ArrayType::ArrayVertex: {
            PackedVec3Array array = std::get<PackedVec3Array>(arrays[index]);
            assert((array.size() == vertexArrayLength) && "Vertex array size mismatch");

            const vec3* src = array.data();

            for (size_t i = 0; i < vertexArrayLength; i++) {
                float vector[3] = {src[i].x, src[i].y, src[i].z};
                memcpy(&basePtr[offsets[index] + i * vertexStride], vector, sizeof(float) * 3);
            }
            break;
        }
        case ArrayType::ArrayNormal: {
            PackedVec3Array array = std::get<PackedVec3Array>(arrays[index]);
            assert((array.size() == vertexArrayLength) && "Normal array size mismatch");

            const vec3* src = array.data();

            for (size_t i = 0; i < vertexArrayLength; i++) {
                float vector[3] = {src[i].x, src[i].y, src[i].z};
                memcpy(&basePtr[offsets[index] + i * vertexStride], vector, sizeof(float) * 3);
            }
            break;
        }
        case ArrayType::ArrayColor: {
            PackedVec4Array array = std::get<PackedVec4Array>(arrays[index]);
            assert((array.size() == vertexArrayLength) && "Color array size mismatch");

            const vec4* src = array.data();

            for (size_t i = 0; i < vertexArrayLength; i++) {
                float vector[4] = {src[i].x, src[i].y, src[i].z, src[i].w};
                memcpy(&basePtr[offsets[index] + i * vertexStride], vector, sizeof(float) * 4);
            }
            break;
        }
        case ArrayType::ArrayTexCoord0:
        case ArrayType::ArrayTexCoord1: {
            PackedVec2Array array = std::get<PackedVec2Array>(arrays[index]);

            assert((array.size() == vertexArrayLength) && "TexCoord0 array size mismatch");

            const vec2* src = array.data();

            for (size_t i = 0; i < vertexArrayLength; i++) {
                float vector[2] = {src[i].x, src[i].y};
                memcpy(&basePtr[offsets[index] + i * vertexStride], vector, sizeof(float) * 2);
            }
            break;
        }
        case ArrayType::ArrayIndex: {
            PackedUint32Array array = std::get<PackedUint32Array>(arrays[index]);

            assert((array.size() == indexArrayLength) && "Index array size mismatch");

            uint32_t* dest = reinterpret_cast<uint32_t*>(indexArray.data());
            const uint32_t* src = array.data();
            memcpy(dest, src, sizeof(uint32_t) * indexArrayLength);
            break;
        }
        default:
            // Other attributes can be handled similarly
            OCF_LOG_WARN("Unsupported array type: {}", index);
            break;
        }
    }
    return true;
}

VertexBuffer* Mesh::createVertexBuffer(Engine& engine, uint64_t format, uint32_t vertexCount,
                                       const std::array<uint32_t, ArrayType::ArrayMax>& offsets,
                                       uint8_t stride, const void* data, size_t size)
{
    VertexBuffer::Builder builder;
    if ((format & ArrayFormat::ArrayFormatVertex) != 0) {
        builder.attribute(VertexAttribute::Position, VertexBuffer::AttributeType::Float3, stride,
                             offsets[ArrayType::ArrayVertex]);
    }
    if ((format & ArrayFormat::ArrayFormatNormal) != 0) {
        builder.attribute(VertexAttribute::Normal, VertexBuffer::AttributeType::Float3, stride,
                          offsets[ArrayType::ArrayNormal]);
    }
    if ((format & ArrayFormat::ArrayFormatTexCoord0) != 0) {
        builder.attribute(VertexAttribute::TexCoord0, VertexBuffer::AttributeType::Float2, stride,
                          offsets[ArrayType::ArrayTexCoord0]);
    }
    builder.bufferCount(1);
    builder.vertexCount(vertexCount);
    
    VertexBuffer* vb = builder.build(engine);
    vb->setBufferData(engine, data, size, 0);

    return vb;
}

IndexBuffer* Mesh::createIndexBuffer(Engine& engine, uint32_t indexCount, const void* data,
                                     size_t size)
{
    IndexBuffer* ib = IndexBuffer::Builder()
                          .indexType(IndexBuffer::IndexType::Uint)
                          .indexCount(indexCount)
                          .build(engine);
    ib->setBufferData(engine, data, size, 0);

    return ib;
}

} // namespace ocf
