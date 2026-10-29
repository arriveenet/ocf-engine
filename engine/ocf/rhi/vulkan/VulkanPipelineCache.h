// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/rhi/RHIEnums.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <type_traits>
#include <unordered_map>

namespace ocf::rhi {

/**
 * Caches VkPipelines by the full pipeline state, like Filament's VulkanPipelineCache.
 * A pipeline is built the first time its state is bound, and destroyed once it has not been used
 * for MAX_PIPELINE_AGE frames.
 */
class VulkanPipelineCache {
public:
    /** One vertex attribute. Unused attributes stay zero so that they do not affect the key. */
    struct VertexAttribute {
        uint32_t location;
        VkFormat format;
        uint32_t offset;
    };

    /**
     * Every state a VkPipeline is built from. It is hashed and compared as raw bytes, so it has
     * no padding and must be value-initialized (PipelineKey key{}) before being filled.
     *
     * Shaders and the layout are identified by ids that are never reused, not by their Vulkan
     * handles, so a destroyed object whose handle value is reused can never hit a stale entry.
     */
    struct PipelineKey {
        uint32_t vertexShader;   ///< VulkanShaderModuleObject::getId()
        uint32_t fragmentShader; ///< VulkanShaderModuleObject::getId()
        uint32_t layout;         ///< Id of the pipeline layout
        uint32_t rasterState;    ///< RasterState::value
        VkPrimitiveTopology topology;
        VkFormat colorFormat; ///< VK_FORMAT_UNDEFINED when the target has no color attachment
        VkFormat depthFormat; ///< VK_FORMAT_UNDEFINED when the target has no depth attachment
        uint32_t vertexStride;
        uint32_t attributeCount;
        VertexAttribute attributes[VERTEX_ATTRIBUTE_COUNT_MAX];
    };

    static_assert(std::has_unique_object_representations_v<PipelineKey>,
                  "PipelineKey must have no padding, as it is hashed as raw bytes");

    /** The objects the ids in the key refer to, used to build the pipeline on a cache miss */
    struct PipelineObjects {
        VkShaderModule vertexShader = VK_NULL_HANDLE;
        const char* vertexEntryPoint = "main";
        VkShaderModule fragmentShader = VK_NULL_HANDLE;
        const char* fragmentEntryPoint = "main";
        VkPipelineLayout layout = VK_NULL_HANDLE;
    };

    /** Frames a pipeline is kept for after its last use */
    static constexpr uint64_t MAX_PIPELINE_AGE = 10;

    explicit VulkanPipelineCache(VkDevice device);
    ~VulkanPipelineCache();

    VulkanPipelineCache(const VulkanPipelineCache&) = delete;
    VulkanPipelineCache& operator=(const VulkanPipelineCache&) = delete;

    /**
     * Returns the pipeline for the key, building it if it is not cached yet.
     * @param frameNumber The frame the pipeline is used in, which keeps it alive until then
     */
    VkPipeline getPipeline(const PipelineKey& key, const PipelineObjects& objects,
                           uint64_t frameNumber);

    /**
     * Destroys the pipelines that have not been used for MAX_PIPELINE_AGE frames and that the GPU
     * is done with.
     */
    void gc(uint64_t currentFrameNumber, uint64_t completedFrameNumber);

    /** Destroys all pipelines. The GPU must be idle. */
    void clear();

private:
    struct KeyHasher {
        size_t operator()(const PipelineKey& key) const noexcept;
    };

    struct KeyEqual {
        bool operator()(const PipelineKey& lhs, const PipelineKey& rhs) const noexcept;
    };

    struct Entry {
        VkPipeline pipeline = VK_NULL_HANDLE;
        uint64_t lastUsed = 0; // Frame number of the last frame that used the pipeline
    };

    VkPipeline buildPipeline(const PipelineKey& key, const PipelineObjects& objects) const;

private:
    VkDevice m_device;
    std::unordered_map<PipelineKey, Entry, KeyHasher, KeyEqual> m_pipelines;
};

} // namespace ocf::rhi
