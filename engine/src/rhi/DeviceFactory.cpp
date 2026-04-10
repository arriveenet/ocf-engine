// SPDX-License-Identifier: MIT

#include "ocf/rhi/DeviceFactory.h"

#include "vulkan/VulkanDeviceFactory.h"

namespace ocf {
namespace rhi {

DeviceFactory& DeviceFactory::getInstance()
{
    static std::unique_ptr<DeviceFactory> instance = CreateNativeFactory();
    return *instance;
}

std::unique_ptr<DeviceFactory> DeviceFactory::CreateNativeFactory()
{
    // TODO: Add platform-specific factory creation logic here
    return std::make_unique<VulkanDeviceFactory>();
}

} // namespace rhi
} // namespace ocf
