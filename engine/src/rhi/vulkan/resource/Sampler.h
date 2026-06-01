#pragma once

#include "GpuResourceBase.h"

#include <vulkan/vulkan.h>

namespace ocf::rhi {

class Sampler : public GpuResourceBase<Sampler> {
    friend class GpuResourceBase<Sampler>;

private:
    Sampler() =default;

public:
    Sampler( const Sampler & ) = delete;
    Sampler & operator=( const Sampler & ) = delete;

    void initialize(VkDevice device, VkFilter minFilter, VkFilter magFilter, VkSamplerMipmapMode mipmapMode,
        VkSamplerAddressMode addressModeU, VkSamplerAddressMode addressModeV,
        float minLod = 0.0f, float maxLod = VK_LOD_CLAMP_NONE);

    void cleanup();

    VkSampler getSamplerHandle() const noexcept { return m_sampler; }
    operator const  VkSampler& () const noexcept { return m_sampler; }

protected:
    VkDevice m_device = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;
};

} // namespace ocf::rhi
