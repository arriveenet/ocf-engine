// SPDX-License-Identifier: MIT

#include "VulkanDevice.h"

#include "VulkanContext.h"
#include "VulkanSwapchain.h"

namespace ocf {
namespace rhi {

VulkanDevice::VulkanDevice(VulkanContext& context)
    : m_context(context)
    , m_handleAllocator("Handles", 0x400000u)
{
}

VulkanDevice::~VulkanDevice()
{
    m_context.terminate();
}

TextureHandle VulkanDevice::createTexture()
{
    return TextureHandle();
}

ShaderHandle VulkanDevice::createShader(std::string_view filename)
{
    Handle<VulkanShader> handle = initHandle<VulkanShader>();

    return Handle<RHIShader>{handle.getId()};
}

SwapchainHandle VulkanDevice::createSwapchain(Window* window, uint32_t width, uint32_t height)
{
    Handle<VulkanSwapchain> handle = m_handleAllocator.allocate<Handle<VulkanSwapchain>>();
    m_context.createSwapchain(window, width, height);

    // TODO: Handle create result
    return Handle<RHISwapchain>{handle.getId()};
}

} // namespace rhi
} // namespace ocf