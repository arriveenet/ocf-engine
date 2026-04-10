// SPDX-License-Identifier: MIT
#include "VulkanDeviceFactory.h"

#include "VulkanContext.h"
#include "VulkanDevice.h"

namespace ocf {
namespace rhi {

std::unique_ptr<Device> VulkanDeviceFactory::create()
{
    static VulkanContext context;
    context.initialize("OCF Engine App");

    return std::make_unique<VulkanDevice>(context);
}

} // namespace rhi
} // namespace ocf
