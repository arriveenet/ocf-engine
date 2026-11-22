// SPDX-License-Identifier: MIT
#include "ocf/rhi/d3d12/D3D12DeviceFactory.h"

#include "ocf/rhi/d3d12/D3D12Device.h"

namespace ocf {
namespace rhi {

std::unique_ptr<Device> D3D12DeviceFactory::create()
{
    Device::DeviceConfig config{};
    config.handlePoolSize = 4u * 1024u * 1024u;

    auto device = std::make_unique<D3D12Device>(config);
    device->initialize();
    return device;
}

} // namespace rhi
} // namespace ocf
