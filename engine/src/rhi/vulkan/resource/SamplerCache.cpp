// SPDX-License-Identifier: MIT
#include "SamplerCache.h"

#include "vulkan/VulkanUtility.h"

namespace ocf::rhi {

ocf::rhi::SamplerCache::SamplerCache(VkDevice device)
    : m_device(device)
{
}

SamplerCache::~SamplerCache()
{
    clear();
}

VkSampler SamplerCache::getSampler(const SamplerParameters& params)
{
    const SamplerKey key = makeKey(params);

    auto iter = m_cache.find(key);
    if (iter != m_cache.end()) {
        return iter->second;
    }

    VkSampler sampler = createSampler(params);
    m_cache.emplace(key, sampler);

    return sampler;
}

void SamplerCache::clear()
{
    for (auto& [key, sampler] : m_cache) {
        vkDestroySampler(m_device, sampler, nullptr);
    }

    m_cache.clear();
}

VkSampler SamplerCache::createSampler(const SamplerParameters& params, float minLod,
                                      float maxLod) const
{
    VkSampler sampler;

    VkSamplerCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VulkanUtility::getMagFilter(params.filterMag),
        .minFilter = VulkanUtility::getMinFilter(params.filterMin),
        .mipmapMode = VulkanUtility::getMipmapMode(params.filterMin),
        .addressModeU = VulkanUtility::getWrapMode(params.wrapS),
        .addressModeV = VulkanUtility::getWrapMode(params.wrapT),
        .addressModeW = VulkanUtility::getWrapMode(params.wrapR),
        .maxAnisotropy = 1.0f,
        .minLod = minLod,
        .maxLod = maxLod,
    };

    auto result = vkCreateSampler(m_device, &createInfo, nullptr, &sampler);
    VK_CHECK_RESULT(result);

    return sampler;
}

} // namespace ocf::rhi
