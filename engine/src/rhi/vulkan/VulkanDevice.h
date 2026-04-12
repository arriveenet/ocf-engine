// SPDX-License-Identifier: MIT

#pragma once

#include "ocf/rhi/Device.h"

#include <vulkan/vulkan.h>

namespace ocf::rhi {

class VulkanContext;

struct VulkanShader : public RHIShader {
    struct VK {
        VkShaderModule id = VK_NULL_HANDLE;
    } vk;

};

class VulkanDevice : public Device {
public:
    VulkanDevice(VulkanContext& context);
    ~VulkanDevice() override;

    TextureHandle createTexture() override;

    ShaderHandle createShader(std::string_view filename) override;

    SwapchainHandle createSwapchain(Window* window, uint32_t width, uint32_t height) override;

private:
    VulkanContext& m_context;
};

} // namespace ocf::rhi
