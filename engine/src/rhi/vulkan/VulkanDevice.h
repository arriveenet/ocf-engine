// SPDX - License - Identifier : MIT

#pragma once

#include "ocf/rhi/Device.h"

namespace ocf {
namespace rhi {

class VulkanContext;

class VulkanDevice : public Device {
public:
    VulkanDevice(VulkanContext& context);
    ~VulkanDevice() override;

    TextureHandle createTexture() override;

    SwapchainHandle createSwapchain(uint32_t width, uint32_t height) override;

private:
    VulkanContext& m_context;
};

} // namespace rhi
} // namespace ocf
