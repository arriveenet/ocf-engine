#pragma once

#include "ocf/rhi/DeviceFactory.h"

namespace ocf {
namespace rhi {

class VulkanDeviceFactory : public DeviceFactory {
public:
    std::unique_ptr<Device> create() override;
};

} // namespace rhi
} // namespace ocf
