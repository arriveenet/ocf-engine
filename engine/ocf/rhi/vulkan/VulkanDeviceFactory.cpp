// SPDX-License-Identifier: MIT
#include "ocf/rhi/vulkan/VulkanDeviceFactory.h"

#include "ocf/rhi/vulkan/VulkanContext.h"
#include "ocf/rhi/vulkan/VulkanDevice.h"

namespace ocf {
namespace rhi {

std::unique_ptr<Device> VulkanDeviceFactory::create()
{
    static VulkanContext context;
    context.initialize("OCF Engine App");

    Device::DeviceConfig config{};
    config.handlePoolSize = 4u * 1024u * 1024u;

    auto device = std::make_unique<VulkanDevice>(config, context);
    device->initialize();
    return device;
}

} // namespace rhi
} // namespace ocf
