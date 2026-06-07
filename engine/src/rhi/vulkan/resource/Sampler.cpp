// SPDX-License-Identifier: MIT
#include "Sampler.h"

#include <cassert>

namespace ocf::rhi {

void Sampler::initialize(VkDevice device, VkFilter minFilter, VkFilter magFilter, VkSamplerMipmapMode mipmapMode,
                         VkSamplerAddressMode addressModeU, VkSamplerAddressMode addressModeV,
                         float minLod, float maxLod)
{
    m_device = device;

    VkSamplerCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = magFilter,
        .minFilter =  minFilter,
        .mipmapMode = mipmapMode,
        .addressModeU = addressModeU,
        .addressModeV = addressModeV,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .maxAnisotropy = 1.0f,
        .minLod = minLod,
        .maxLod = maxLod,
    };

    auto result = vkCreateSampler(device, &createInfo, nullptr, &m_sampler);
    if (result != VK_SUCCESS) {
        assert(false);
    }
}

void Sampler::cleanup()
{
    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, m_sampler, nullptr);
    }
    m_sampler = VK_NULL_HANDLE;
}

} // namespace ocf::rhi
