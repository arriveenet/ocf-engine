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

SwapchainHandle VulkanDevice::createSwapchain(Window* window, uint32_t width,
                                              uint32_t height)
{
    m_context.createSwapchain(window, width, height);

    // TODO: Handle create result
    return SwapchainHandle();
}

} // namespace rhi
} // namespace ocf