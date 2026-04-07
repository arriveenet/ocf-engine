// SPDX - License - Identifier : MIT

#pragma once

#include "ocf/rhi/Device.h"

namespace ocf::rhi {

class VulkanContext;

class VulkanDevice : public Device {
public:
    VulkanDevice(VulkanContext& context);
    ~VulkanDevice() override;

    TextureHandle createTexture() override;

    SwapchainHandle createSwapchain(Window* window, uint32_t width, uint32_t height) override;

private:
    VulkanContext& m_context;
};

} // namespace ocf::rhi
