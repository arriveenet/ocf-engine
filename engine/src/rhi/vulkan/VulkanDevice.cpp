// SPDX - License - Identifier : MIT

#include "VulkanDevice.h"

#include "VulkanContext.h"
#include "VulkanSwapchain.h"

namespace ocf {
namespace rhi {

VulkanDevice::VulkanDevice(VulkanContext& context)
    : m_context(context)
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

SwapchainHandle VulkanDevice::createSwapchain(uint32_t width, uint32_t height)
{
    VulkanSwapchain* swapchain = new VulkanSwapchain(m_context);

    // TODO: Handle create result
    return SwapchainHandle();
}

} // namespace rhi
} // namespace ocf