// SPDX - License - Identifier : MIT

#include "VulkanDevice.h"

#include "VulkanContext.h"

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

} // namespace rhi
} // namespace ocf