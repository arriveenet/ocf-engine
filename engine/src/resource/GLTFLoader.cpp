// SPDX-License-Identifier: MIT
#ifdef _WIN32
#   define _CRT_SECURE_NO_WARNINGS
#endif

#include "GLTFLoader.h"

#include "ocf/core/Logger.h"
#include "ocf/math/vec2.h"
#include "ocf/math/vec3.h"
#include "ocf/math/quat.h"
#include "ocf/math/matrix_transform.h"
#include "ocf/platform/FileSystem.h"
#include "ocf/renderer/TextureSampler.h"
#include "ocf/rhi/RHIEnums.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <cassert>
#include <memory>


namespace ocf {

using namespace math;

namespace {

constexpr TextureSampler::MagFilter getMagFilter(cgltf_filter_type filter)
{
    switch (filter) {
    case cgltf_filter_type_nearest:
        return TextureSampler::MagFilter::Nearest;
    case cgltf_filter_type_linear:
        return TextureSampler::MagFilter::Linear;
    default:
        return TextureSampler::MagFilter::Linear;
    }
}

constexpr TextureSampler::MinFilter getMinFilter(cgltf_filter_type filter)
{
    switch (filter) {
    case cgltf_filter_type_nearest:
        return TextureSampler::MinFilter::Nearest;
    case cgltf_filter_type_linear:
        return TextureSampler::MinFilter::Linear;
    case cgltf_filter_type_nearest_mipmap_nearest:
        return TextureSampler::MinFilter::NearestMipmapNearest;
    case cgltf_filter_type_linear_mipmap_nearest:
        return TextureSampler::MinFilter::LinearMipmapNearest;
    case cgltf_filter_type_nearest_mipmap_linear:
        return TextureSampler::MinFilter::NearestMipmapLinear;
    case cgltf_filter_type_linear_mipmap_linear:
        return TextureSampler::MinFilter::LinearMipmapLinear;
    default:
        return TextureSampler::MinFilter::Linear;
    }
}

constexpr TextureSampler::WrapMode getWrapMode(cgltf_wrap_mode wrap)
{
    switch (wrap) {
    case cgltf_wrap_mode_repeat:
        return TextureSampler::WrapMode::Repeat;
    case cgltf_wrap_mode_clamp_to_edge:
        return TextureSampler::WrapMode::ClampToEdge;
    case cgltf_wrap_mode_mirrored_repeat:
        return TextureSampler::WrapMode::MirroredRepeat;
    default:
        return TextureSampler::WrapMode::Repeat;
    }
}
} // anonymous namespace

GLTFLoader::GLTFLoader()
{
}

GLTFLoader::~GLTFLoader()
{
}

bool GLTFLoader::load(std::string_view fileName, Mesh& mesh)
{
    cgltf_options options{};

    cgltf_data* rawData = nullptr;
    if (cgltf_parse_file(&options, fileName.data(), &rawData) != cgltf_result_success) {
        OCF_LOG_ERROR("Failed to parse GLTF file: {}", fileName);
        return false;
    }

    std::unique_ptr<cgltf_data, decltype(&cgltf_free)> data(rawData, cgltf_free);

    if (cgltf_load_buffers(&options, data.get(), fileName.data()) != cgltf_result_success) {
        OCF_LOG_ERROR("Failed to load GLTF buffers: {}", fileName);
        return false;
    }

    if (cgltf_validate(data.get()) != cgltf_result_success) {
        OCF_LOG_ERROR("Failed to validate GLTF data: {}", fileName);
        return false;
    }

    m_gltfPath = FileSystem::getInstance()->getParentFullPath(fileName);

    OCF_LOG_TRACE("Successfully loaded GLTF file: {}", fileName);
    OCF_LOG_TRACE("Meshes : {}", data->meshes_count);
    OCF_LOG_TRACE("Nodes  : {}", data->nodes_count);
    OCF_LOG_TRACE("Scenes : {}", data->scenes_count);

    for (cgltf_size i = 0; i < data->scenes_count; ++i) {
        const cgltf_scene& scene = data->scenes[i];
        OCF_LOG_TRACE("Scene {}: {}", i, scene.name ? scene.name : "Unnamed");
        
        for (cgltf_size j = 0; j < scene.nodes_count; ++j) {
            const cgltf_node* node = scene.nodes[j];
            recursePrimitives(node, &mesh);
        }
    }

    return true;
}

Mesh* GLTFLoader::createMesh(std::string_view fileName)
{
    cgltf_options options{};

    cgltf_data* rawData = nullptr;
    if (cgltf_parse_file(&options, fileName.data(), &rawData) != cgltf_result_success) {
        OCF_LOG_ERROR("Failed to parse GLTF file: {}", fileName);
        return nullptr;
    }

    std::unique_ptr<cgltf_data, decltype(&cgltf_free)> data(rawData, cgltf_free);
    m_gltfData = data.get();

    if (cgltf_load_buffers(&options, data.get(), fileName.data()) != cgltf_result_success) {
        OCF_LOG_ERROR("Failed to load GLTF buffers: {}", fileName);
        return nullptr;
    }

    if (cgltf_validate(data.get()) != cgltf_result_success) {
        OCF_LOG_ERROR("Failed to validate GLTF data: {}", fileName);
        return nullptr;
    }

    m_gltfPath = FileSystem::getInstance()->getParentFullPath(fileName);

    Mesh* mesh = new Mesh();

    OCF_LOG_TRACE("Successfully loaded GLTF file: {}", fileName);

    for (cgltf_size i = 0; i < data->scenes_count; ++i) {
        const cgltf_scene& scene = data->scenes[i];
        OCF_LOG_TRACE("Scene {}: {}", i, scene.name ? scene.name : "Unnamed");

        for (cgltf_size j = 0; j < scene.nodes_count; ++j) {
            const cgltf_node* node = scene.nodes[j];
            recursePrimitives(node, mesh);
        }
    }

    return mesh;
}

bool GLTFLoader::supportsExtension(const std::string& extension) const
{
    return extension == "gltf" || extension == "glb";
}

void GLTFLoader::recursePrimitives(const cgltf_node* node, Mesh* mesh)
{
    if (node->mesh) {
        createPrimitive(node, mesh);
    }

    for (cgltf_size i = 0; i < node->children_count; ++i) {
        recursePrimitives(node->children[i], mesh);
    }
}

void GLTFLoader::createPrimitive(const cgltf_node* node, Mesh* mesh)
{
    OCF_LOG_TRACE("  Node {}: ", node->name ? node->name : "Unnamed");

    math::vec3 translation{0.0f, 0.0f, 0.0f};
    math::quat rotation{0.0f, 0.0f, 0.0f, 1.0f};
    math::vec3 scale{1.0f, 1.0f, 1.0f};

    if (node->has_matrix) {
        OCF_LOG_TRACE("    Transformation: Matrix");
        OCF_LOG_TRACE("    {:.3f}, {:.3f}, {:.3f}, {:.3f}", node->matrix[0], node->matrix[1],
                      node->matrix[2], node->matrix[3]);
        OCF_LOG_TRACE("    {:.3f}, {:.3f}, {:.3f}, {:.3f}", node->matrix[4], node->matrix[5],
                      node->matrix[6], node->matrix[7]);
        OCF_LOG_TRACE("    {:.3f}, {:.3f}, {:.3f}, {:.3f}", node->matrix[8], node->matrix[9],
                      node->matrix[10], node->matrix[11]);
        OCF_LOG_TRACE("    {:.3f}, {:.3f}, {:.3f}, {:.3f}", node->matrix[12], node->matrix[13],
                      node->matrix[14], node->matrix[15]);
    }

    if (node->has_translation) {
        OCF_LOG_TRACE("    Transformation: Translation");
        OCF_LOG_TRACE("    {:.3f}, {:.3f}, {:.3f}", node->translation[0], node->translation[1],
                      node->translation[2]);
        translation = math::vec3{node->translation[0], node->translation[1], node->translation[2]};
    }

    if (node->has_rotation) {
        OCF_LOG_TRACE("    Transformation: Rotation");
        OCF_LOG_TRACE("    {:.3f}, {:.3f}, {:.3f}, {:.3f}", node->rotation[0], node->rotation[1],
                      node->rotation[2], node->rotation[3]);
        rotation =
            math::quat{node->rotation[0], node->rotation[1], node->rotation[2], node->rotation[3]};
    }

    if (node->has_scale) {
        OCF_LOG_TRACE("    Transformation: Scale");
        OCF_LOG_TRACE("    {:.3f}, {:.3f}, {:.3f}", node->scale[0], node->scale[1], node->scale[2]);
        scale = math::vec3{node->scale[0], node->scale[1], node->scale[2]};
    }

    math::mat4 localTransform = math::mat4(1.0f);
    localTransform = math::translate(localTransform, translation);
    localTransform = localTransform * mat4_cast(rotation);
    localTransform = math::scale(localTransform, scale);

    const cgltf_mesh* gltfMesh = node->mesh;
    if (gltfMesh != nullptr) {
        OCF_LOG_TRACE("Mesh : {}", gltfMesh->name ? gltfMesh->name : "Unnamed");
        OCF_LOG_TRACE("  Primitives: {}", gltfMesh->primitives_count);

        for (cgltf_size j = 0; j < gltfMesh->primitives_count; ++j) {
            const cgltf_primitive& primitive = gltfMesh->primitives[j];
            OCF_LOG_TRACE("    Attributes: {}", primitive.attributes_count);

            processPrimitive(primitive, localTransform, mesh);
        }
    }
}

void GLTFLoader::processPrimitive(const cgltf_primitive& primitive, const math::mat4& transform,
                                  Mesh* mesh)
{
    std::array<Variant, Mesh::ArrayType::ArrayMax> arrays;
    PackedVec3Array positions;
    PackedVec3Array normals;
    PackedVec4Array tangents;
    PackedVec2Array texCoords;
    PackedUint32Array indices;

    /* ------------------------- Vertex Attributes ------------------------- */
    for (cgltf_size i = 0; i < primitive.attributes_count; ++i) {
        const cgltf_attribute& attribute = primitive.attributes[i];
        const cgltf_accessor* accessor = attribute.data;

        if (attribute.type == cgltf_attribute_type_position) {
            OCF_LOG_TRACE("    Position attribute: count = {}", accessor->count);
            assert(accessor->type == cgltf_type_vec3 &&
                   accessor->component_type == cgltf_component_type_r_32f);

            float position[3];
            for (cgltf_size j = 0; j < accessor->count; ++j) {
                cgltf_accessor_read_float(accessor, j, position, 3);
                vec4 pos = vec4{position[0], position[1], position[2], 1.0f};
                pos = transform * pos;
                positions.push_back(vec3{pos.x, pos.y, pos.z});
            }

        }
        else if (attribute.type == cgltf_attribute_type_normal) {
            OCF_LOG_TRACE("    Normal attribute: count = {}", accessor->count);
            assert(accessor->type == cgltf_type_vec3 &&
                   accessor->component_type == cgltf_component_type_r_32f);

            float normal[3];
            for (cgltf_size j = 0; j < accessor->count; ++j) {
                cgltf_accessor_read_float(accessor, j, normal, 3);
                normals.push_back(vec3{normal[0], normal[1], normal[2]});
            }
        }
        else if (attribute.type == cgltf_attribute_type_tangent) {
            OCF_LOG_TRACE("    Tangent attribute: count = {}", accessor->count);

            assert(accessor->type == cgltf_type_vec4 &&
                   accessor->component_type == cgltf_component_type_r_32f);

            float tangent[4];
            for (cgltf_size j = 0; j < accessor->count; ++j) {
                cgltf_accessor_read_float(accessor, j, tangent, 4);
                tangents.push_back(vec4{tangent[0], tangent[1], tangent[2], tangent[3]});
            }
        }
        else if (attribute.type == cgltf_attribute_type_texcoord) {
            OCF_LOG_TRACE("    TexCoord attribute: count = {}", accessor->count);
            assert(accessor->type == cgltf_type_vec2 &&
                   accessor->component_type == cgltf_component_type_r_32f);

            float texCoord[2];
            for (cgltf_size j = 0; j < accessor->count; ++j) {
                cgltf_accessor_read_float(accessor, j, texCoord, 2);
                const float u =  texCoord[0];
                const float v =  texCoord[1];
                texCoords.push_back(vec2{u, v});
            }
        }
    }

    assert(!positions.empty() && "Positions array should not be empty");
    assert(!normals.empty() && "Normals array should not be empty");

    if (tangents.empty()) {
        // Generate tangents if they are not provided
        // TODO: Implement tangent generation algorithm
        tangents.resize(positions.size(), vec4{0.0f, 0.0f, 0.0f, 1.0f});
    }

    if (texCoords.empty()) {
        // Generate default texCoords if they are not provided
        texCoords.resize(positions.size(), vec2{0.0f, 0.0f});
    }

    /* ----------------------------- Indices ------------------------------ */
    const cgltf_accessor* indexAccessor = primitive.indices;
    if (indexAccessor) {
        OCF_LOG_TRACE("    Indices: {}", indexAccessor->count);

        uint32_t index = 0;
        for (cgltf_size i = 0; i < indexAccessor->count; ++i) {
            cgltf_accessor_read_uint(indexAccessor, i, &index, 1);
            indices.push_back(index);
        }
    }

    /* ----------------------------- Material ------------------------------ */
    Mesh::MaterialParams materialParams;
    std::unordered_map<std::string, Mesh::TextureData> textures;
    if (primitive.material) {
        processMaterial(*primitive.material, materialParams, textures);
    }

    arrays[Mesh::ArrayType::ArrayVertex] = std::move(positions);
    arrays[Mesh::ArrayType::ArrayNormal] = std::move(normals);
    arrays[Mesh::ArrayType::ArrayTangent] = std::move(tangents);
    arrays[Mesh::ArrayType::ArrayTexCoord0] = std::move(texCoords);
    arrays[Mesh::ArrayType::ArrayIndex] = std::move(indices);

    mesh->addSubMeshFromArrays(Mesh::PrimitiveType::Triangles, arrays, materialParams, textures);
}

void GLTFLoader::processMaterial(const cgltf_material& material, Mesh::MaterialParams& materialParams,
                                 std::unordered_map<std::string, Mesh::TextureData>& textures)
{
    materialParams.name = material.name ? material.name : "Unnamed";
    OCF_LOG_TRACE("    Material: {}", materialParams.name);

    // PBR Metallic Roughness(Base Color, Metallic-Roughness)
    if (material.has_pbr_metallic_roughness) {
        const cgltf_pbr_metallic_roughness& pbr = material.pbr_metallic_roughness;
        materialParams.baseColorFactor = vec4{pbr.base_color_factor[0], pbr.base_color_factor[1],
                                              pbr.base_color_factor[2], pbr.base_color_factor[3]};
        materialParams.metallicFactor = pbr.metallic_factor;
        materialParams.roughnessFactor = pbr.roughness_factor;

        OCF_LOG_TRACE("      Base Color Factor: {}, {}, {}, {}", pbr.base_color_factor[0],
                      pbr.base_color_factor[1], pbr.base_color_factor[2], pbr.base_color_factor[3]);
        OCF_LOG_TRACE("      Metallic Factor: {}", pbr.metallic_factor);
        OCF_LOG_TRACE("      Roughness Factor: {}", pbr.roughness_factor);

        // Base Color Texture
        processTexture("Base Color", pbr.base_color_texture, textures);

        // Metallic-Roughness Texture
        processTexture("Metallic-Roughness", pbr.metallic_roughness_texture, textures);
    }

    // Normal Map
    processTexture("Normal", material.normal_texture, textures);

    // Occlusion Map
    processTexture("Occlusion", material.occlusion_texture, textures);

    // Emissive Map
    processTexture("Emissive", material.emissive_texture, textures);
    OCF_LOG_TRACE("      Emissive Factor: {}, {}, {}", material.emissive_factor[0],
                  material.emissive_factor[1], material.emissive_factor[2]);

    // Alpha Mode
    Mesh::RasterState& rasterState = materialParams.rasterState;
    switch (material.alpha_mode) {
    case cgltf_alpha_mode_opaque:
        break;
    case cgltf_alpha_mode_mask:
        rasterState.bits.blendFunctionSrcColor = Mesh::RasterState::BlendFunction::SrcAlpha;
        rasterState.bits.blendFunctionDstColor = Mesh::RasterState::BlendFunction::OneMinusSrcAlpha;
        rasterState.bits.blendEquationColor = Mesh::RasterState::BlendEquation::Add;
        rasterState.bits.blendFunctionSrcAlpha = Mesh::RasterState::BlendFunction::SrcAlpha;
        rasterState.bits.blendFunctionDstAlpha = Mesh::RasterState::BlendFunction::OneMinusSrcAlpha;
        rasterState.bits.blendEquationAlpha = Mesh::RasterState::BlendEquation::Add;

        rasterState.bits.depthWriteEnable = true;
        break;
    case cgltf_alpha_mode_blend:
        rasterState.bits.blendFunctionSrcColor = Mesh::RasterState::BlendFunction::SrcAlpha;
        rasterState.bits.blendFunctionDstColor = Mesh::RasterState::BlendFunction::OneMinusSrcAlpha;
        rasterState.bits.blendEquationColor = Mesh::RasterState::BlendEquation::Add;
        rasterState.bits.blendFunctionSrcAlpha = Mesh::RasterState::BlendFunction::SrcAlpha;
        rasterState.bits.blendFunctionDstAlpha = Mesh::RasterState::BlendFunction::OneMinusSrcAlpha;
        rasterState.bits.blendEquationAlpha = Mesh::RasterState::BlendEquation::Add;

        rasterState.bits.depthWriteEnable = false;
        break;
    default:
        break;
    }

    const char* alphaModeStr = (material.alpha_mode == cgltf_alpha_mode_opaque)  ? "Opaque"
                               : (material.alpha_mode == cgltf_alpha_mode_mask)  ? "Mask"
                               : (material.alpha_mode == cgltf_alpha_mode_blend) ? "Blend"
                                                                                 : "Unknown";

    OCF_LOG_TRACE("      Alpha Mode: {}", alphaModeStr);
    OCF_LOG_TRACE("      Alpha Cutoff: {}", material.alpha_cutoff);
    OCF_LOG_TRACE("      Double Sided: {}", material.double_sided);
}

void GLTFLoader::processTexture(const char* name, const cgltf_texture_view& textureView,
                                std::unordered_map<std::string, Mesh::TextureData>& textures)
{
    if (!textureView.texture) {
        OCF_LOG_TRACE("      {} Texture: No texture associated.", name);
        return;
    }
    const cgltf_image* image = textureView.texture->image;
    Mesh::TextureData textureData;

    OCF_LOG_TRACE("      {} Texture: {}", name, image->uri ? image->uri : "No URI");
    if (image && image->uri) {
        textureData.uri = FileSystem::getInstance()->getAssetFullPath(m_gltfPath + "/" + image->uri);
        OCF_LOG_TRACE("        Image URI: {}", image->uri);
    }
    else if (image && image->buffer_view) {
        OCF_LOG_TRACE("        Image is embedded in buffer view.");
    }
    else {
        OCF_LOG_TRACE("        No associated image.");
    }

    const cgltf_sampler* sampler = textureView.texture->sampler;
    if (sampler) {
        OCF_LOG_TRACE("        Sampler: {}", sampler->name ? sampler->name : "Unnamed");
        textureData.sampler.setMagFilter(getMagFilter(sampler->mag_filter));
        textureData.sampler.setMinFilter(getMinFilter(sampler->min_filter));
        textureData.sampler.setWrapModeS(getWrapMode(sampler->wrap_s));
        textureData.sampler.setWrapModeT(getWrapMode(sampler->wrap_t));
    }

    textures[name] = std::move(textureData);
}

} // namespace ocf
