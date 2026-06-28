// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/rhi/RHIEnums.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <unordered_map>

namespace ocf::rhi {

class SamplerCache {
public:
    using SamplerKey = uint16_t;

    constexpr SamplerKey makeKey(const SamplerParameters& params)
    {
        return (uint16_t(params.filterMag) << 0) |
               (uint16_t(params.filterMin) << 1) |
               (uint16_t(params.wrapS) << 4) |
               (uint16_t(params.wrapT) << 6) |
               (uint16_t(params.wrapR) << 8) |
               (uint16_t(params.anisotropyLog2) << 10);
    }

    SamplerCache(VkDevice device, float maxAnisotropy);
    ~SamplerCache();

    VkSampler getSampler(const SamplerParameters& params);

    void clear();

private:
    VkSampler createSampler(const SamplerParameters& params, float minLod = 0.0f,
                            float maxLod = VK_LOD_CLAMP_NONE) const;

private:
    VkDevice m_device;
    float m_maxAnisotropy;
    std::unordered_map<SamplerKey, VkSampler> m_cache;
};

} // namespace ocf::rhi
